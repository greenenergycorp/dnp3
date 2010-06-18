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
#include "AsyncPhysTestObject.h"

#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>

#include <APL/LogToStdio.h>
#include <APL/TimerSourceASIO.h>
#include <APL/Exception.h>
#include <APL/IOServiceThread.h>

#include <APLTestTools/TestHelpers.h>
#include <APLTestTools/BufferHelpers.h>
#include <APLTestTools/AsyncTestObjectASIO.h>

#include <iostream>

using namespace apl;
using namespace boost;


BOOST_AUTO_TEST_SUITE(PhysicalLayerAsyncTCPSuite)

	BOOST_AUTO_TEST_CASE(TestStateClosed)
	{
		AsyncPhysTestObject t(LEV_INFO, false);

		byte_t buff[100];

		// Test that reads/writes of length 0 throw ArgumentException
		BOOST_REQUIRE_THROW(t.mTCPClient.AsyncWrite(buff,0), ArgumentException);
		BOOST_REQUIRE_THROW(t.mTCPClient.AsyncRead(buff,0), ArgumentException);

		//Test that in the closed state we get the proper invalid state exceptions
		BOOST_REQUIRE_THROW(t.mTCPClient.AsyncWrite(buff,100), InvalidStateException);
		BOOST_REQUIRE_THROW(t.mTCPClient.AsyncRead(buff,100), InvalidStateException);
		BOOST_REQUIRE_THROW(t.mTCPClient.AsyncClose(), InvalidStateException);	
	}

	BOOST_AUTO_TEST_CASE(ClientConnectionRejected)
	{
		AsyncPhysTestObject t(LEV_INFO, false);

		BOOST_REQUIRE_EQUAL(t.mClientAdapter.GetNumOpenFailure(), 0);
		
		for(size_t i=0; i<2; ++i) {
			t.mTCPClient.AsyncOpen();
			BOOST_REQUIRE(t.ProceedUntil(boost::bind(&LowerLayerToPhysAdapter::OpenFailureEquals, &t.mClientAdapter, i+1)));
		}
	}

	BOOST_AUTO_TEST_CASE(ClientConnectionCanceled)
	{
		AsyncPhysTestObject t(LEV_INFO, false);
		
		for(size_t i=0; i<2; ++i) {	
			t.mTCPClient.AsyncOpen();
			t.mTCPClient.AsyncClose();

			BOOST_REQUIRE(t.ProceedUntil(boost::bind(&LowerLayerToPhysAdapter::OpenFailureEquals, &t.mClientAdapter, i+1)));
		}
	}

	BOOST_AUTO_TEST_CASE(ServerAcceptCanceled)
	{
		AsyncPhysTestObject t(LEV_INFO, false);
		
		for(size_t i=0; i<2; ++i) {	
			t.mTCPServer.AsyncOpen();
			t.mTCPServer.AsyncClose();

			BOOST_REQUIRE(t.ProceedUntil(boost::bind(&LowerLayerToPhysAdapter::OpenFailureEquals, &t.mServerAdapter, i+1)));
		}
	}

	BOOST_AUTO_TEST_CASE(ConnectDisconnect)
	{
		AsyncPhysTestObject t(LEV_INFO, false);

		for(size_t i=0; i< 10; ++i) {

			t.mTCPServer.AsyncOpen();
			t.mTCPClient.AsyncOpen();
			BOOST_REQUIRE(t.ProceedUntil(bind(&MockUpperLayer::IsLowerLayerUp, &t.mServerUpper)));
			BOOST_REQUIRE(t.ProceedUntil(bind(&MockUpperLayer::IsLowerLayerUp, &t.mClientUpper)));
			
			//Check that since reads are outstanding, you only have to stop 1/2 of the connection
			if( (i%2) == 0 ) t.mTCPServer.AsyncClose();
			else t.mTCPClient.AsyncClose();
			BOOST_REQUIRE(t.ProceedUntilFalse(bind(&MockUpperLayer::IsLowerLayerUp, &t.mServerUpper)));
			BOOST_REQUIRE(t.ProceedUntilFalse(bind(&MockUpperLayer::IsLowerLayerUp, &t.mClientUpper)));
		}
	}

	BOOST_AUTO_TEST_CASE(TestSendShutdown)
	{
		AsyncPhysTestObject t(LEV_INFO, false);

		t.mTCPServer.AsyncOpen();
		t.mTCPClient.AsyncOpen();
		BOOST_REQUIRE(t.ProceedUntil(bind(&MockUpperLayer::IsLowerLayerUp, &t.mServerUpper)));
		BOOST_REQUIRE(t.ProceedUntil(bind(&MockUpperLayer::IsLowerLayerUp, &t.mClientUpper)));

		ByteStr bs(1024, 77); //give some interesting seed value to make sure bytes are correctly written
		t.mClientUpper.SendDown(bs.Buffer(), bs.Size());

		t.mTCPClient.AsyncClose();
		BOOST_REQUIRE(t.ProceedUntilFalse(bind(&MockUpperLayer::IsLowerLayerUp, &t.mServerUpper)));
		BOOST_REQUIRE(t.ProceedUntilFalse(bind(&MockUpperLayer::IsLowerLayerUp, &t.mClientUpper)));
	}

	BOOST_AUTO_TEST_CASE(TwoWaySend)
	{
		const size_t SEND_SIZE = 1 << 20; // 1 MB

		AsyncPhysTestObject t(LEV_INFO, false);

		t.mTCPServer.AsyncOpen();
		t.mTCPClient.AsyncOpen();
		BOOST_REQUIRE(t.ProceedUntil(bind(&MockUpperLayer::IsLowerLayerUp, &t.mServerUpper)));
		BOOST_REQUIRE(t.ProceedUntil(bind(&MockUpperLayer::IsLowerLayerUp, &t.mClientUpper)));

		//both layers are now up and reading, start them both writing
		ByteStr bs(SEND_SIZE, 77); //give some interesting seed value to make sure bytes are correctly written
		t.mClientUpper.SendDown(bs.Buffer(), bs.Size());
		t.mServerUpper.SendDown(bs.Buffer(), bs.Size());

		BOOST_REQUIRE(t.ProceedUntil(boost::bind(&MockUpperLayer::SizeEquals, &t.mServerUpper, SEND_SIZE)));
		BOOST_REQUIRE(t.ProceedUntil(boost::bind(&MockUpperLayer::SizeEquals, &t.mClientUpper, SEND_SIZE)));
		
		BOOST_REQUIRE(t.mClientUpper.BufferEquals(bs.Buffer(), bs.Size()));
		BOOST_REQUIRE(t.mServerUpper.BufferEquals(bs.Buffer(), bs.Size()));

		t.mTCPServer.AsyncClose(); //stop one side
		BOOST_REQUIRE(t.ProceedUntilFalse(bind(&MockUpperLayer::IsLowerLayerUp, &t.mServerUpper)));
		BOOST_REQUIRE(t.ProceedUntilFalse(bind(&MockUpperLayer::IsLowerLayerUp, &t.mClientUpper)));
	}

#ifndef ARM

	BOOST_AUTO_TEST_CASE(Loopback)
	{		
		EventLog logserver;
		boost::asio::io_service loopservice;
		TimerSourceASIO timer_src(&loopservice);
		PhysicalLayerAsyncTCPServer server(logserver.GetLogger(LEV_INFO, "server"), &loopservice, "127.0.0.1", 30000);
		AsyncLoopback t(logserver.GetLogger(LEV_INFO, "loopback"), &server, &timer_src, LEV_WARNING, false);
		IOServiceThread iost(&loopservice);				
		t.Start();
		iost.Start();

		EventLog log;		
		Logger* logger = log.GetLogger(LEV_DEBUG, "client");			
		AsyncTestObjectASIO test;
		PhysicalLayerAsyncTCPClient client(logger, test.GetService(), "127.0.0.1", 30000);
		LowerLayerToPhysAdapter adapter(logger, &client);
		MockUpperLayer upper(logger);
		adapter.SetUpperLayer(&upper);
							
		client.AsyncOpen();
		BOOST_REQUIRE(test.ProceedUntil(boost::bind(&MockUpperLayer::IsLowerLayerUp, &upper)));
									
		for(size_t i=0; i<1000; ++i) {						
			upper.SendDown("01 02 03");			
			BOOST_REQUIRE(test.ProceedUntil(boost::bind(&MockUpperLayer::BufferEquals, &upper, "01 02 03")));
			BOOST_REQUIRE(test.ProceedUntil(boost::bind(&MockUpperLayer::CountersEqual, &upper, 1, 0)));
			upper.ClearBuffer();
			upper.Reset();
		}		
	}

#endif


BOOST_AUTO_TEST_SUITE_END()

