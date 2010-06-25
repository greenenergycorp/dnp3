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
mpState(AMS_Closed::Inst()),
mpTask(NULL),
mRequest(aCfg.FragSize),
mpAppLayer(apAppLayer),
mpPublisher(apPublisher),
mpTaskGroup(apTaskGroup),
mpTimerSrc(apTimerSrc),
mpTimeSrc(apTimeSrc),
mpCommandTask(NULL),
mpTimeTask(NULL),
mCommsStatus(apLogger, "comms_status")
{
	AsyncTaskBase* pIntegrity = mpTaskGroup->Add(aCfg.IntegrityRate, aCfg.TaskRetryRate, AMP_POLL, bind(&AsyncMaster::IntegrityPoll, this, _1), "Integrity Poll");	
	pIntegrity->SetFlags(ONLINE_ONLY_TASKS | START_UP_TASKS);
	
	if(aCfg.DoUnsolOnStartup)
	{
		// DNP3Spec-V2-Part2-ApplicationLayer-_20090315.pdf, page 8 says that UNSOL should be disabled before an integrity scan is done 
		TaskHandler handler = bind(&AsyncMaster::ChangeUnsol, this, _1, false, PC_ALL_EVENTS);
		AsyncTaskBase* pUnsolDisable = mpTaskGroup->Add(-1, aCfg.TaskRetryRate, AMP_UNSOL_CHANGE, handler, "Unsol Disable");
		pUnsolDisable->SetFlags(ONLINE_ONLY_TASKS | START_UP_TASKS);
		pIntegrity->AddDependency(pUnsolDisable);

		if(aCfg.EnableUnsol)
		{
			TaskHandler handler = bind(&AsyncMaster::ChangeUnsol, this, _1, true, aCfg.UnsolClassMask);
			AsyncTaskBase* pUnsolEnable = mpTaskGroup->Add(-1, aCfg.TaskRetryRate, AMP_UNSOL_CHANGE, handler, "Unsol Enable");
			pUnsolEnable->AddDependency(pIntegrity);
			pUnsolEnable->SetFlags(ONLINE_ONLY_TASKS | START_UP_TASKS);
		}
	}

	// load any exception scans and make them dependent on the integrity poll
	BOOST_FOREACH(ExceptionScan e, aCfg.mScans)
	{
		AsyncTaskBase* pEventScan = mpTaskGroup->Add(e.ScanRate, aCfg.TaskRetryRate, AMP_POLL, bind(&AsyncMaster::EventPoll, this, _1, e.ClassMask), "Event Scan");
		pEventScan->SetFlags(ONLINE_ONLY_TASKS);
		pEventScan->AddDependency(pIntegrity);
	}

	// Tasks are executed when the master is is idle
	mpTimeTask = mpTaskGroup->AddContinuous(AMP_TIME_SYNC, boost::bind(&AsyncMaster::SyncTime, this, _1), "TimeSync");	
	mpCommandTask = mpTaskGroup->AddContinuous(AMP_COMMAND, boost::bind(&AsyncMaster::ExecuteCommand, this), "Command");
	mpClearRestartTask = mpTaskGroup->AddContinuous(AMP_CLEAR_RESTART, boost::bind(&AsyncMaster::WriteIIN, this, _1), "Clear IIN");

	mpClearRestartTask->SetFlags(ONLINE_ONLY_TASKS);
	mpTimeTask->SetFlags(ONLINE_ONLY_TASKS);

	mCommandQueue.SetNotifier(mNotifierSource.Get(boost::bind(&AsyncTaskBase::Enable, mpCommandTask), mpTimerSrc));

	mCommsStatus.Set(0);
}

void AsyncMaster::EnableOnlineTasks()
{
	mpTaskGroup->Enable(ONLINE_ONLY_TASKS);
}

void AsyncMaster::DisableOnlineTasks()
{
	mpTaskGroup->Disable(ONLINE_ONLY_TASKS);
}

void AsyncMaster::ProcessIIN(const IINField& arIIN)
{
	bool check_state = false;

	//The clear IIN task only happens in response to detecting an IIN bit.
	if(arIIN.GetNeedTime()) {
		LOG_BLOCK(LEV_INFO, "Need time detected");
		mpTimeTask->SilentEnable();
		check_state = true;
	}

	if(mLastIIN.GetDeviceTrouble()) LOG_BLOCK(LEV_WARNING, "IIN Device trouble detected");

	if(mLastIIN.GetEventBufferOverflow()) LOG_BLOCK(LEV_WARNING, "Event buffer overflow detected");

	// If this is detected, we need to reset the startup tasks
	if(mLastIIN.GetDeviceRestart()) {
		LOG_BLOCK(LEV_WARNING, "Device restart detected");
		mpTaskGroup->ResetTasks(START_UP_TASKS);
		mpClearRestartTask->SilentEnable();
		check_state = true;
	}

	if(check_state) mpTaskGroup->CheckState();
}

void AsyncMaster::ExecuteCommand()
{
	switch(mCommandQueue.Next())
	{
		case(CT_BINARY_OUTPUT):
		{
			BinaryOutput b;
			mCommandQueue.Read(b, mCmdInfo);
			mpState->Execute(this, b, mCmdInfo.mIndex);			
			break;
		}
		case(CT_SETPOINT):
		{
			Setpoint s;
			mCommandQueue.Read(s, mCmdInfo);
			mpState->Execute(this, s, mCmdInfo.mIndex);
			break;
		}
		case(CT_NONE):
		{
			mpCommandTask->Disable();
			break;
		}
	}
}

void AsyncMaster::SyncTime(ITaskCompletion* apTask)
{
	if(!mLastIIN.GetNeedTime()) mpTimeTask->Disable();
	else mpState->SyncTime(this, apTask);
}

void AsyncMaster::CompleteCommandTask(CommandStatus aStatus)
{
	CommandResponse	rsp(aStatus);
	mCmdInfo.mpRspAcceptor->AcceptResponse(rsp, mCmdInfo.mSequence);
	mpCommandTask->OnComplete(true);
}

CopyableBuffer AsyncMaster::FormatSetpoint(const Setpoint& arCmd, CommandObject<Setpoint>* apObj, size_t aIndex, bool aIsSelect)
{
	mRequest.Set(aIsSelect ? FC_SELECT : FC_OPERATE, true, true, false, false);
	IndexedWriteIterator i = mRequest.WriteIndexed(apObj, 1, aIndex);
	i.SetIndex(aIndex);
	apObj->Write(*i, arCmd);
	return apObj->GetValueBytes(*i);
}

CopyableBuffer AsyncMaster::FormatBinaryOutput(const BinaryOutput& arCmd, size_t aIndex, bool aIsSelect)
{
	mRequest.Set(aIsSelect ? FC_SELECT : FC_OPERATE, true, true, false, false);
	IndexedWriteIterator i = mRequest.WriteIndexed(Group12Var1::Inst(), 1, aIndex);
	i.SetIndex(aIndex);
	Group12Var1::Inst()->Write(*i, arCmd);
	return Group12Var1::Inst()->GetValueBytes(*i);
}

bool AsyncMaster::ValidateDelayMeas(const APDU& arAPDU, ptime aStart, millis_t& arDelay)
{
	ptime now = mpTimeSrc->GetUTC();

	HeaderReadIterator hri = arAPDU.BeginRead();
	if(hri.Count() != 1) {
		LOG_BLOCK(LEV_WARNING, "DelayMeas response w/ unexcpected header count");
		return false;
	}
	
	if(!hri->GetBaseObject()->Equals(Group52Var2::Inst())) {
		LOG_BLOCK(LEV_WARNING, "DelayMeas response w/ unexpected object ");
		return false;
	}

	ObjectReadIterator ori = hri.BeginRead();
	if(ori.Count() != 1) {
		LOG_BLOCK(LEV_WARNING, "DelayMeas got more than 1 object in response");
		return false;
	}

	millis_t send_rcv_time = (now - aStart).total_milliseconds();
	millis_t rtu_turn_around = Group52Var2::Inst()->mTime.Get(*ori);

	// The later shouldn't happen, but could cause a negative delay which would
	// result in a weird time setting
	arDelay = (send_rcv_time >= rtu_turn_around) ? (send_rcv_time - rtu_turn_around)/2 : 0;

	

	return true;
}

/* Tasks */

void AsyncMaster::WriteIIN(ITaskCompletion* apTask)
{
	if(mLastIIN.GetDeviceRestart())
	{
		mpState->WriteIIN(this, apTask);
	}
	else mpClearRestartTask->Disable();
}

void AsyncMaster::IntegrityPoll(ITaskCompletion* apTask)
{
	mpState->IntegrityPoll(this, apTask);
}

void AsyncMaster::EventPoll(ITaskCompletion* apTask, int aClassMask)
{
	mpState->EventPoll(this, apTask, aClassMask);
}

void AsyncMaster::ChangeUnsol(ITaskCompletion* apTask, bool aEnable, int aClassMask)
{
	mpState->ChangeUnsol(this, apTask, aEnable, aClassMask);
}

/* Implement IAsyncAppUser */

void AsyncMaster::OnLowerLayerUp()
{
	mpState->OnLowerLayerUp(this);
	this->EnableOnlineTasks();
}

void AsyncMaster::OnLowerLayerDown()
{
	mpState->OnLowerLayerDown(this);
	this->DisableOnlineTasks();
	mCommsStatus.Set(0);
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
	mCommsStatus.Set(1);
}

void AsyncMaster::OnFinalResponse(const APDU& arAPDU)
{
	mLastIIN = arAPDU.GetIIN();
	this->ProcessIIN(arAPDU.GetIIN());
	mpState->OnFinalResponse(this, arAPDU);
	mCommsStatus.Set(1);
}

void AsyncMaster::OnUnsolResponse(const APDU& arAPDU)
{
	mLastIIN = arAPDU.GetIIN();
	this->ProcessIIN(mLastIIN);
	mpState->OnUnsolResponse(this, arAPDU);
	mCommsStatus.Set(1);
}

/* Private functions */

void AsyncMaster::ProcessDataResponse(const APDU& arResponse)
{
	try {
		ResponseLoader loader(mpLogger, mpTimeSrc->GetTimeStampUTC(), mpPublisher);

		for(HeaderReadIterator hdr = arResponse.BeginRead(); !hdr.IsEnd(); ++hdr) {
			loader.Process(hdr);
		}
	}
	catch(Exception ex)
	{
		EXCEPTION_BLOCK(LEV_WARNING, ex)
	}
}

}} //end ns
