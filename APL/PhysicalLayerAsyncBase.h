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
#ifndef __PHYSICAL_LAYER_ASYNC_BASE_H_
#define __PHYSICAL_LAYER_ASYNC_BASE_H_


#include <boost/system/error_code.hpp>
#include "IPhysicalLayerAsync.h"
#include "Loggable.h"

namespace apl {

	class PLAS_Base;

	/// This is the base class for the new async physical layers. It assumes that all of the functions
	/// are called from a single thread.
	class PhysicalLayerAsyncBase : public IPhysicalLayerAsync, public Loggable
	{
		struct State
		{
			State();

			bool mOpen;
			bool mOpening;
			bool mReading;
			bool mWriting;
			bool mClosing;

			bool CanOpen();
			bool CanClose();
			bool CanRead();
			bool CanWrite();

			bool CallbacksPending();
			bool CheckForClose();

			std::string ToString();
		};

		public:
			PhysicalLayerAsyncBase(Logger*);

			/// destructor should only be called once the object is totally finished with all of its async operations
			/// to avoid segfaulting. There are a # of asserts that make sure the object has been shutdown properly.
			virtual ~PhysicalLayerAsyncBase() {}

			bool IsReading() { return mState.mReading; }
			bool IsWriting() { return mState.mWriting; }
			bool IsClosing() { return mState.mClosing; }
			bool IsOpening() { return mState.mOpening; }
			bool IsOpen() { return mState.mOpen; }

			/* Implement IPhysicalLayerAsync - Events from the outside */
			void AsyncOpen();
			void AsyncClose();
			void AsyncWrite(const apl::byte_t*, size_t);
			void AsyncRead(apl::byte_t*, size_t);

			// Not an event delegated to the states
			void SetHandler(IHandlerAsync* apHandler);

			/* Actions taken by the states - These must be implemented by the concrete
			classes inherited from this class */
			virtual void DoOpen() = 0;
			virtual void DoClose() = 0;
			virtual void DoOpeningClose() { DoClose(); } //optionally override this action
			virtual void DoAsyncRead(byte_t*, size_t) = 0;
			virtual void DoAsyncWrite(const byte_t*, size_t) = 0;

			// These can be optionally overriden to do something more interesting, i.e. specific logging
			virtual void DoOpenSuccess() {}
			virtual void DoOpenFailure() {}

			void DoWriteSuccess();
			void DoThisLayerDown();
			void DoReadCallback(byte_t*, size_t);

			//Error reporting function(s)
			Logger* GetLogger() { return mpLogger; }

		protected:

			//Internally produced events
			void OnOpenCallback(const boost::system::error_code& arError);
			void OnReadCallback(const boost::system::error_code& arError, byte_t*, size_t aSize);
			void OnWriteCallback(const boost::system::error_code& arError, size_t aSize);

			/// "user" object that recieves the callbacks
			IHandlerAsync* mpHandler;

			/// State object that tracks the activities of the class, state pattern too heavy
			PhysicalLayerAsyncBase::State mState;

		private:

			void StartClose();
	};

	inline void PhysicalLayerAsyncBase::SetHandler(IHandlerAsync* apHandler)
	{ assert(mpHandler == NULL); assert(apHandler != NULL); this->mpHandler = apHandler; }

}
#endif
