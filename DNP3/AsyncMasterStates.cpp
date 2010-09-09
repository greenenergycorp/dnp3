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
#include "AsyncMasterStates.h"

#include <APL/Configure.h>
#include <APL/Exception.h>
#include <APL/Logger.h>
#include <APL/AsyncTaskInterfaces.h>
#include <APL/AsyncTaskGroup.h>

#include "AsyncMaster.h"
#include <boost/bind.hpp>


namespace apl { namespace dnp {

/* AMS_Base */

void AMS_Base::StartTask(AsyncMaster*, ITask*, MasterTaskBase*)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::OnLowerLayerUp(AsyncMaster*)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::OnLowerLayerDown(AsyncMaster*)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::OnSendSuccess(AsyncMaster*)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::OnFailure(AsyncMaster*)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::OnPartialResponse(AsyncMaster*, const APDU&)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::OnFinalResponse(AsyncMaster*, const APDU&)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::OnUnsolResponse(AsyncMaster*, const APDU&)
{ throw InvalidStateException(LOCATION, this->Name()); }


void AMS_Base::ChangeState(AsyncMaster* c, AMS_Base* apState)
{
	c->mpState = apState;
}

void AMS_Base::ChangeTask(AsyncMaster* c, MasterTaskBase* apTask)
{
	c->mpTask = apTask;
}

/* AMS_Closed */

AMS_Closed AMS_Closed::mInstance;

void AMS_Closed::OnLowerLayerUp(AsyncMaster* c)
{
	ChangeState(c, AMS_Idle::Inst());
}

/* AMS_OpenBase */

void AMS_OpenBase::OnUnsolResponse(AsyncMaster* c, const APDU& arAPDU)
{
	c->ProcessDataResponse(arAPDU);
}

void AMS_OpenBase::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
}

/* AMS_Idle */

AMS_Idle AMS_Idle::mInstance;

void AMS_Idle::StartTask(AsyncMaster* c, ITask* apScheTask, MasterTaskBase* apMasterTask)
{
	this->ChangeState(c, AMS_Waiting::Inst());
	this->ChangeTask(c, apMasterTask);
	c->mpScheduledTask = apScheTask;
	c->StartTask(apMasterTask, true);	
}


/* AMS_WaitForSimpleRsp */

AMS_Waiting AMS_Waiting::mInstance;

void AMS_Waiting::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
	c->mpTask->OnFailure();
}

void AMS_Waiting::OnFailure(AsyncMaster* c)
{	
	this->ChangeState(c, AMS_Idle::Inst());
	c->mpTask->OnFailure();
	c->mpScheduledTask->OnComplete(false);	
}

void AMS_Waiting::OnPartialResponse(AsyncMaster* c, const APDU& arAPDU)
{
	switch(c->mpTask->OnPartialResponse(arAPDU)) {		
		case(TR_FAIL):
			this->ChangeState(c, AMS_Idle::Inst());			
			c->mpScheduledTask->OnComplete(false);
			break;
		case(TR_CONTINUE):
			c->StartTask(c->mpTask, false);
			break;
		default:
			throw InvalidStateException(LOCATION, "Tasks must return FAIL or CONTINUE in on partial responses");
	}
}

void AMS_Waiting::OnFinalResponse(AsyncMaster* c, const APDU& arAPDU)
{
	switch(c->mpTask->OnFinalResponse(arAPDU)) {		
		case(TR_FAIL):
			this->ChangeState(c, AMS_Idle::Inst());			
			c->mpScheduledTask->OnComplete(false);
			break;
		case(TR_CONTINUE):	//multi request task!
			c->StartTask(c->mpTask, false);
			break;
		case(TR_SUCCESS):
			this->ChangeState(c, AMS_Idle::Inst());
			c->mpScheduledTask->OnComplete(true);		
	}
}

}} //ens ns

