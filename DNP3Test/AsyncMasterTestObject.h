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
#ifndef __ASYNC_MASTER_TEST_OBJECT_H_
#define __ASYNC_MASTER_TEST_OBJECT_H_

#include <APLTestTools/MockTimerSource.h>
#include <APL/AsyncTaskScheduler.h>
#include <APL/FlexibleDataObserver.h>
#include <APLTestTools/LogTester.h>
#include <DNP3/AsyncMaster.h>

#include "MockAsyncAppLayer.h"

namespace apl { namespace dnp {

struct MasterConfig;

class AsyncMasterTestObject : public LogTester
{
	public:
	AsyncMasterTestObject(MasterConfig, FilterLevel aLevel = LEV_INFO, bool aImmediate = false);

	void RespondToMaster(const std::string& arData, bool aFinal = true);
	std::string Read();
	std::string Peek();
	void Pop();

	MockTimeSource fake_time;
	MockTimerSource mts;
	AsyncTaskScheduler ats;
	FlexibleDataObserver fdo;
	MockAsyncAppLayer app;
	AsyncMaster master;
	APDU mAPDU;
};

}}

#endif
