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
#ifndef __ASYNC_TRANSPORT_LAYER_H_
#define __ASYNC_TRANSPORT_LAYER_H_



#include "TransportRx.h"
#include "TransportTx.h"

#include "DNPConstants.h"

#include <APL/AsyncLayerInterfaces.h>

namespace apl { namespace dnp {

	class TLS_Base;

	/** Implements the DNP3 transport layer as a generic
	asynchronous protocol stack layer
	*/
	class AsyncTransportLayer : public IUpperLayer, public ILowerLayer
	{
		public:

		AsyncTransportLayer(apl::Logger* apLogger, size_t aFragSize = DEFAULT_FRAG_SIZE);
		virtual ~AsyncTransportLayer(){}

		/* Actions - Taken by the states/transmitter/receiver in response to events */

		void ThisLayerUp();
		void ThisLayerDown();
		void ChangeState(TLS_Base* apNewState);

		void TransmitAPDU(const byte_t* apData, size_t aNumBytes);
		void TransmitTPDU(const byte_t* apData, size_t aNumBytes);
		void ReceiveAPDU(const byte_t* apData, size_t aNumBytes);
		void ReceiveTPDU(const byte_t* apData, size_t aNumBytes);

		bool ContinueSend(); /// return true if
		void SignalSendSuccess();
		void SignalSendFailure();

		/* Events - NVII delegates from ILayerUp/ILayerDown and Events produced internally */
		static std::string ToString(byte_t aHeader);

		private:

		//delegated to the states
		void _Send(const apl::byte_t*, size_t); //Implement ILowerLayer
		void _OnReceive(const apl::byte_t*, size_t); //Implement IUpperLayer
		void _OnLowerLayerUp();
		void _OnLowerLayerDown();
		void _OnSendSuccess();
		void _OnSendFailure();
		void _OnLowerLayerShutdown();

		/* Members and Helpers */
		TLS_Base* mpState;

		const size_t M_FRAG_SIZE;

		/* Transmitter and Receiver Classes */
		TransportRx mReceiver;
		TransportTx mTransmitter;

		bool mThisLayerUp;
	};

}}

#endif
