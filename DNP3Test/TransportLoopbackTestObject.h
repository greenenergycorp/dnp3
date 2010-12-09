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
#ifndef __TRANSPORT_LOOPBACK_TEST_OBJECT_H_
#define __TRANSPORT_LOOPBACK_TEST_OBJECT_H_

#include <APLTestTools/AsyncTestObjectASIO.h>

#include <APL/TimerSourceASIO.h>
#include <APLTestTools/LogTester.h>
#include <APLTestTools/MockUpperLayer.h>

#include <DNP3/AsyncLinkLayerRouter.h>
#include <DNP3/AsyncLinkLayer.h>
#include <DNP3/AsyncTransportLayer.h>

namespace apl { namespace dnp {

class TransportLoopbackTestObject : public LogTester, public AsyncTestObjectASIO
{
	public:
		TransportLoopbackTestObject(
			boost::asio::io_service*,
			IPhysicalLayerAsync*,
			LinkConfig,
			LinkConfig,
			FilterLevel aLevel = LEV_INFO,
			bool aImmediate = false);

		~TransportLoopbackTestObject();

		Logger* GetLogger() { return mpLogger; }

		bool LayersUp();

		void Start();


	private:
		Logger* mpLogger;
		TimerSourceASIO mTimerSource;


		LinkConfig mCfgA;
		LinkConfig mCfgB;

		AsyncLinkLayer mLinkA;
		AsyncLinkLayer mLinkB;
		AsyncTransportLayer mTransA;
		AsyncTransportLayer mTransB;
		AsyncLinkLayerRouter mRouter;

	public:
		MockUpperLayer mUpperA;
		MockUpperLayer mUpperB;

};

}}

#endif
