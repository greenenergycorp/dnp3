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
#include "AsyncSlave.h"


#include "AsyncSlaveStates.h"
#include "AsyncDatabase.h"
#include "DNPExceptions.h"
#include "ObjectReadIterator.h"

#include <APL/Logger.h>
#include <APL/TimingTools.h>
#include <APL/AsyncTaskGroup.h>
#include <APL/AsyncTaskBase.h>

#include <boost/bind.hpp>

namespace apl { namespace dnp {

AsyncSlave::AsyncSlave(
					   Logger* apLogger, 
					   IAsyncAppLayer* apAppLayer,
					   ITimerSource* apTimerSrc,
					   ITimeManager* apTime,
					   AsyncDatabase* apDatabase,
					   IDNPCommandMaster* apCmdMaster,
					   const SlaveConfig& arCfg
					   ) :
Loggable(apLogger),
mpAppLayer(apAppLayer),
mpTimerSrc(apTimerSrc),
mpDatabase(apDatabase),
mpCmdMaster(apCmdMaster),
mpState(AS_Closed::Inst()),
mConfig(arCfg),
mRspTypes(arCfg),
mpUnsolTimer(NULL),
mResponse(arCfg.mMaxFragSize),
mUnsol(arCfg.mMaxFragSize),
mRspContext(apLogger, apDatabase, &mRspTypes, arCfg.mMaxBinaryEvents, arCfg.mMaxAnalogEvents, arCfg.mMaxCounterEvents),
mHaveLastRequest(false),
mLastRequest(arCfg.mMaxFragSize),
mpTime(apTime),
mDeferredUpdate(false),
mDeferredRequest(false),
mDeferredUnsol(false),
mDeferredUnknown(false),
mStartupNullUnsol(false),
mpTimeTimer(NULL)
{
	// link the event buffer to the database
	mpDatabase->SetEventBuffer(mRspContext.GetBuffer());

	mIIN.SetDeviceRestart(true); // always set on restart

	// use the cmd master to send and rsp queue to wait for reply
	mpCmdMaster->SetResponseObserver(&mRspQueue);

	// Incoming data will trigger a POST on the timer source to call OnDataUpdate
	mChangeBuffer.AddObserver(mNotifierSource.Get(boost::bind(&AsyncSlave::OnDataUpdate, this), mpTimerSrc));

	// this will cause the slave to go through the null-unsol startup sequence
	if(!mConfig.mDisableUnsol) mDeferredUnsol = true;
}

/* Implement IAsyncAppUser - external callbacks from the app layer */
	
void AsyncSlave::OnLowerLayerUp()
{
	mpState->OnLowerLayerUp(this);
	this->FlushDeferredEvents();	// all of the events check for Deferred events
}

void AsyncSlave::OnLowerLayerDown()
{
	mpState->OnLowerLayerDown(this);
	this->FlushDeferredEvents();
}

void AsyncSlave::OnSolSendSuccess()
{
	mpState->OnSolSendSuccess(this);
	this->FlushDeferredEvents();
}

void AsyncSlave::OnSolFailure()
{
	mpState->OnSolFailure(this);
	this->FlushDeferredEvents();
	LOG_BLOCK(LEV_WARNING, "Response failure");
}

void AsyncSlave::OnUnsolSendSuccess()
{
	mpState->OnUnsolSendSuccess(this);
	this->FlushDeferredEvents();
}

void AsyncSlave::OnUnsolFailure()
{
	mpState->OnUnsolFailure(this);
	LOG_BLOCK(LEV_WARNING, "Unsol response failure");
	this->FlushDeferredEvents();
}

void AsyncSlave::OnRequest(const APDU& arAPDU, SequenceInfo aSeqInfo)
{
	mpState->OnRequest(this, arAPDU, aSeqInfo);
	this->FlushDeferredEvents();
}

void AsyncSlave::OnUnknownObject()
{
	mpState->OnUnknown(this);
	this->FlushDeferredEvents();
}

/* Internally generated events */

void AsyncSlave::OnDataUpdate()
{
	// let the current state decide how to handle the change buffer
	mpState->OnDataUpdate(this);
	this->FlushDeferredEvents();
}

void AsyncSlave::OnUnsolTimerExpiration()
{
	// let the current state decide how to handle the timer expiration
	mpUnsolTimer = NULL;
	mpState->OnUnsolExpiration(this);
	this->FlushDeferredEvents();
}

/* Private functions */

void AsyncSlave::FlushDeferredEvents()
{
	// if a data update events was previously Deferred
	// this action might cause the state to change
	// for an unsol response
	if(mpState->AcceptsDeferredUpdates() && mDeferredUpdate) {
		mDeferredUpdate = false;
		mpState->OnDataUpdate(this);
	}
	
	// if a request APDU was previously Deferred by a state,
	// this action might cause a response and subsequent state
	// change
	if(mpState->AcceptsDeferredRequests() && mDeferredRequest) {
		mDeferredRequest = false;
		mpState->OnRequest(this, mRequest, mSeqInfo);
	}

	// if an unsol timer expiration was Deferred by a state,
	// this action might cause an unsolicted response to be 
	// generated
	if(mpState->AcceptsDeferredUnsolExpiration() && mDeferredUnsol) {
		mDeferredUnsol = false;
		mpState->OnUnsolExpiration(this);
	}

	if(mpState->AcceptsDeferredUnknown() && mDeferredUnknown) {
		mDeferredUnknown = false;
		mpState->OnUnknown(this);
	}
}

size_t AsyncSlave::FlushUpdates()
{
	size_t num = 0;
	try 
	{
		num = mChangeBuffer.FlushUpdates(mpDatabase);
	}
	catch ( Exception& ex ) 
	{
		LOG_BLOCK(LEV_ERROR, "Error in flush updates: " << ex.Message());
		Transaction tr(mChangeBuffer);
		mChangeBuffer.Clear();
		return 0;
	}

	LOG_BLOCK(LEV_INFO, "Processed " << num << " updates");
	return num;
}


void AsyncSlave::ConfigureAndSendSimpleResponse()
{
	mResponse.Set(FC_RESPONSE);
	mRspIIN.BitwiseOR(mIIN);
	mResponse.SetIIN(mRspIIN);
	mpAppLayer->SendResponse(mResponse);
}

void AsyncSlave::Send(APDU& arAPDU, const IINField& arIIN)
{
	mRspIIN.BitwiseOR(mIIN);
	mRspIIN.BitwiseOR(arIIN);
	arAPDU.SetIIN(mRspIIN);
	mpAppLayer->SendResponse(arAPDU);
}

void AsyncSlave::Send(APDU& arAPDU)
{
	mRspIIN.BitwiseOR(mIIN);	
	arAPDU.SetIIN(mRspIIN);
	mpAppLayer->SendResponse(arAPDU);
}

void AsyncSlave::SendUnsolicited(APDU& arAPDU)
{
	mRspIIN.BitwiseOR(mIIN);
	arAPDU.SetIIN(mRspIIN);
	mpAppLayer->SendUnsolicited(arAPDU);
}

void AsyncSlave::ConfigureDelayMeasurement(const APDU& arRequest)
{
	HeaderReadIterator hdr = arRequest.BeginRead();
	if(hdr.Count() > 0) mRspIIN.SetFuncNotSupported(true);

	Group52Var2* pObj = Group52Var2::Inst();

	mResponse.Set(FC_RESPONSE);
	
	IndexedWriteIterator i = mResponse.WriteIndexed(pObj, 1, QC_1B_CNT);
	i.SetIndex(0);
	pObj->mTime.Set(*i, 0);	
}

void AsyncSlave::HandleWriteIIN(HeaderReadIterator& arHdr)
{
	for(ObjectReadIterator obj = arHdr.BeginRead(); !obj.IsEnd(); ++obj)
	{
		switch(obj->Index())
		{
			case(IINI_DEVICE_RESTART):
			{
				bool value = Group80Var1::Inst()->Read(*obj, obj->Start(), obj->Index());
				if(!value) mIIN.SetDeviceRestart(false);
				else
				{
					mRspIIN.SetParameterError(true);
					ERROR_BLOCK(LEV_WARNING, "", SERR_INVALID_IIN_WRITE);
				}
				break;
			}
			default:
				mRspIIN.SetParameterError(true);
				ERROR_BLOCK(LEV_WARNING, "", SERR_INVALID_IIN_WRITE);
				break;
		}
	}
}

void AsyncSlave::HandleWriteTimeDate(HeaderReadIterator& arHWI)
{
	if(!mIIN.GetNeedTime())
	{
		LOG_BLOCK(LEV_WARNING, "Master is attempting to write time but slave is not requesting time sync");
		return;
	}

	ObjectReadIterator obj = arHWI.BeginRead();

	if(obj.Count() != 1) {
		mRspIIN.SetParameterError(true);
		return;
	}
	
	millis_t val = Group50Var1::Inst()->mTime.Get(*obj);
	mpTime->SetTime(val);

	mIIN.SetNeedTime(false);
}

void AsyncSlave::HandleWrite(const APDU& arRequest)
{
	for(HeaderReadIterator hdr = arRequest.BeginRead(); !hdr.IsEnd(); ++hdr)
	{
		switch(MACRO_DNP_RADIX(hdr->GetGroup(), hdr->GetVariation()))
		{
			case(MACRO_DNP_RADIX(80,1)):
				this->HandleWriteIIN(hdr);
				break;
			case(MACRO_DNP_RADIX(50,1)):
				this->HandleWriteTimeDate(hdr);
				break;
			default:
				mRspIIN.SetFuncNotSupported(true);
				ERROR_BLOCK(LEV_WARNING, "Object/Function mismatch", SERR_OBJ_FUNC_MISMATCH);
				break;				
		}
	}
}

void AsyncSlave::HandleSelect(const APDU& arRequest, SequenceInfo aSeqInfo)
{
	mpCmdMaster->DeselectAll();

	mResponse.Set(FC_RESPONSE);

	for(HeaderReadIterator hdr = arRequest.BeginRead(); !hdr.IsEnd(); ++hdr) {

		ObjectReadIterator i = hdr.BeginRead();

		switch(MACRO_DNP_RADIX(hdr->GetGroup(), hdr->GetVariation())) {

			case(MACRO_DNP_RADIX(12,1)):
				this->RespondToCommands<BinaryOutput>(Group12Var1::Inst(), i, boost::bind(&AsyncSlave::Select<BinaryOutput>, this, _1, _2, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
			
			case(MACRO_DNP_RADIX(41,1)):
				this->RespondToCommands<Setpoint>(Group41Var1::Inst(), i, boost::bind(&AsyncSlave::Select<Setpoint>, this, _1, _2, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
				
			case(MACRO_DNP_RADIX(41,2)):
				this->RespondToCommands<Setpoint>(Group41Var2::Inst(), i, boost::bind(&AsyncSlave::Select<Setpoint>, this, _1, _2, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;

			case(MACRO_DNP_RADIX(41,3)):
				this->RespondToCommands<Setpoint>(Group41Var3::Inst(), i, boost::bind(&AsyncSlave::Select<Setpoint>, this, _1, _2, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
				
			case(MACRO_DNP_RADIX(41,4)):
				this->RespondToCommands<Setpoint>(Group41Var4::Inst(), i, boost::bind(&AsyncSlave::Select<Setpoint>, this, _1, _2, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;

			default:
				mRspIIN.SetFuncNotSupported(true);
				ERROR_BLOCK(LEV_WARNING, "Object/Function mismatch", SERR_OBJ_FUNC_MISMATCH);
				break;
		}
	}
}

void AsyncSlave::HandleOperate(const APDU& arRequest, SequenceInfo aSeqInfo)
{
	if ( aSeqInfo == SI_PREV && mLastRequest == arRequest )
		return;

	mResponse.Set(FC_RESPONSE);

	for(HeaderReadIterator hdr = arRequest.BeginRead(); !hdr.IsEnd(); ++hdr) {

		ObjectReadIterator i = hdr.BeginRead();

		switch(MACRO_DNP_RADIX(hdr->GetGroup(), hdr->GetVariation())) {

			case(MACRO_DNP_RADIX(12,1)):	
				this->RespondToCommands<BinaryOutput>(Group12Var1::Inst(), i, boost::bind(&AsyncSlave::Operate<BinaryOutput>, this, _1, _2, false, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
			
			case(MACRO_DNP_RADIX(41,1)):
				this->RespondToCommands<Setpoint>(Group41Var1::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, false, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
				
			case(MACRO_DNP_RADIX(41,2)):
				this->RespondToCommands<Setpoint>(Group41Var2::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, false, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;

			case(MACRO_DNP_RADIX(41,3)):
				this->RespondToCommands<Setpoint>(Group41Var3::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, false, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
				
			case(MACRO_DNP_RADIX(41,4)):
				this->RespondToCommands<Setpoint>(Group41Var4::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, false, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;

			default:
				mRspIIN.SetFuncNotSupported(true);
				ERROR_BLOCK(LEV_WARNING, "Object/Function mismatch", SERR_OBJ_FUNC_MISMATCH);
				break;
		}
	}
}

void AsyncSlave::HandleDirectOperate(const APDU& arRequest, SequenceInfo aSeqInfo)
{
	mResponse.Set(FC_RESPONSE);

	for(HeaderReadIterator hdr = arRequest.BeginRead(); !hdr.IsEnd(); ++hdr) {

		ObjectReadIterator i = hdr.BeginRead();

		switch(MACRO_DNP_RADIX(hdr->GetGroup(), hdr->GetVariation())) {

			case(MACRO_DNP_RADIX(12,1)):	
				this->RespondToCommands<BinaryOutput>(Group12Var1::Inst(), i, boost::bind(&AsyncSlave::Operate<BinaryOutput>, this, _1, _2, true, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
			
			case(MACRO_DNP_RADIX(41,1)):
				this->RespondToCommands<Setpoint>(Group41Var1::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, true, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
				
			case(MACRO_DNP_RADIX(41,2)):
				this->RespondToCommands<Setpoint>(Group41Var2::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, true, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;

			case(MACRO_DNP_RADIX(41,3)):
				this->RespondToCommands<Setpoint>(Group41Var3::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, true, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;
				
			case(MACRO_DNP_RADIX(41,4)):
				this->RespondToCommands<Setpoint>(Group41Var4::Inst(), i, boost::bind(&AsyncSlave::Operate<Setpoint>, this, _1, _2, true, hdr.info(), aSeqInfo, arRequest.GetControl().SEQ));
				break;

			default:
				mRspIIN.SetFuncNotSupported(true);
				ERROR_BLOCK(LEV_WARNING, "Object/Function mismatch", SERR_OBJ_FUNC_MISMATCH);
				break;
		}
	}
}

void AsyncSlave::HandleEnableUnsolicited(const APDU& arRequest, bool aIsEnable)
{
	mResponse.Set(FC_RESPONSE);

	if(mConfig.mDisableUnsol) {
		mRspIIN.SetFuncNotSupported(true);
	}
	else {

		if(aIsEnable) this->mDeferredUnsol = true;

		for(HeaderReadIterator hdr = arRequest.BeginRead(); !hdr.IsEnd(); ++hdr) {

			switch(MACRO_DNP_RADIX(hdr->GetGroup(), hdr->GetVariation())) {

				case(MACRO_DNP_RADIX(60,2)):	
					mConfig.mUnsolMask.class1 = aIsEnable;
					break;

				case(MACRO_DNP_RADIX(60,3)):
					mConfig.mUnsolMask.class2 = aIsEnable;
					break;
					
				case(MACRO_DNP_RADIX(60,4)):
					mConfig.mUnsolMask.class3 = aIsEnable;
					break;

				default:
					mRspIIN.SetFuncNotSupported(true);
					LOG_BLOCK(LEV_WARNING, "Cannot enable/disable unsol for " << hdr->GetBaseObject()->Name());
					break;
			}
		}
	}	
}

void AsyncSlave::HandleUnknown()
{
	mResponse.Set(FC_RESPONSE);
	mRspIIN.SetObjectUnknown(true);
}

void AsyncSlave::StartUnsolTimer(millis_t aTimeout)
{
	assert(mpUnsolTimer == NULL);
	mpUnsolTimer = mpTimerSrc->Start(aTimeout, boost::bind(&AsyncSlave::OnUnsolTimerExpiration, this));
}

void AsyncSlave::ResetTimeIIN()
{
	mpTimeTimer = NULL;
	mIIN.SetNeedTime(true);
	mpTimeTimer = mpTimerSrc->Start(mConfig.mTimeSyncPeriod, boost::bind(&AsyncSlave::ResetTimeIIN, this));
}

}} //end ns

