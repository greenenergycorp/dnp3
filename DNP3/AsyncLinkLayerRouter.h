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
#ifndef __ASYNC_LINK_LAYER_ROUTER_H_
#define __ASYNC_LINK_LAYER_ROUTER_H_


#include <map>
#include <queue>

#include <APL/AsyncPhysLayerMonitor.h>

#include "LinkLayerReceiver.h"
#include "IFrameSink.h"
#include "ILinkRouter.h"

namespace apl {
  class IPhysicalLayerAsync;
}

namespace apl { namespace dnp {

	class ILinkContext;
	class LinkFrame;

	///	Implements the parsing and de-multiplexing portion of
	///	of DNP 3 Data Link Layer. AsyncPhysLayerMonitor inherits
	/// from IHandlerAsync, which inherits from IUpperLayer
	class AsyncLinkLayerRouter : public AsyncPhysLayerMonitor, public IFrameSink, public ILinkRouter
	{
		public:

		AsyncLinkLayerRouter(apl::Logger*, IPhysicalLayerAsync*, ITimerSource*, millis_t aOpenRetry);

		/// Ties the lower part of the link layer to the upper part
		void AddContext(ILinkContext*, uint_16_t aAddress);

		/// This is safe to do at runtime, so long as the request happens from the io_service thread.
		void RemoveContext(uint_16_t aAddress);

		// Implement the IFrameSink interface - This is how the receiver pushes data
		void Ack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);
		void Nack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);
		void LinkStatus(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);
		void NotSupported (bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc);
		void TestLinkStatus(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc);
		void ResetLinkStates(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc);
		void RequestLinkStatus(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc);
		void ConfirmedUserData(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength);
		void UnconfirmedUserData(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength);

		// ILinkRouter interface
		void Transmit(const LinkFrame&);

		size_t NumContext() { return mAddressMap.size(); }

		private:

		ILinkContext* GetDestination(uint_16_t aDest);
		ILinkContext* GetContext(uint_16_t aDest);

		void CheckForSend();


		typedef std::map<uint_16_t, ILinkContext*> AddressMap;
		typedef std::deque<LinkFrame> TransmitQueue;

		AddressMap mAddressMap;
		TransmitQueue mTransmitQueue;

		/// Handles the parsing of incoming frames
		LinkLayerReceiver mReceiver;
		bool mTransmitting;

		/* Events - NVII delegates from IUpperLayer */

		/// Called when the physical layer has read data into to the requested buffer
		void _OnReceive(const apl::byte_t*, size_t);
		void _OnSendSuccess();
		void _OnSendFailure();

		/// Implement virtual AsyncPhysLayerMonitor
		void Up();
		void Down();

		std::string RecvString() { return "<~"; }
	};

}}

#endif
