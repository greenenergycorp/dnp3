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
#include <APL/ASIOIncludes.h>
#include "AsyncStartupTeardownTest.h"

#include <DNP3/AsyncMasterStack.h>
#include <APL/PhysicalLayerAsyncTCPClient.h>

#include <boost/foreach.hpp>

namespace apl { namespace dnp {

AsyncStartupTeardownTest::AsyncStartupTeardownTest(FilterLevel aLevel, bool aAutoStart) :
mLog(),
mMgr(mLog.GetLogger(aLevel, "mgr"), aAutoStart)
{

}

void AsyncStartupTeardownTest::CreatePort(const std::string& arName, FilterLevel aLevel)
{
	std::string name = arName + " router";
	PhysLayerSettings s(aLevel, 1000);
	mMgr.AddTCPClient(arName, s, "127.0.0.1", 30000);
}

void AsyncStartupTeardownTest::AddMaster(const std::string& arStackName, const std::string& arPortName, uint_16_t aLocalAddress, FilterLevel aLevel)
{	
	MasterStackConfig cfg;
	cfg.link.LocalAddr = aLocalAddress;
	mMgr.AddMaster(arPortName, arStackName, aLevel, &mFDO, cfg);
}



}}



