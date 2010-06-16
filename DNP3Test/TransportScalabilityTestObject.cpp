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
#include <APL/ASIOIncludes.h>

#include "TransportScalabilityTestObject.h"
#include <sstream>
#include <boost/foreach.hpp>

using namespace std;

namespace apl { namespace dnp {

TransportScalabilityTestObject::TransportScalabilityTestObject(
	LinkConfig aClientCfg,
	LinkConfig aServerCfg,
	uint_16_t aPortStart,
	uint_16_t aNumPair,
	FilterLevel aLevel,
	bool aImmediate) :

LogTester(aImmediate),
AsyncTestObjectASIO(),
mpLogger(mLog.GetLogger(aLevel, "test")),
mTimerSource(this->GetService())
{
	const uint_16_t START = aPortStart;
	const uint_16_t STOP = START + aNumPair;

	for(uint_16_t port = START; port < STOP; ++port) {
		ostringstream oss;
		oss << "pair" << port;
		Logger* pLogger = mpLogger->GetSubLogger(oss.str());
		TransportStackPair* pPair = new TransportStackPair(aClientCfg, aServerCfg, pLogger, this->GetService(), &mTimerSource, port);
		mPairs.push_back(pPair);
	}
}

TransportScalabilityTestObject::~TransportScalabilityTestObject()
{
	BOOST_FOREACH(TransportStackPair* pPair, mPairs) delete pPair;
}

bool TransportScalabilityTestObject::AllLayersUp()
{
	BOOST_FOREACH(TransportStackPair* pPair, mPairs) { 
		if(!pPair->BothLayersUp()) return false;
	}

	return true;
}

bool TransportScalabilityTestObject::AllLayerEqual(const byte_t* apData, size_t aNumBytes)
{
	BOOST_FOREACH(TransportStackPair* pPair, mPairs) { 
		if(! pPair->mServerStack.mUpper.BufferEquals(apData, aNumBytes)) return false;
		if(! pPair->mClientStack.mUpper.BufferEquals(apData, aNumBytes)) return false;
	}

	return true;
}

bool TransportScalabilityTestObject::AllLayerReceived(size_t aNumBytes)
{	
	BOOST_FOREACH(TransportStackPair* pPair, mPairs) {
		if(pPair->mServerStack.mUpper.Size() != aNumBytes) return false;		
		if(pPair->mClientStack.mUpper.Size() != aNumBytes) return false;
	}

	return true;
}

void TransportScalabilityTestObject::SendToAll(const byte_t* apData, size_t aNumBytes)
{
	BOOST_FOREACH(TransportStackPair* pPair, mPairs) {
		pPair->mClientStack.mUpper.SendDown(apData, aNumBytes);
		pPair->mServerStack.mUpper.SendDown(apData, aNumBytes);
	}
}

void TransportScalabilityTestObject::Start()
{
	BOOST_FOREACH(TransportStackPair* pPair, mPairs) { pPair->Start(); }
}

}}


