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
#include "AsyncResponseContext.h"

#include "Objects.h"
#include "DNPConstants.h"
#include "SlaveResponseTypes.h"
#include <APL/Logger.h>

#include <boost/bind.hpp>

using namespace boost;

#define MACRO_CONTINUOUS_CASE(obj,var) \
case(MACRO_DNP_RADIX(obj,var)): { \
	/*WriteFunc<datatype>::Type func = bind(DataToDNP::WriteGroup##obj##Var##var, _1, _2); \*/ \
	if(!this->IterateContiguous(iter, arAPDU)) return false; \
	break; \
}

namespace apl { namespace dnp {

AsyncResponseContext::AsyncResponseContext(Logger* apLogger, AsyncDatabase* apDB, SlaveResponseTypes* apRspTypes, size_t aMaxBinary, size_t aMaxAnalog, size_t aMaxCounter) :
Loggable(apLogger),
mBuffer(aMaxBinary, aMaxAnalog, aMaxCounter),
mMode(UNDEFINED),

mpDB(apDB),
mFIR(true),
mFIN(false),
mpRspTypes(apRspTypes)
{}

void AsyncResponseContext::Reset()
{
	mFIR = true;
	mMode = UNDEFINED;
	mTempIIN.Zero();
	
	this->mStaticBinaries.clear();
	this->mStaticAnalogs.clear();
	this->mStaticCounters.clear();
	this->mStaticControls.clear();
	this->mStaticSetpoints.clear();

	this->mBinaryEvents.clear();
	this->mAnalogEvents.clear();
	this->mCounterEvents.clear();

	mBuffer.Deselect();
}

void AsyncResponseContext::ClearWritten()
{
	mBuffer.ClearWritten();
}

void AsyncResponseContext::ClearAndReset()
{
	this->ClearWritten();
	this->Reset();
}

inline size_t GetEventCount(const HeaderInfo& arHeader) 
{
	switch(arHeader.GetQualifier()) {
		case QC_1B_CNT:
		case QC_2B_CNT:
			return arHeader.GetCount();
		default:
			return std::numeric_limits<size_t>::max();
	}
}

IINField AsyncResponseContext::Configure(const APDU& arRequest)
{
	this->Reset();
	mMode = SOLICITED;

	for(HeaderReadIterator hdr = arRequest.BeginRead(); !hdr.IsEnd(); ++hdr)
	{
		switch(MACRO_DNP_RADIX(hdr->GetGroup(), hdr->GetVariation()))
		{	
			// static objects, all variations		
			case(MACRO_DNP_RADIX(1,0)):
				this->AddIntegrity(mStaticBinaries, mpRspTypes->mpStaticBinary);
				break;
			case(MACRO_DNP_RADIX(10,0)):
				this->AddIntegrity(mStaticControls, mpRspTypes->mpStaticControlStatus);
				break;
			case(MACRO_DNP_RADIX(20,0)):
				this->AddIntegrity(mStaticCounters, mpRspTypes->mpStaticCounter);
				break;
			case(MACRO_DNP_RADIX(30,0)):
				this->AddIntegrity(mStaticAnalogs, mpRspTypes->mpStaticAnalog);
				break;
			case(MACRO_DNP_RADIX(40,0)):
				this->AddIntegrity(mStaticSetpoints, mpRspTypes->mpStaticSetpointStatus);
				break;
			
			// event objects
			case(MACRO_DNP_RADIX(2,0)):
				this->SelectEvents(PC_ALL_EVENTS, mpRspTypes->mpEventBinary, mBinaryEvents, GetEventCount(hdr.info()));				
				break;
			case(MACRO_DNP_RADIX(22,0)):
				this->SelectEvents(PC_ALL_EVENTS, mpRspTypes->mpEventCounter, mCounterEvents, GetEventCount(hdr.info()));				
				break;
			case(MACRO_DNP_RADIX(32,0)):
				this->SelectEvents(PC_ALL_EVENTS, mpRspTypes->mpEventAnalog, mAnalogEvents, GetEventCount(hdr.info()));
				break;

			//specific objects
			case(MACRO_DNP_RADIX(2,1)):
				this->SelectEvents(PC_ALL_EVENTS, Group2Var1::Inst(), mBinaryEvents, GetEventCount(hdr.info()));
				break;
			case(MACRO_DNP_RADIX(2,2)):
				this->SelectEvents(PC_ALL_EVENTS, Group2Var2::Inst(), mBinaryEvents, GetEventCount(hdr.info()));
				break;
			case(MACRO_DNP_RADIX(2,3)):
				this->SelectEvents(PC_ALL_EVENTS, Group2Var3::Inst(), mBinaryEvents, GetEventCount(hdr.info()));
				break;


			// Class Objects
			case(MACRO_DNP_RADIX(60,1)):
				this->AddIntegrityPoll();
				break;			
			case(MACRO_DNP_RADIX(60,2)):
				this->SelectEvents(PC_CLASS_1, GetEventCount(hdr.info()));								
				break;
			case(MACRO_DNP_RADIX(60,3)):
				this->SelectEvents(PC_CLASS_2, GetEventCount(hdr.info()));
				break;
			case(MACRO_DNP_RADIX(60,4)):			
				this->SelectEvents(PC_CLASS_3, GetEventCount(hdr.info()));
				break;
			default:
				LOG_BLOCK(LEV_WARNING, "READ for obj " << hdr->GetGroup() << " var " << hdr->GetVariation() << " not supported.");
				this->mTempIIN.SetFuncNotSupported(true);
				break;
		}
	}

	return mTempIIN;
}

void AsyncResponseContext::SelectEvents(PointClass aClass, size_t aNum)
{
	size_t remain = aNum;

	if(mBuffer.IsOverflow())
		mTempIIN.SetEventBufferOverflow(true);

	remain -= this->SelectEvents(aClass, mpRspTypes->mpEventBinary, mBinaryEvents, remain);
	remain -= this->SelectEvents(aClass, mpRspTypes->mpEventAnalog, mAnalogEvents, remain);
	remain -= this->SelectEvents(aClass, mpRspTypes->mpEventCounter, mCounterEvents, remain);
}

void AsyncResponseContext::LoadResponse(APDU& arAPDU)
{
	//delay the setting of FIR/FIN until we know if it will be multifragmented or not
	arAPDU.Set(FC_RESPONSE);

	bool events = false;

	bool wrote_all = this->LoadEventData(arAPDU, events);

	if(wrote_all) wrote_all = LoadStaticData(arAPDU);

	FinalizeResponse(arAPDU, events, wrote_all);
}

bool AsyncResponseContext::SelectUnsol(ClassMask m) 
{
	if(m.class1) this->SelectEvents(PC_CLASS_1);
	if(m.class2) this->SelectEvents(PC_CLASS_2);
	if(m.class3) this->SelectEvents(PC_CLASS_3);

	return mBuffer.NumSelected() > 0;
}

bool AsyncResponseContext::HasEvents(ClassMask m)
{
	if(m.class1 && mBuffer.HasClassData(PC_CLASS_1)) return true;
	if(m.class2 && mBuffer.HasClassData(PC_CLASS_2)) return true;
	if(m.class3 && mBuffer.HasClassData(PC_CLASS_3)) return true;

	return false;
}

bool AsyncResponseContext::LoadUnsol(APDU& arAPDU, const IINField& arIIN, ClassMask m)
{	
	this->SelectUnsol(m);

	arAPDU.Set(FC_UNSOLICITED_RESPONSE, true, true, true, true);
	bool events = false;
	this->LoadEventData(arAPDU, events);
	return events;
}

bool AsyncResponseContext::LoadStaticData(APDU& arAPDU)
{
	if(!this->LoadStaticBinaries(arAPDU)) return false;
	if(!this->LoadStaticCounters(arAPDU)) return false;
	if(!this->LoadStaticAnalogs(arAPDU)) return false;
	if(!this->LoadStaticControlStatii(arAPDU)) return false;
	if(!this->LoadStaticSetpointStatii(arAPDU)) return false;

	return true;
}

bool AsyncResponseContext::LoadEventData(APDU& arAPDU, bool& arEventsLoaded)
{
	if(!this->LoadEvents<Binary>(arAPDU, mBinaryEvents, arEventsLoaded)) return false;
	if(!this->LoadEvents<Analog>(arAPDU, mAnalogEvents, arEventsLoaded)) return false;
	if(!this->LoadEvents<Counter>(arAPDU, mCounterEvents, arEventsLoaded)) return false;

	return true;
}

bool AsyncResponseContext::IsEmpty()
{
	return this->IsStaticEmpty() && this->IsEventEmpty();
}

bool AsyncResponseContext::IsStaticEmpty()
{
	return this->mStaticBinaries.empty() && this->mStaticCounters.empty() &&
		   this->mStaticAnalogs.empty() && this->mStaticControls.empty() &&
		   this->mStaticSetpoints.empty();
}

bool AsyncResponseContext::IsEventEmpty()
{
	// are there unwritten events in the selection buffer?
	return mBuffer.NumSelected() == 0;
}

void AsyncResponseContext::FinalizeResponse(APDU& arAPDU, bool aHasEventData, bool aFIN)
{
	mFIN = aFIN;
	bool confirm = !aFIN || aHasEventData;
	arAPDU.SetControl(mFIR, mFIN, confirm);
	mFIR = false;
}

bool AsyncResponseContext::LoadStaticBinaries(APDU& arAPDU)
{
	while(!mStaticBinaries.empty())
	{
		IterRecord<BinaryInfo>& iter = this->mStaticBinaries.front();
		int grp = iter.pObject->GetGroup();
		int var = iter.pObject->GetVariation();

		switch(MACRO_DNP_RADIX(grp, var))
		{
			//special case for the bitfield
			/*case(MACRO_DNP_RADIX(1,1)):
			{
				WriteFunc<Binary>::Type func = bind(&Group1Var1::Write, Group1Var1::Inst(), _1, iter.first->mIndex, _3, _2);
				if(!this->IterateContiguous(iter, arAPDU, func)) return false;
				break;
			}*/

			MACRO_CONTINUOUS_CASE(1,2);
			
			default:
				break;
		}

		this->mStaticBinaries.pop_front();
	}

	return true;
}

bool AsyncResponseContext::LoadStaticAnalogs(APDU& arAPDU)
{
	while(!mStaticAnalogs.empty())
	{
		IterRecord<AnalogInfo>& iter = this->mStaticAnalogs.front();
		int grp = iter.pObject->GetGroup();
		int var = iter.pObject->GetVariation();

		switch(MACRO_DNP_RADIX(grp, var))
		{
			MACRO_CONTINUOUS_CASE(30,1);
			MACRO_CONTINUOUS_CASE(30,2);
			MACRO_CONTINUOUS_CASE(30,3);
			MACRO_CONTINUOUS_CASE(30,4);
			MACRO_CONTINUOUS_CASE(30,5);
			MACRO_CONTINUOUS_CASE(30,6);			
					
			default:
				break;
		}

		this->mStaticAnalogs.pop_front();
	}

	return true;	
}

bool AsyncResponseContext::LoadStaticCounters(APDU& arAPDU)
{
	while(!mStaticCounters.empty())
	{
		IterRecord<CounterInfo>& iter = this->mStaticCounters.front();
		int grp = iter.pObject->GetGroup();
		int var = iter.pObject->GetVariation();

		// Delta counters omitted
		switch(MACRO_DNP_RADIX(grp, var))
		{
			MACRO_CONTINUOUS_CASE(20,1);
			MACRO_CONTINUOUS_CASE(20,2);
			MACRO_CONTINUOUS_CASE(20,5);
			MACRO_CONTINUOUS_CASE(20,6);
			
			default:
				break;
		}
			
		this->mStaticCounters.pop_front();
	}

	return true;
}

bool AsyncResponseContext::LoadStaticControlStatii(APDU& arAPDU)
{
	while(!mStaticControls.empty())
	{
		IterRecord<ControlStatusInfo>& iter = this->mStaticControls.front();
		int grp = iter.pObject->GetGroup();
		int var = iter.pObject->GetVariation();

		switch(MACRO_DNP_RADIX(grp, var))
		{
			MACRO_CONTINUOUS_CASE(10,2);
			
			default:
				break;
		}
			
		this->mStaticControls.pop_front();
	}

	return true;
}

/*
bool AsyncResponseContext::WriteCTO(const TimeStamp_t& arTime, APDU& arAPDU)
{
	Group51Var1* pObj = Group51Var1::Inst();
	ObjectWriteIterator owi = arAPDU.WriteContiguous(pObj, 0, 0);
	if(owi.IsEnd()) return false;
	pObj->mTime.Set(*owi, arTime);
	return true;
}*/

bool AsyncResponseContext::LoadStaticSetpointStatii(APDU& arAPDU)
{
	while(!mStaticSetpoints.empty())
	{
		IterRecord<SetpointStatusInfo>& iter = this->mStaticSetpoints.front();
		int grp = iter.pObject->GetGroup();
		int var = iter.pObject->GetVariation();

		switch(MACRO_DNP_RADIX(grp, var))
		{
			MACRO_CONTINUOUS_CASE(40,1);
			MACRO_CONTINUOUS_CASE(40,2);
			MACRO_CONTINUOUS_CASE(40,3);
			MACRO_CONTINUOUS_CASE(40,4);
			
			default:
				break;
		}
			
		this->mStaticSetpoints.pop_front();
	}

	return true;
}

void AsyncResponseContext::AddIntegrityPoll()
{	
	this->AddIntegrity(mStaticBinaries, mpRspTypes->mpStaticBinary);
	this->AddIntegrity(mStaticAnalogs, mpRspTypes->mpStaticAnalog);
	this->AddIntegrity(mStaticCounters, mpRspTypes->mpStaticCounter);
	this->AddIntegrity(mStaticControls, mpRspTypes->mpStaticControlStatus);
	this->AddIntegrity(mStaticSetpoints, mpRspTypes->mpStaticSetpointStatus);
}

}}

