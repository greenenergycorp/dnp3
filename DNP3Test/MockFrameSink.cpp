// 
// Licensed to Green Energy Corp (www.greenenergycorp.com) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  Green Energy Corp licenses this file
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
#include "MockFrameSink.h"

namespace apl { namespace dnp {

MockFrameSink::MockFrameSink() : mNumFrames(0), mLowerOnline(false)
{}

void MockFrameSink::OnLowerLayerUp()
{
	mLowerOnline = true;
}

void MockFrameSink::OnLowerLayerDown()
{
	mLowerOnline = false;
}

void MockFrameSink::Reset()
{
	this->ClearBuffer();
	mNumFrames = 0;	
}

bool MockFrameSink::CheckLast(FuncCodes aCode, bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{
	return (mCode == aCode) && (aIsMaster == mIsMaster) && (mSrc == aSrc) && (mDest == aDest);
}

bool MockFrameSink::CheckLastWithFCB(FuncCodes aCode, bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc)
{
	return (mFcb == aFcb) && CheckLast(aCode, aIsMaster, aDest, aSrc);
}

bool MockFrameSink::CheckLastWithDFC(FuncCodes aCode, bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	return  (mIsRcvBuffFull == aIsRcvBuffFull) && CheckLast(aCode, aIsMaster, aDest, aSrc);
}

//	Sec to Pri
	
void MockFrameSink::Ack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	this->Update(FC_SEC_ACK, aIsMaster, aDest, aSrc);
	mIsRcvBuffFull = aIsRcvBuffFull;
}

void MockFrameSink::Nack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	this->Update(FC_SEC_NACK, aIsMaster, aDest, aSrc);
	mIsRcvBuffFull = aIsRcvBuffFull;
}

void MockFrameSink::LinkStatus(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	this->Update(FC_SEC_LINK_STATUS, aIsMaster, aDest, aSrc);
	mIsRcvBuffFull = aIsRcvBuffFull;
}

void MockFrameSink::NotSupported (bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	this->Update(FC_SEC_NOT_SUPPORTED, aIsMaster, aDest, aSrc);
	mIsRcvBuffFull = aIsRcvBuffFull;
}

//	Pri to Sec
	
void MockFrameSink::TestLinkStatus(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc)
{
	this->Update(FC_PRI_TEST_LINK_STATES, aIsMaster, aDest, aSrc);
	mFcb = aFcb;
}

void MockFrameSink::ResetLinkStates(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{
	this->Update(FC_PRI_RESET_LINK_STATES, aIsMaster, aDest, aSrc);
}

void MockFrameSink::RequestLinkStatus(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{
	this->Update(FC_PRI_REQUEST_LINK_STATUS, aIsMaster, aDest, aSrc);
}

void MockFrameSink::ConfirmedUserData(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength)
{
	this->Update(FC_PRI_CONFIRMED_USER_DATA, aIsMaster, aDest, aSrc);
	this->WriteToBuffer(apData, aDataLength);
	mFcb = aFcb;
}

void MockFrameSink::UnconfirmedUserData(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength)
{
	this->Update(FC_PRI_UNCONFIRMED_USER_DATA, aIsMaster, aDest, aSrc);
	this->WriteToBuffer(apData, aDataLength);
}

void MockFrameSink::Update(FuncCodes aCode, bool aIsMaster, uint_16_t aDest, uint_16_t aSrc)
{
	++mNumFrames;
	mCode = aCode;
	mIsMaster = aIsMaster;
	mDest = aDest;
	mSrc = aSrc;
}

}}

