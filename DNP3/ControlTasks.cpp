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

#include "ControlTasks.h"
#include "APDU.h"
#include <boost/bind.hpp>

namespace apl { namespace dnp {

ControlTaskBase::ControlTaskBase(Logger* apLogger) : 
SingleRspBase(apLogger),
mState(INVALID)
{}

bool ControlTaskBase::GetSelectBit()
{
	switch(mState) {
		case(SELECT): return true;
		case(OPERATE): return false;
		default:
			throw InvalidStateException(LOCATION, "INVALID");
	}
}

/*
TaskResult ControlTaskBase::_OnFinalResponse(const APDU& arAPDU)
{
	
}
*/

/* -------- BinaryOutputTask -------- */

BinaryOutputTask::BinaryOutputTask(Logger* apLogger) : 
ControlTask<BinaryOutput>(apLogger)
{}

CommandObject<BinaryOutput>* BinaryOutputTask::GetObject(const BinaryOutput&) 
{ return Group12Var1::Inst(); }

/* -------- SetpointTask -------- */

SetpointTask::SetpointTask(Logger* apLogger) :  ControlTask<Setpoint>(apLogger)
{}

CommandObject<Setpoint>* SetpointTask::GetOptimalEncoder(SetpointEncodingType aType)
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

CommandObject<Setpoint>* SetpointTask::GetObject(const Setpoint& arSetpoint) 
{
	return GetOptimalEncoder(arSetpoint.GetEncodingType());
}



}} //ens ns
