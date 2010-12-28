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

#ifndef __ADDRESS_SCANNER_H_
#define __ADDRESS_SCANNER_H_

#include <APLXML/PhysicalLayerManagerXML.h>
#include <APL/IOServiceThread.h>
#include <APL/TimerSourceASIO.h>
#include <APL/Loggable.h>

#include <DNP3/AsyncLinkLayerRouter.h>
#include <DNP3/ILinkContext.h>
#include <DNP3/LinkFrame.h>

namespace APLXML_MTS { class MasterTestSet_t; }
namespace apl { class Logger; }
namespace boost { namespace asio {
	class io_service;
}}

namespace apl { namespace dnp {

class AddressScanner : private Loggable, public ILinkContext
{
	public:
		AddressScanner(Logger* apLogger, const APLXML_MTS::MasterTestSet_t& cfg, uint_16_t start, uint_16_t stop);

		void Run();

		void OnLowerLayerUp();
		void OnLowerLayerDown();

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

	private:

		void OnTimeout();

		void NextFrame();
		
		apl::xml::PhysicalLayerManagerXML manager;		
		std::auto_ptr<boost::asio::io_service> mpService;
		TimerSourceASIO mTimerSrc;
		IOServiceThread mThread;
		dnp::AsyncLinkLayerRouter mRouter;
		LinkFrame mFrame;
		ITimer* mpTimer;
		uint_16_t mMasterAddr;
		millis_t mScanTimeout;

		uint_16_t mCurrent;
		uint_16_t mStop;
};

}}

#endif
