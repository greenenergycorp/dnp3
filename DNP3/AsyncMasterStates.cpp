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

void AMS_Base::StartTask(AsyncMaster*, MasterTaskBase*)
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


void AMS_Base::ChangeState(AsyncMaster* c, AMS_Base* apState)
{
	c->mpState = apState;
}


/*
void AMS_Base::SetTask(AsyncMaster* c, ITaskCompletion* apTask)
{
	c->mpTask = apTask;
}

void AMS_Base::SetToOperate(AsyncMaster* c)
{
	//c->mFormatter(false);
}

void AMS_Base::SendRequest(AsyncMaster* c)
{ c->mpAppLayer->SendRequest(c->mRequest); }

void AMS_Base::CompleteTask(AsyncMaster* c, bool aSuccess)
{ c->mpTask->OnComplete(aSuccess); }

Logger* AMS_Base::GetLogger(AsyncMaster* c)
{ return c->mpLogger; }

void AMS_Base::ProcessDataResponse(AsyncMaster* c, const APDU& arAPDU)
{ c->ProcessDataResponse(arAPDU); }

void AMS_Base::CompleteCommandTask(AsyncMaster* c, CommandStatus aStatus)
{ c->CompleteCommandTask(aStatus); }

CommandStatus AMS_Base::Validate(AsyncMaster* c, const APDU& arAPDU)
{
	return CS_SUCCESS; //return c->mValidator(arAPDU);
}

bool AMS_Base::ValidateDelayMeas(AsyncMaster* c, const APDU& arAPDU, millis_t& arDelay)
{
	return true; //return c->mDelayValidator(arAPDU, arDelay);
}
*/


/* AMS_Closed */

AMS_Closed AMS_Closed::mInstance;

void AMS_Closed::OnLowerLayerUp(AsyncMaster* c)
{
	ChangeState(c, AMS_Idle::Inst());
}

/* AMS_OpenBase */

void AMS_OpenBase::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
}

/* AMS_Idle */

AMS_Idle AMS_Idle::mInstance;

void AMS_Idle::StartTask(AsyncMaster* c, MasterTaskBase* t)
{
	
}


/* AMS_WaitForSimpleRsp */

AMS_Waiting AMS_Waiting::mInstance;

void AMS_Waiting::OnFailure(AsyncMaster* c)
{
	
	
}

void AMS_Waiting::OnPartialResponse(AsyncMaster* c, const APDU&)
{
	
}

void AMS_Waiting::OnFinalResponse(AsyncMaster* c, const APDU& arAPDU)
{
	
}

}} //ens ns

