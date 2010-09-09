// 
// Licensed to Green Energy Corp (www.greenenergycorp.com) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  Green Enery Corp licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
//  
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
// 
#include "AsyncMaster.h"

#include "AsyncMasterStates.h"
#include "ObjectReadIterator.h"
#include "ResponseLoader.h"

#include <APL/DataInterfaces.h>
#include <APL/AsyncTaskInterfaces.h>
#include <APL/AsyncTaskGroup.h>
#include <APL/AsyncTaskBase.h>
#include <APL/AsyncTaskPeriodic.h>
#include <APL/AsyncTaskNonPeriodic.h>
#include <APL/Logger.h>
#include <APL/TimerInterfaces.h>
#include <APL/AsyncTaskContinuous.h>
#include <APL/CopyableBuffer.h>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

using namespace boost;

namespace apl { namespace dnp {

AsyncMaster::AsyncMaster(Logger* apLogger, MasterConfig aCfg, IAsyncAppLayer* apAppLayer, IDataObserver* apPublisher, AsyncTaskGroup* apTaskGroup, ITimerSource* apTimerSrc, ITimeSource* apTimeSrc) :
Loggable(apLogger),
mCommsStatus(apLogger, "comms_status"),
mRequest(aCfg.FragSize),
mpAppLayer(apAppLayer),
mpPublisher(apPublisher),
mpTaskGroup(apTaskGroup),
mpTimerSrc(apTimerSrc),
mpTimeSrc(apTimeSrc),
mpState(AMS_Closed::Inst()),
mpTask(NULL),
mpScheduledTask(NULL),
mSchedule(MasterSchedule::GetSchedule(aCfg, this, apTaskGroup)),
mClassPoll(apLogger, apPublisher),
mClearRestart(apLogger),
mConfigureUnsol(apLogger),
mTimeSync(apLogger, apTimeSrc),
mExecuteBO(apLogger),
mExecuteSP(apLogger)
{
	mCommandQueue.SetNotifier(mNotifierSource.Get(boost::bind(&AsyncTaskBase::Enable, mSchedule.mpCommandTask), mpTimerSrc));
	mCommsStatus.Set(COMMS_DOWN);
}

void AsyncMaster::ProcessIIN(const IINField& arIIN)
{
	bool check_state = false;

	//The clear IIN task only happens in response to detecting an IIN bit.
	if(arIIN.GetNeedTime()) {
		LOG_BLOCK(LEV_INFO, "Need time detected");
		mSchedule.mpTimeTask->SilentEnable();
		check_state = true;
	}

	if(mLastIIN.GetDeviceTrouble()) LOG_BLOCK(LEV_WARNING, "IIN Device trouble detected");

	if(mLastIIN.GetEventBufferOverflow()) LOG_BLOCK(LEV_WARNING, "Event buffer overflow detected");

	// If this is detected, we need to reset the startup tasks
	if(mLastIIN.GetDeviceRestart()) {
		LOG_BLOCK(LEV_WARNING, "Device restart detected");
		mSchedule.ResetStartupTasks();
		mSchedule.mpClearRestartTask->SilentEnable();
		check_state = true;
	}

	if(check_state) mpTaskGroup->CheckState();
}

void AsyncMaster::ProcessCommand(ITask* apTask)
{
	CommandData info;

	if(mpState == AMS_Closed::Inst()) { //we're closed
		if(!mCommandQueue.RespondToCommand(CS_HARDWARE_ERROR)) apTask->Disable();
	}
	else {

		switch(mCommandQueue.Next()) 
		{
			case(apl::CT_BINARY_OUTPUT): 
			{
				apl::BinaryOutput cmd;
				mCommandQueue.Read(cmd, info);
				mExecuteBO.Set(cmd, info, true);
				mpState->StartTask(this, apTask, &mExecuteBO);
			}
			break;
			case(apl::CT_SETPOINT):
			{
				apl::Setpoint cmd;
				mCommandQueue.Read(cmd, info);
				mExecuteSP.Set(cmd, info, true);
				mpState->StartTask(this, apTask, &mExecuteSP);
			}
			break;
			default:
				apTask->Disable(); //no commands to be read
				break;					
		}
	}
}

void AsyncMaster::StartTask(MasterTaskBase* apMasterTask, bool aInit)
{
	if(aInit) apMasterTask->Init();
	apMasterTask->ConfigureRequest(mRequest);
	mpAppLayer->SendRequest(mRequest);
}

/* Tasks */

void AsyncMaster::SyncTime(ITask* apTask)
{
	if(mLastIIN.GetNeedTime())
	{
		mpState->StartTask(this, apTask, &mTimeSync);
	}
	else apTask->Disable();
}

void AsyncMaster::WriteIIN(ITask* apTask)
{
	if(mLastIIN.GetDeviceRestart()) 
	{
		mpState->StartTask(this, apTask, &mClearRestart);
	}
	else apTask->Disable();
}

void AsyncMaster::IntegrityPoll(ITask* apTask)
{
	mClassPoll.Set(PC_CLASS_0);
	mpState->StartTask(this, apTask, &mClassPoll);
}

void AsyncMaster::EventPoll(ITask* apTask, int aClassMask)
{
	mClassPoll.Set(aClassMask);
	mpState->StartTask(this, apTask, &mClassPoll);
}

void AsyncMaster::ChangeUnsol(ITask* apTask, bool aEnable, int aClassMask)
{
	mConfigureUnsol.Set(aEnable, aClassMask);
	mpState->StartTask(this, apTask, &mConfigureUnsol);
}

/* Implement IAsyncAppUser */

void AsyncMaster::OnLowerLayerUp()
{
	mpState->OnLowerLayerUp(this);
	mSchedule.EnableOnlineTasks();
}

void AsyncMaster::OnLowerLayerDown()
{
	mpState->OnLowerLayerDown(this);
	mSchedule.DisableOnlineTasks();
	mCommsStatus.Set(COMMS_DOWN);
}

void AsyncMaster::OnSolSendSuccess()
{
	mpState->OnSendSuccess(this);
}

void AsyncMaster::OnSolFailure()
{
	mpState->OnFailure(this);
}

void AsyncMaster::OnUnsolSendSuccess()
{
	throw InvalidStateException(LOCATION, "Master can't send unsol");
}

void AsyncMaster::OnUnsolFailure()
{
	throw InvalidStateException(LOCATION, "Master can't send unsol");
}

void AsyncMaster::OnPartialResponse(const APDU& arAPDU)
{
	mLastIIN = arAPDU.GetIIN();
	this->ProcessIIN(mLastIIN);
	mpState->OnPartialResponse(this, arAPDU);
	mCommsStatus.Set(COMMS_UP);
}

void AsyncMaster::OnFinalResponse(const APDU& arAPDU)
{
	mLastIIN = arAPDU.GetIIN();
	this->ProcessIIN(arAPDU.GetIIN());
	mpState->OnFinalResponse(this, arAPDU);
	mCommsStatus.Set(COMMS_UP);
}

void AsyncMaster::OnUnsolResponse(const APDU& arAPDU)
{
	mLastIIN = arAPDU.GetIIN();
	this->ProcessIIN(mLastIIN);
	mpState->OnUnsolResponse(this, arAPDU);	
	mCommsStatus.Set(COMMS_UP);
}

/* Private functions */

void AsyncMaster::ProcessDataResponse(const APDU& arResponse)
{
	try {
		ResponseLoader loader(mpLogger, mpPublisher);

		for(HeaderReadIterator hdr = arResponse.BeginRead(); !hdr.IsEnd(); ++hdr) 
			loader.Process(hdr);		
	}
	catch(Exception ex)
	{
		EXCEPTION_BLOCK(LEV_WARNING, ex)
	}
}

}} //end ns
