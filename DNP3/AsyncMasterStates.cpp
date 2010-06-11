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

void AMS_Base::WriteIIN(AsyncMaster*, ITaskCompletion*)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::IntegrityPoll(AsyncMaster*, ITaskCompletion*)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::Execute(AsyncMaster* c, const BinaryOutput&, size_t)
{ c->CompleteCommandTask(CS_HARDWARE_ERROR); }

void AMS_Base::Execute(AsyncMaster* c, const Setpoint&, size_t)
{ c->CompleteCommandTask(CS_HARDWARE_ERROR); }

void AMS_Base::EventPoll(AsyncMaster*, ITaskCompletion* apTask, int aClassMask)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::ChangeUnsol(AsyncMaster*, ITaskCompletion* apTask, bool aEnable, int aClassMask)
{ throw InvalidStateException(LOCATION, this->Name()); }

void AMS_Base::SyncTime(AsyncMaster*, ITaskCompletion* apTask)
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

void AMS_Base::SetTask(AsyncMaster* c, ITaskCompletion* apTask)
{
	c->mpTask = apTask;
}

void AMS_Base::ConfigureClearIIN(AsyncMaster* c)
{
	c->mRequest.Set(FC_WRITE);
	Group80Var1* pObj = Group80Var1::Inst(); // Internal indications object
	ObjectWriteIterator i = c->mRequest.WriteContiguous(pObj, 7, 7);
	pObj->Write(*i, 7, 7, false);
}

void AMS_Base::ConfigureIntegrityPoll(AsyncMaster* c)
{
	c->mRequest.Set(FC_READ);
	c->mRequest.DoPlaceholderWrite(Group60Var1::Inst());
}

void AMS_Base::ConfigureChangeUnsol(AsyncMaster* c, bool aEnable, int aClassMask)
{
	c->mRequest.Set(aEnable ? FC_ENABLE_UNSOLICITED : FC_DISABLE_UNSOLICITED);
	if(aClassMask & PC_CLASS_1) c->mRequest.DoPlaceholderWrite(Group60Var2::Inst());
	if(aClassMask & PC_CLASS_2) c->mRequest.DoPlaceholderWrite(Group60Var3::Inst());
	if(aClassMask & PC_CLASS_3) c->mRequest.DoPlaceholderWrite(Group60Var4::Inst());
}

void AMS_Base::ConfigureEventPoll(AsyncMaster* c, int aClassMask)
{
	c->mRequest.Set(FC_READ);
	if(aClassMask & PC_CLASS_1) c->mRequest.DoPlaceholderWrite(Group60Var2::Inst());
	if(aClassMask & PC_CLASS_2) c->mRequest.DoPlaceholderWrite(Group60Var3::Inst());
	if(aClassMask & PC_CLASS_3) c->mRequest.DoPlaceholderWrite(Group60Var4::Inst());
}

void AMS_Base::ConfigureDelayMeas(AsyncMaster* c)
{
	c->mRequest.Set(FC_DELAY_MEASURE);
	c->mDelayValidator = boost::bind(&AsyncMaster::ValidateDelayMeas, c, _1, c->mpTimeSrc->GetUTC(), _2);
}

void AMS_Base::ConfigureWriteDelay(AsyncMaster* c, millis_t aDelay)
{
	c->mRequest.Set(FC_WRITE);
	ObjectWriteIterator owi = c->mRequest.WriteContiguous(Group50Var1::Inst(), 0, 0, QC_1B_CNT);
	Group50Var1::Inst()->mTime.Set(*owi, c->mpTimeSrc->GetTimeStampUTC() + aDelay);
}

void AMS_Base::ConfigureCommand(AsyncMaster* c, const BinaryOutput& arCmd, size_t aIndex, bool aIsSelect)
{
	c->mFormatter = boost::bind(&AsyncMaster::FormatBinaryOutput, c, arCmd, aIndex, _1);
	CopyableBuffer buff = c->mFormatter(aIsSelect);
	c->mValidator = boost::bind(&AsyncMaster::ValidateCommandResponse<BinaryOutput>, c, _1, Group12Var1::Inst(), buff, aIndex);
	
}

void AMS_Base::ConfigureCommand(AsyncMaster* c, const Setpoint& arCmd, size_t aIndex, bool aIsSelect)
{
	CommandObject<Setpoint>* pObj = this->GetOptimalEncoder(arCmd.GetOptimalEncodingType());
	c->mFormatter = boost::bind(&AsyncMaster::FormatSetpoint, c, arCmd, pObj, aIndex, _1);
	CopyableBuffer buff = c->mFormatter(aIsSelect);
	c->mValidator = boost::bind(&AsyncMaster::ValidateCommandResponse<Setpoint>, c, _1, pObj, buff, aIndex);
}

CommandObject<Setpoint>* AMS_Base::GetOptimalEncoder(SetpointEncodingType aType)
{
	switch(aType) {
		case SPET_INT16: return Group41Var2::Inst();				
		case SPET_INT32: return Group41Var1::Inst();		
		case SPET_FLOAT: return Group41Var3::Inst();
		case SPET_DOUBLE: return Group41Var4::Inst();		
	default:
		throw ArgumentException(LOCATION, "Enum not handled");
	}
}


void AMS_Base::SetToOperate(AsyncMaster* c)
{
	c->mFormatter(false);
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
	return c->mValidator(arAPDU);
}

bool AMS_Base::ValidateDelayMeas(AsyncMaster* c, const APDU& arAPDU, millis_t& arDelay)
{
	return c->mDelayValidator(arAPDU, arDelay);
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
	ProcessDataResponse(c, arAPDU);
}


/* AMS_Idle */

AMS_Idle AMS_Idle::mInstance;

void AMS_Idle::WriteIIN(AsyncMaster* c, ITaskCompletion* apTask)
{
	ChangeState(c, AMS_WaitForSimpleRsp::Inst());
	SetTask(c, apTask);
	ConfigureClearIIN(c);
	SendRequest(c);
}

void AMS_Idle::IntegrityPoll(AsyncMaster* c, ITaskCompletion* apTask)
{
	ChangeState(c, AMS_WaitForRspToPoll::Inst());
	SetTask(c, apTask);
	ConfigureIntegrityPoll(c);
	SendRequest(c);
}

void AMS_Idle::EventPoll(AsyncMaster* c, ITaskCompletion* apTask, int aClassMask)
{
	ChangeState(c, AMS_WaitForRspToPoll::Inst());
	SetTask(c, apTask);
	ConfigureEventPoll(c, aClassMask);
	SendRequest(c);
}

void AMS_Idle::ChangeUnsol(AsyncMaster* c, ITaskCompletion* apTask, bool aEnable, int aClassMask)
{
	ChangeState(c, AMS_WaitForSimpleRsp::Inst());
	SetTask(c, apTask);
	ConfigureChangeUnsol(c, aEnable, aClassMask);
	SendRequest(c);
}

void AMS_Idle::SyncTime(AsyncMaster* c, ITaskCompletion* apTask)
{
	ChangeState(c, AMS_WaitForDelayMeasRsp::Inst());
	SetTask(c, apTask);
	ConfigureDelayMeas(c);
	SendRequest(c);
}

void AMS_Idle::Execute(AsyncMaster* c, const BinaryOutput& arCmd, size_t aIndex)
{ 
	ChangeState(c, AMS_WaitForRspToSelect::Inst());
	ConfigureCommand(c, arCmd, aIndex, true);
	SendRequest(c);
}

void AMS_Idle::Execute(AsyncMaster* c, const Setpoint& arCmd, size_t aIndex)
{ 
	ChangeState(c, AMS_WaitForRspToSelect::Inst());
	ConfigureCommand(c, arCmd, aIndex, true);
	SendRequest(c);
}

void AMS_Idle::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
}

/* AMS_WaitForSimpleRsp */

AMS_WaitForSimpleRsp AMS_WaitForSimpleRsp::mInstance;

void AMS_WaitForSimpleRsp::OnFailure(AsyncMaster* c)
{
	ChangeState(c, AMS_Idle::Inst());
	CompleteTask(c, false);
}

void AMS_WaitForSimpleRsp::OnPartialResponse(AsyncMaster* c, const APDU&)
{
	ERROR_LOGGER_BLOCK(GetLogger(c), LEV_WARNING, "Unexpected multi-fragment response", MERR_UNEXPECTED_MULTI_FRAG_RSP);
}

void AMS_WaitForSimpleRsp::OnFinalResponse(AsyncMaster* c, const APDU& arAPDU)
{
	ChangeState(c, AMS_Idle::Inst());
	CompleteTask(c, true);
}

void AMS_WaitForSimpleRsp::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
}

/* AMS_WaitForDelayMeasRsp */

AMS_WaitForDelayMeasRsp AMS_WaitForDelayMeasRsp::mInstance;

void AMS_WaitForDelayMeasRsp::OnFailure(AsyncMaster* c)
{
	ChangeState(c, AMS_Idle::Inst());
	CompleteTask(c, false);
}

void AMS_WaitForDelayMeasRsp::OnPartialResponse(AsyncMaster* c, const APDU&)
{
	ERROR_LOGGER_BLOCK(GetLogger(c), LEV_WARNING, "Unexpected multi-fragment response", MERR_UNEXPECTED_MULTI_FRAG_RSP);
}

void AMS_WaitForDelayMeasRsp::OnFinalResponse(AsyncMaster* c, const APDU& arAPDU)
{
	millis_t delay;
	if(this->ValidateDelayMeas(c, arAPDU, delay)) {
		ChangeState(c, AMS_WaitForSimpleRsp::Inst());
		ConfigureWriteDelay(c, delay);
		SendRequest(c);
	}
	else {
		ChangeState(c, AMS_Idle::Inst());
		CompleteTask(c, false);
	}
}

void AMS_WaitForDelayMeasRsp::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
}


/* AMS_WaitForRspToPoll */

AMS_WaitForRspToPoll AMS_WaitForRspToPoll::mInstance;

void AMS_WaitForRspToPoll::OnFailure(AsyncMaster* c)
{
	ChangeState(c, AMS_Idle::Inst());
	CompleteTask(c, false);
}

void AMS_WaitForRspToPoll::OnPartialResponse(AsyncMaster* c, const APDU& arAPDU)
{
	ProcessDataResponse(c, arAPDU);
}

void AMS_WaitForRspToPoll::OnFinalResponse(AsyncMaster* c, const APDU& arAPDU)
{
	ChangeState(c, AMS_Idle::Inst());
	ProcessDataResponse(c, arAPDU);
	CompleteTask(c, true);
}

void AMS_WaitForRspToPoll::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
}

/* AMS_WaitForRspToSelect */

AMS_WaitForRspToSelect AMS_WaitForRspToSelect::mInstance;

void AMS_WaitForRspToSelect::OnFailure(AsyncMaster* c)
{
	ChangeState(c, AMS_Idle::Inst());
	this->CompleteCommandTask(c, CS_HARDWARE_ERROR);
}

void AMS_WaitForRspToSelect::OnPartialResponse(AsyncMaster*, const APDU&)
{
	
}

void AMS_WaitForRspToSelect::OnFinalResponse(AsyncMaster* c, const APDU& arAPDU)
{
	CommandStatus status = this->Validate(c, arAPDU);
	if(status == CS_SUCCESS)
	{
		ChangeState(c, AMS_WaitForRspToOperate::Inst());	
		this->SetToOperate(c);
		SendRequest(c);
	}
	else 
	{
		ChangeState(c, AMS_Idle::Inst());
		this->CompleteCommandTask(c, status);
	}
}

void AMS_WaitForRspToSelect::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
	this->CompleteCommandTask(c, CS_HARDWARE_ERROR);
}

/* AMS_WaitForRspToOperate */

AMS_WaitForRspToOperate AMS_WaitForRspToOperate::mInstance;

void AMS_WaitForRspToOperate::OnFailure(AsyncMaster* c)
{
	ChangeState(c, AMS_Idle::Inst());
	this->CompleteCommandTask(c, CS_HARDWARE_ERROR);
}

void AMS_WaitForRspToOperate::OnPartialResponse(AsyncMaster*, const APDU&)
{
	
}

void AMS_WaitForRspToOperate::OnFinalResponse(AsyncMaster* c, const APDU& arAPDU)
{
	ChangeState(c, AMS_Idle::Inst());
	CommandStatus status = this->Validate(c, arAPDU);
	this->CompleteCommandTask(c, status);
}

void AMS_WaitForRspToOperate::OnLowerLayerDown(AsyncMaster* c)
{
	ChangeState(c, AMS_Closed::Inst());
	this->CompleteCommandTask(c, CS_HARDWARE_ERROR);
}

}} //ens ns

