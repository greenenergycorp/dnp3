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
#ifndef __ASYNC_INTEGRATION_TEST_H_
#define __ASYNC_INTEGRATION_TEST_H_

#include <vector>
#include <APL/FlexibleDataObserver.h>
#include <APLTestTools/AsyncTestObject.h>
#include <APLTestTools/LogTester.h>
#include <APLTestTools/MockCommandAcceptor.h>
#include <DNP3/AsyncStackManager.h>
#include <boost/foreach.hpp>
#include <APL/ChangeBuffer.h>
#include <APL/BoundNotifier.h>

#include <boost/random/mersenne_twister.hpp>
#include <memory>

namespace boost { namespace asio { class io_service; } }

namespace apl { namespace dnp {

class ObserverFanout : public IDataObserver
{
	public:

	void Add(IDataObserver* apObserver) { mObservers.push_back(apObserver); }

	void _Start() { mBuffer.Start(); }
	void _End()
	{
		mBuffer.End();

		BOOST_FOREACH(IDataObserver* p, mObservers) { mBuffer.FlushUpdates(p, false); }

		Transaction tr(&mBuffer); mBuffer.Clear();
	}

	void _Update(const Binary& arPoint, size_t aIndex) { mBuffer.Update(arPoint, aIndex); }
	void _Update(const Analog& arPoint, size_t aIndex) { mBuffer.Update(arPoint, aIndex); }
	void _Update(const Counter& arPoint, size_t aIndex) { mBuffer.Update(arPoint, aIndex); }
	void _Update(const ControlStatus& arPoint, size_t aIndex) { mBuffer.Update(arPoint, aIndex); }
	void _Update(const SetpointStatus& arPoint, size_t aIndex) { mBuffer.Update(arPoint, aIndex); }

	private:
	ChangeBuffer<NullLock> mBuffer;
	std::vector<IDataObserver*> mObservers;

};

class AsyncIntegrationTest : public AsyncTestObject, public AsyncStackManager
{
	public:

		AsyncIntegrationTest(Logger* apLogger, FilterLevel aLevel, uint_16_t aStartPort, size_t aNumPairs, size_t aNumPoints);
		virtual ~AsyncIntegrationTest();

		IDataObserver* GetFanout() { return &mFanout; }

		bool SameData();

		Binary RandomBinary();
		Analog RandomAnalog();
		Counter RandomCounter();

	private:

		void RegisterChange() { mChange = true; }
		void AddStackPair(FilterLevel aLevel, size_t aNumPoints);
		void Next();

		ObserverFanout mFanout;
		const uint_16_t M_START_PORT;
		Logger* mpLogger;

		bool mChange;
		BoundNotifier mNotifier;
		std::vector<FlexibleDataObserver*> mMasterObservers;
		FlexibleDataObserver mLocalFDO;
		MockCommandAcceptor mCmdAcceptor;
		boost::mt19937 rng; //random number generator
};

}}

#endif

