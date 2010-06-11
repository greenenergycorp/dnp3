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
#include "AsyncLinkLayer.h"


#include <assert.h>
#include <boost/bind.hpp>
#include <APL/Logger.h>
#include <APL/Exception.h>

#include "ILinkRouter.h"
#include "PriLinkLayerStates.h"
#include "SecLinkLayerStates.h"
#include "DNPConstants.h"

using namespace boost;

namespace apl { namespace dnp {

AsyncLinkLayer::AsyncLinkLayer(apl::Logger* apLogger, ITimerSource* apTimerSrc, const LinkConfig& arConfig) :
Loggable(apLogger),
ILowerLayer(apLogger),
mCONFIG(arConfig),
mRetryRemaining(0),
mpTimerSrc(apTimerSrc),
mpTimer(NULL),
mNextReadFCB(false),
mNextWriteFCB(false),
mIsOnline(false),
mpRouter(NULL),
mpPriState(PLLS_SecNotReset::Inst()),
mpSecState(SLLS_NotReset::Inst())
{}

void AsyncLinkLayer::SetRouter(ILinkRouter* apRouter)
{
	assert(mpRouter == NULL); assert(apRouter != NULL);
	mpRouter = apRouter;	
}

void AsyncLinkLayer::ChangeState(PriStateBase* apState) 
{ mpPriState = apState; }

void AsyncLinkLayer::ChangeState(SecStateBase* apState) 
{ mpSecState = apState; }

bool AsyncLinkLayer::Validate(bool aIsMaster, uint_16_t aSrc, uint_16_t aDest)
{
	if(!mIsOnline)
		throw InvalidStateException(LOCATION, "LowerLayerDown");

	if(aIsMaster == mCONFIG.IsMaster) {
		ERROR_BLOCK(LEV_WARNING, 
		(aIsMaster ? "Master frame received for master" : "Slave frame received for slave"),
		DLERR_MASTER_BIT_MATCH);
		return false;
	}

	if(aDest != mCONFIG.LocalAddr) {
		ERROR_BLOCK(LEV_WARNING, "Frame for unknown destintation", DLERR_UNKNOWN_DESTINATION);
		return false;
	}

	if(aSrc != mCONFIG.RemoteAddr) {
		ERROR_BLOCK(LEV_WARNING, "Frame from unknwon source", DLERR_UNKNOWN_SOURCE);
		return false;
	}

	return true;
}

////////////////////////////////////////////////
// ILinkContext
////////////////////////////////////////////////

void AsyncLinkLayer::OnLowerLayerUp()
{
	if(mIsOnline)
		throw InvalidStateException(LOCATION, "LowerLayerUp");
	mIsOnline = true;
	if(mpUpperLayer) mpUpperLayer->OnLowerLayerUp();
}

void AsyncLinkLayer::OnLowerLayerDown()
{
	if(!mIsOnline)
		throw InvalidStateException(LOCATION, "LowerLayerDown");

	if(mpTimer != NULL) this->CancelTimer();
	mIsOnline = false;	
	mpPriState = PLLS_SecNotReset::Inst();
	mpSecState = SLLS_NotReset::Inst();	
	
	if(mpUpperLayer) mpUpperLayer->OnLowerLayerDown();
}

void AsyncLinkLayer::Transmit(const LinkFrame& arFrame)
{
	mpRouter->Transmit(arFrame);
}

void AsyncLinkLayer::SendAck()
{
	mSecFrame.FormatAck(mCONFIG.IsMaster, false, mCONFIG.RemoteAddr, mCONFIG.LocalAddr);
	this->Transmit(mSecFrame);
}

void AsyncLinkLayer::SendLinkStatus()
{
	mSecFrame.FormatLinkStatus(mCONFIG.IsMaster, false, mCONFIG.RemoteAddr, mCONFIG.LocalAddr);
	this->Transmit(mSecFrame);
}

void AsyncLinkLayer::SendResetLinks()
{
	mPriFrame.FormatResetLinkStates(mCONFIG.IsMaster, mCONFIG.RemoteAddr, mCONFIG.LocalAddr);
	this->Transmit(mPriFrame);
}

void AsyncLinkLayer::SendUnconfirmedUserData(const byte_t* apData, size_t aLength)
{
	mPriFrame.FormatUnconfirmedUserData(mCONFIG.IsMaster, mCONFIG.RemoteAddr, mCONFIG.LocalAddr, apData, aLength);
	this->Transmit(mPriFrame);
	this->DoSendSuccess();
}

void AsyncLinkLayer::SendDelayedUserData(bool aFCB)
{
	mDelayedPriFrame.ChangeFCB(aFCB);
	this->Transmit(mDelayedPriFrame);
}

void AsyncLinkLayer::StartTimer()
{
	assert(mpTimer == NULL);
	mpTimer = this->mpTimerSrc->Start(mCONFIG.Timeout, bind(&AsyncLinkLayer::OnTimeout, this));
}

void AsyncLinkLayer::CancelTimer()
{
	assert(mpTimer);
	mpTimer->Cancel();
	mpTimer = NULL;
}

void AsyncLinkLayer::ResetRetry()
{
	this->mRetryRemaining = mCONFIG.NumRetry;
}

bool AsyncLinkLayer::Retry()
{
	if(mRetryRemaining > 0) {
		--mRetryRemaining;
		return true;
	}
	else return false;
}

////////////////////////////////////////////////
// IFrameSink
////////////////////////////////////////////////

void AsyncLinkLayer::Ack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{ 
	if(this->Validate(aIsMaster, aSrc, aDest)) 
		mpPriState->Ack(this, aIsRcvBuffFull);
}

void AsyncLinkLayer::Nack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{ 
	if(this->Validate(aIsMaster, aSrc, aDest))
		mpPriState->Nack(this, aIsRcvBuffFull);
}

void AsyncLinkLayer::LinkStatus(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	if(this->Validate(aIsMaster, aSrc, aDest))
		mpPriState->LinkStatus(this, aIsRcvBuffFull); 
}

void AsyncLinkLayer::NotSupported (bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	if(this->Validate(aIsMaster, aSrc, aDest))
		mpPriState->NotSupported(this, aIsRcvBuffFull);
}

void AsyncLinkLayer::TestLinkStatus(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc)
{
	if(this->Validate(aIsMaster, aSrc, aDest))
		mpSecState->TestLinkStatus(this, aFcb);
}

void AsyncLinkLayer::ResetLinkStates(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{ 
	if(this->Validate(aIsMaster, aSrc, aDest))
		mpSecState->ResetLinkStates(this);
}

void AsyncLinkLayer::RequestLinkStatus(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{
	if(this->Validate(aIsMaster, aSrc, aDest)) 
		mpSecState->RequestLinkStatus(this);
}

void AsyncLinkLayer::ConfirmedUserData(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength)
{
	if(this->Validate(aIsMaster, aSrc, aDest))
		mpSecState->ConfirmedUserData(this, aFcb, apData, aDataLength);
}

void AsyncLinkLayer::UnconfirmedUserData(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength)
{
	if(this->Validate(aIsMaster, aSrc, aDest))
		mpSecState->UnconfirmedUserData(this, apData, aDataLength);
}

////////////////////////////////////////////////
// ILowerLayer
////////////////////////////////////////////////

void AsyncLinkLayer::_Send(const apl::byte_t* apData, size_t aDataLength)
{
	if(!mIsOnline)
		throw InvalidStateException(LOCATION, "LowerLayerDown");
	if(mCONFIG.UseConfirms) mpPriState->SendConfirmed(this, apData, aDataLength);
	else mpPriState->SendUnconfirmed(this, apData, aDataLength); 
}

void AsyncLinkLayer::OnTimeout()
{
	assert(mpTimer);
	mpTimer = NULL;
	mpPriState->OnTimeout(this);
}

}}
