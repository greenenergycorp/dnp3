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
#ifndef __PRI_LINK_LAYER_STATES_H_
#define __PRI_LINK_LAYER_STATES_H_

#include <APL/Types.h>
#include <APL/Singleton.h>
#include <string>

#include "AsyncLinkLayer.h"

namespace apl { namespace dnp {

	class AsyncLinkLayer;

	class PriStateBase
	{
		public:

		/* Incoming messages for primary station */
		virtual void Ack(AsyncLinkLayer*, bool aIsRcvBuffFull);
		virtual void Nack(AsyncLinkLayer*, bool aIsRcvBuffFull);
		virtual void LinkStatus(AsyncLinkLayer*, bool aIsRcvBuffFull);
		virtual void NotSupported (AsyncLinkLayer*, bool aIsRcvBuffFull);

		virtual void OnTimeout(AsyncLinkLayer*);

		/*Upper layer events to handle */
		virtual void SendConfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t);
		virtual void SendUnconfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t);

		//every concrete state implements this for logging purposes
		virtual std::string Name() const = 0;
	};

	///	@section desc Entry state for primary station
	class PLLS_SecNotReset : public PriStateBase
	{
		MACRO_STATE_SINGLETON_INSTANCE(PLLS_SecNotReset);
		void SendUnconfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t);
		void SendConfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t);
	};

	///	@section desc for reset state
	class PLLS_SecReset : public PriStateBase
	{
		MACRO_STATE_SINGLETON_INSTANCE(PLLS_SecReset);
		void SendUnconfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t);
		void SendConfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t);
	};

	///	@section desc As soon as we get an ACK, send the delayed pri frame
	class PLLS_ResetLinkWait : public PriStateBase
	{
		MACRO_STATE_SINGLETON_INSTANCE(PLLS_ResetLinkWait);

		void Ack(AsyncLinkLayer*, bool aIsRcvBuffFull);
		void Nack(AsyncLinkLayer*  apLL, bool) { Failure(apLL); }
		void LinkStatus(AsyncLinkLayer* apLL, bool) { Failure(apLL); }
		void NotSupported (AsyncLinkLayer*  apLL, bool) { Failure(apLL); }

		void OnTimeout(AsyncLinkLayer*);

		private:
		void Failure(AsyncLinkLayer*);
	};

	///	@section desc As soon as we get an ACK, send the delayed pri frame
	class PLLS_ConfDataWait : public PriStateBase
	{
		MACRO_STATE_SINGLETON_INSTANCE(PLLS_ConfDataWait);

		void Ack(AsyncLinkLayer*, bool aIsRcvBuffFull);
		void Nack(AsyncLinkLayer* apLL, bool) { Failure(apLL); }
		void LinkStatus(AsyncLinkLayer* apLL, bool) { Failure(apLL); }
		void NotSupported (AsyncLinkLayer* apLL, bool) { Failure(apLL); }
		void OnTimeout(AsyncLinkLayer*);

		private:
		void Failure(AsyncLinkLayer*);
	};



}} //end namespace


#endif
