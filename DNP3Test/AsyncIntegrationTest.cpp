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
#include "AsyncIntegrationTest.h"

#include <sstream>

#include <APL/PhysicalLayerFactory.h>
#include <APL/IPhysicalLayerAsync.h>

#include <DNP3/MasterStackConfig.h>
#include <DNP3/SlaveStackConfig.h>

#include <DNP3/AsyncMasterStack.h>
#include <DNP3/AsyncSlaveStack.h>

#include <boost/foreach.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/bind.hpp>

using namespace std;

namespace apl { namespace dnp {

AsyncIntegrationTest::AsyncIntegrationTest(Logger* apLogger, FilterLevel aLevel, uint_16_t aStartPort, size_t aNumPairs, size_t aNumPoints) :
AsyncStackManager(apLogger),
M_START_PORT(aStartPort),
mChange(false),
mNotifier(boost::bind(&AsyncIntegrationTest::RegisterChange, this))
{
	for(size_t i=0; i<aNumPairs; ++i) AddStackPair(aLevel, aNumPoints);
	mFanout.Add(&mLocalFDO);
}

AsyncIntegrationTest::~AsyncIntegrationTest()
{
	BOOST_FOREACH(FlexibleDataObserver* pFDO, mMasterObservers) { delete pFDO; }
}

void AsyncIntegrationTest::Next() { AsyncTestObject::Next(this->mService.Get(), 10); }

bool AsyncIntegrationTest::SameData()
{
	if(!mChange) return false;
	
	mChange = false;

	BOOST_FOREACH(FlexibleDataObserver* pObs, mMasterObservers)
	{
		if(!FlexibleDataObserver::StrictEquality(*pObs, mLocalFDO)) return false;
	}

	return true;
}

Binary AsyncIntegrationTest::RandomBinary()
{
    boost::uniform_int<> num(0,1);      
	boost::variate_generator<boost::mt19937&, boost::uniform_int<> > val(rng, num);
	Binary v(val() ? true : false, BQ_ONLINE);
	return v;
}

Analog AsyncIntegrationTest::RandomAnalog()
{           
    boost::uniform_int<int_32_t> num;
	boost::variate_generator<boost::mt19937&, boost::uniform_int<int_32_t> > val(rng, num);
	Analog v(val(), AQ_ONLINE);
	return v;
}

Counter AsyncIntegrationTest::RandomCounter()
{              
    boost::uniform_int<uint_32_t> num;
	boost::variate_generator<boost::mt19937&, boost::uniform_int<uint_32_t> > val(rng, num);
	Counter v(val(), CQ_ONLINE);
	return v;
}

void AsyncIntegrationTest::AddStackPair(FilterLevel aLevel, size_t aNumPoints)
{
	uint_16_t port = M_START_PORT + this->mMasterObservers.size();
	
	FlexibleDataObserver* pMasterFDO = new FlexibleDataObserver(); mMasterObservers.push_back(pMasterFDO);
	pMasterFDO->AddObserver(&mNotifier);
	
	ostringstream oss;
	oss << "Port: " << port;
	std::string client = oss.str() + " Client ";
	std::string server = oss.str() + " Server ";

	PhysLayerSettings s(aLevel, 1000);
	this->AddTCPClient(client, s, "127.0.0.1", port);
	this->AddTCPServer(server, s, "127.0.0.1", port);

	{
	MasterStackConfig cfg;
	cfg.app.RspTimeout = 20000;
	cfg.master.IntegrityRate = 60000; //set this to retry, if the task timer doesn't close properly this will seal the deal
	cfg.master.EnableUnsol = true;
	cfg.master.DoUnsolOnStartup = true;
	cfg.master.UnsolClassMask = PC_ALL_EVENTS;
	this->AddMaster(client, client, aLevel, pMasterFDO, cfg);
	}

	{
	SlaveStackConfig cfg;
	cfg.app.RspTimeout = 20000;
	cfg.slave.mDisableUnsol = false;
	cfg.slave.mUnsolPackDelay = 0;
	cfg.device = DeviceTemplate(aNumPoints, aNumPoints, aNumPoints);
	IDataObserver* pObs = this->AddSlave(server, server, aLevel, &mCmdAcceptor, cfg);
	this->mFanout.Add(pObs);
	}

	
}


}}



