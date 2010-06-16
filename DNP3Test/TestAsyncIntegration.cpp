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
#include <boost/test/unit_test.hpp>
#include <APLTestTools/TestHelpers.h>

#include <APL/Log.h>
#include <APL/LogToStdio.h>
#include "AsyncIntegrationTest.h"

using namespace apl;
using namespace apl::dnp;

BOOST_AUTO_TEST_SUITE(AsyncIntegrationSuite)

	BOOST_AUTO_TEST_CASE(MasterToSlave)
	{
		#ifdef WIN32
		uint_16_t port = 50000;
		size_t NUM_PAIRS = 10;
		#else
			uint_16_t port = 30000;	
			#ifdef ARM
				size_t NUM_PAIRS = 10;				
			#else
				size_t NUM_PAIRS = 100;
			#endif
		#endif


		StopWatch sw;

		size_t NUM_POINTS = 500;
		size_t NUM_CHANGES = 10;

		EventLog log;
		//log.AddLogSubscriber(LogToStdio::Inst());
		AsyncIntegrationTest t(log.GetLogger(LEV_WARNING, "test"), LEV_WARNING
			, port, NUM_PAIRS, NUM_POINTS);	
		
		IDataObserver* pObs = t.GetFanout();

		for(size_t j=0; j < NUM_CHANGES; ++j) {

			{
				Transaction tr(pObs);
				for(size_t i = 0; i<NUM_POINTS; ++i) pObs->Update(t.RandomBinary(), i);
				for(size_t i = 0; i<NUM_POINTS; ++i) pObs->Update(t.RandomAnalog(), i);
				for(size_t i = 0; i<NUM_POINTS; ++i) pObs->Update(t.RandomCounter(), i);
			}

			BOOST_REQUIRE(t.ProceedUntil(boost::bind(&AsyncIntegrationTest::SameData, &t)));
			//std::cout << "***  Finished change set " <<  j << " ***" << std::endl;
		}

		/*
		double elapsed_sec = sw.Elapsed()/1000.0;
		size_t points = 3*NUM_POINTS*NUM_CHANGES*NUM_PAIRS*2;
		std::cout << "num points: " << points << std::endl;
		std::cout << "elapsed seconds: " << elapsed_sec << std::endl;
		std::cout << "points/sec: " << points/elapsed_sec << std::endl;
		*/
	}

BOOST_AUTO_TEST_SUITE_END()

