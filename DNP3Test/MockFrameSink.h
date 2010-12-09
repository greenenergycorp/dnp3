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
#ifndef __MOCK_FRAME_SINK_H_
#define __MOCK_FRAME_SINK_H_

#include <DNP3/ILinkContext.h>
#include <DNP3/LinkLayerConstants.h>
#include <APLTestTools/BufferTestObject.h>

namespace apl { namespace dnp {

class MockFrameSink : public ILinkContext, public BufferTestObject
{
	public:

	MockFrameSink();

	// ILinkContext members
	void OnLowerLayerUp();
	void OnLowerLayerDown();

	//	Sec to Pri
	void Ack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);
	void Nack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);
	void LinkStatus(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);
	void NotSupported (bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);

	//	Pri to Sec

	void TestLinkStatus(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc);
	void ResetLinkStates(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc);
	void RequestLinkStatus(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc);
	void ConfirmedUserData(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength);
	void UnconfirmedUserData(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength);

	void Reset();

	size_t mNumFrames;

	bool CheckLast(FuncCodes aCode, bool aIsMaster, uint_16_t aDest, uint_16_t aSrc);
	bool CheckLastWithFCB(FuncCodes aCode, bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc);
	bool CheckLastWithDFC(FuncCodes aCode, bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);

	// Last frame information
	FuncCodes mCode;
	bool mIsMaster;
	bool mIsRcvBuffFull;
	uint_16_t mSrc;
	uint_16_t mDest;
	bool mFcb;

	bool mLowerOnline;

	private:

	void Update(FuncCodes aCode, bool aIsMaster, uint_16_t aSrc, uint_16_t aDest);
};

}}

#endif
