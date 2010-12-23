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
#include "AsyncLinkLayerRouter.h"


#include <APL/Exception.h>
#include <APL/IPhysicalLayerAsync.h>
#include <APL/Logger.h>
#include <sstream>
#include <boost/foreach.hpp>

#include "ILinkContext.h"
#include "LinkFrame.h"

using namespace std;

namespace apl { namespace dnp {

AsyncLinkLayerRouter::AsyncLinkLayerRouter(apl::Logger* apLogger, IPhysicalLayerAsync* apPhys, ITimerSource* apTimerSrc, millis_t aOpenRetry) :
Loggable(apLogger),
AsyncPhysLayerMonitor(apLogger, apPhys, apTimerSrc, aOpenRetry),
mReceiver(apLogger, this),
mTransmitting(false)
{}

void AsyncLinkLayerRouter::AddContext(ILinkContext* apContext, uint_16_t aAddress)
{
	assert(apContext != NULL);

	if(mAddressMap.find(aAddress) != mAddressMap.end()) {
	  ostringstream oss;
	  oss << "Address already in use: " << aAddress;
      throw ArgumentException(LOCATION, oss.str());
	}

	BOOST_FOREACH(AddressMap::value_type v, mAddressMap) {
		if(apContext == v.second) {
			ostringstream oss;
			oss << "Context already in bound to address:  " << v.first;
			throw ArgumentException(LOCATION, oss.str());
		}
	}

	mAddressMap[aAddress] = apContext;
	if(this->IsOpen()) apContext->OnLowerLayerUp();
}

void AsyncLinkLayerRouter::RemoveContext(uint_16_t aAddress)
{
	AddressMap::iterator i = mAddressMap.find(aAddress);	
	if(i != mAddressMap.end()) { 
		ILinkContext* pContext = i->second;
		mAddressMap.erase(i);
		if(this->IsOpen()) pContext->OnLowerLayerDown();
	}
}


ILinkContext* AsyncLinkLayerRouter::GetContext(uint_16_t aDest)
{
	AddressMap::iterator i = mAddressMap.find(aDest);
	return (i == mAddressMap.end()) ? NULL : i->second;
}


ILinkContext* AsyncLinkLayerRouter::GetDestination(uint_16_t aDest)
{
	ILinkContext* pDest = GetContext(aDest);
	
	if(pDest == NULL) {
		ERROR_BLOCK(LEV_WARNING, "Frame for unknown destination: " << aDest, DLERR_UNKNOWN_DESTINATION);
	}
	
	return pDest;
}

////////////////////////////////////////////////////////////////////////////////
// IFrameSink Implementation
////////////////////////////////////////////////////////////////////////////////

void AsyncLinkLayerRouter::Ack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->Ack(aIsMaster, aIsRcvBuffFull, aDest, aSrc);
}
void AsyncLinkLayerRouter::Nack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->Nack(aIsMaster, aIsRcvBuffFull, aDest, aSrc);
}
void AsyncLinkLayerRouter::LinkStatus(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->LinkStatus(aIsMaster, aIsRcvBuffFull, aDest, aSrc);
}
void AsyncLinkLayerRouter::NotSupported (bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->NotSupported(aIsMaster, aIsRcvBuffFull, aDest, aSrc);
}
void AsyncLinkLayerRouter::TestLinkStatus(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->TestLinkStatus(aIsMaster, aFcb, aDest, aSrc);
}
void AsyncLinkLayerRouter::ResetLinkStates(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->ResetLinkStates(aIsMaster, aDest, aSrc);
}
void AsyncLinkLayerRouter::RequestLinkStatus(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->RequestLinkStatus(aIsMaster, aDest, aSrc);
}
void AsyncLinkLayerRouter::ConfirmedUserData(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->ConfirmedUserData(aIsMaster, aFcb, aDest, aSrc, apData, aDataLength);
}
void AsyncLinkLayerRouter::UnconfirmedUserData(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength)
{
	ILinkContext* pDest = GetDestination(aDest);
	if(pDest) pDest->UnconfirmedUserData(aIsMaster, aDest, aSrc, apData, aDataLength);
}

void AsyncLinkLayerRouter::_OnReceive(const apl::byte_t*, size_t aNumBytes)
{	
	// The order is important here. You must let the receiver process the byte or another read could write
	// over the buffer before it is processed
	mReceiver.OnRead(aNumBytes); //this may trigger callbacks to the local ILinkContext interface	
	if(mpPhys->CanRead()) { // this is required because the call above could trigger the layer to be closed
		mpPhys->AsyncRead(mReceiver.WriteBuff(), mReceiver.NumWriteBytes()); //start another read
	}
}

void AsyncLinkLayerRouter::Transmit(const LinkFrame& arFrame)
{	
	if(this->GetContext(arFrame.GetSrc())) {
		if(!this->IsLowerLayerUp()) 
			throw InvalidStateException(LOCATION, "LowerLayerDown");
		this->mTransmitQueue.push_back(arFrame);
		this->CheckForSend();
	}
	else {
		ostringstream oss;
		oss << "Unassociated context w/ address: " << arFrame.GetSrc();
		throw ArgumentException(LOCATION, oss.str());
	}	
}

void AsyncLinkLayerRouter::_OnSendSuccess()
{
	assert(mTransmitQueue.size() > 0);
	assert(mTransmitting);
	ILinkContext* pContext = this->GetContext(mTransmitQueue.front().GetSrc());
	assert(pContext != NULL);
	mTransmitting = false;	
	mTransmitQueue.pop_front();
	this->CheckForSend();	
}

void AsyncLinkLayerRouter::_OnSendFailure()
{	
	LOG_BLOCK(LEV_ERROR, "Unexpected _OnSendFailure");
	mTransmitting = false;
	this->CheckForSend();
}

void AsyncLinkLayerRouter::CheckForSend()
{
	if(mTransmitQueue.size() > 0 && !mTransmitting) {
		mTransmitting = true;
		const LinkFrame& f = mTransmitQueue.front();
		LOG_BLOCK(LEV_INTERPRET, "~> " << f.ToString());
		mpPhys->AsyncWrite(f.GetBuffer(), f.GetSize());
	}
}

void AsyncLinkLayerRouter::Up()
{
	mpPhys->AsyncRead(mReceiver.WriteBuff(), mReceiver.NumWriteBytes());
	BOOST_FOREACH(AddressMap::value_type p, mAddressMap) { p.second->OnLowerLayerUp(); }
}

void AsyncLinkLayerRouter::Down()
{
	mTransmitting = false;
	mTransmitQueue.erase(mTransmitQueue.begin(), mTransmitQueue.end());
	for(AddressMap::iterator i = mAddressMap.begin(); i != mAddressMap.end(); ++i) {		
		i->second->OnLowerLayerDown();
	}
	
}

}}
