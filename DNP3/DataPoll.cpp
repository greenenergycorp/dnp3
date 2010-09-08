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

#include <APL/ITimeSource.h>

#include "DataPoll.h"
#include "APDU.h"
#include "ResponseLoader.h"
#include "PointClass.h"


namespace apl { namespace dnp {

/* DataPoll - base class */

DataPoll::DataPoll(Logger* apLogger, ITaskCompletion* apTaskCallback, IDataObserver* apObs, ITimeSource* apTimeSrc) : 
MasterTaskBase(apLogger, apTaskCallback),
mpObs(apObs),
mpTimeSrc(apTimeSrc)
{

}

TaskResult DataPoll::_OnPartialResponse(const APDU& f)
{
	this->ReadData(f);
	return TR_SUCCESS;
}

TaskResult DataPoll::_OnFinalResponse(const APDU& f)
{
	this->ReadData(f);
	return TR_SUCCESS;
}

void DataPoll::ReadData(const APDU& f)
{
	ResponseLoader loader(mpLogger, mpTimeSrc->GetTimeStampUTC(), mpObs);
	loader.Process(f.BeginRead());
}

/* Integrity Poll */

IntegrityPoll::IntegrityPoll(Logger* apLogger, ITaskCompletion* apTaskCallback, IDataObserver* apObs, ITimeSource* apTimeSrc) : 
DataPoll(apLogger, apTaskCallback, apObs, apTimeSrc)
{}

void IntegrityPoll::_ConfigureRequest(APDU& arAPDU)
{	
	arAPDU.Set(FC_READ);
	arAPDU.DoPlaceholderWrite(Group60Var1::Inst());
}

/* Exception Scan */

ExceptionPoll::ExceptionPoll(Logger* apLogger, ITaskCompletion* apTaskCallback, IDataObserver* apObs, ITimeSource* apTimeSrc, int aClassMask) : 
DataPoll(apLogger, apTaskCallback, apObs, apTimeSrc),
mClassMask(aClassMask)
{}

void ExceptionPoll::_ConfigureRequest(APDU& arAPDU)
{
	arAPDU.Set(FC_READ);
	if(mClassMask & PC_CLASS_1)	arAPDU.DoPlaceholderWrite(Group60Var2::Inst());
	if(mClassMask & PC_CLASS_2) arAPDU.DoPlaceholderWrite(Group60Var3::Inst());
	if(mClassMask & PC_CLASS_3)	arAPDU.DoPlaceholderWrite(Group60Var4::Inst());
}


}} //ens ns
