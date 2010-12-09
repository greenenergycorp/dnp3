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
#include <boost/test/unit_test.hpp>
#include <APLTestTools/TestHelpers.h>

#include <APL/Exception.h>

#include "AsyncLinkLayerRouterTest.h"
#include "MockFrameSink.h"

using namespace apl;
using namespace apl::dnp;

BOOST_AUTO_TEST_SUITE(AsyncLinkLayerRouterSuite)

	/// Test the open retry behavior
	BOOST_AUTO_TEST_CASE(OpenRetryBehavior) {
		AsyncLinkLayerRouterTest t;
		t.router.Start();
		BOOST_REQUIRE_EQUAL(t.phys.NumOpen(), 1);
		BOOST_REQUIRE_EQUAL(t.phys.NumOpenSuccess(), 0);
		BOOST_REQUIRE_EQUAL(t.phys.NumOpenFailure(), 0);
		t.phys.SignalOpenFailure();
		BOOST_REQUIRE_EQUAL(t.phys.NumOpen(), 1);
		BOOST_REQUIRE_EQUAL(t.phys.NumOpenSuccess(), 0);
		BOOST_REQUIRE_EQUAL(t.phys.NumOpenFailure(), 1);
		BOOST_REQUIRE(t.mts.DispatchOne());
		BOOST_REQUIRE_EQUAL(t.phys.NumOpen(), 2);
		BOOST_REQUIRE_EQUAL(t.phys.NumOpenSuccess(), 0);
		BOOST_REQUIRE_EQUAL(t.phys.NumOpenFailure(), 1);
	}
	
	/// Test that send frames from unknown sources are rejected
	BOOST_AUTO_TEST_CASE(UnknownSourceException) {
		AsyncLinkLayerRouterTest t;
		LinkFrame f;
		f.FormatAck(true, false, 1, 2);
		BOOST_REQUIRE_THROW(t.router.Transmit(f), ArgumentException);
	}

	/// Test that frames with unknown destinations are correctly logged
	BOOST_AUTO_TEST_CASE(UnknownDestination){
		AsyncLinkLayerRouterTest t;
		t.router.Start();
		t.phys.SignalOpenSuccess();
		t.phys.TriggerRead("05 64 05 C0 01 00 00 04 E9 21");
		BOOST_REQUIRE_EQUAL(t.NextErrorCode(), DLERR_UNKNOWN_DESTINATION);
	}
	
	/// Test that the router rejects sends until it is online
	BOOST_AUTO_TEST_CASE(LayerNotOnline){
		AsyncLinkLayerRouterTest t;
		MockFrameSink mfs;
		t.router.AddContext(&mfs, 1024);
		LinkFrame f;
		f.FormatAck(true, false, 1, 1024);
		BOOST_REQUIRE_THROW(t.router.Transmit(f), InvalidStateException);
	}
	
	/// Test that router is correctly clears the send buffer on close
	BOOST_AUTO_TEST_CASE(CloseBehavior){
		AsyncLinkLayerRouterTest t;
		MockFrameSink mfs;
		t.router.AddContext(&mfs, 1024);
		t.router.Start(); t.phys.SignalOpenSuccess();
		LinkFrame f;
		f.FormatAck(true, false, 1, 1024);
		t.router.Transmit(f); // puts the router in the send state
		BOOST_REQUIRE_EQUAL(t.phys.NumWrites(), 1);
		t.phys.AsyncClose(); //we're both reading and writing so this doesn't trigger a callback yet
		BOOST_REQUIRE(mfs.mLowerOnline);
		t.phys.SignalSendFailure();
		BOOST_REQUIRE(mfs.mLowerOnline);
		t.phys.SignalReadFailure();

		// now the layer should go offline, this should clear the transmitt queue,
		// the router should also try to restart
		BOOST_REQUIRE_FALSE(mfs.mLowerOnline); 

		t.phys.ClearBuffer();
		
		t.phys.SignalOpenSuccess();

		//format another request, but change the to address
		LinkFrame f2; f2.FormatAck(true, false, 2, 1024);
		t.router.Transmit(f2);
		BOOST_REQUIRE_EQUAL(t.phys.NumWrites(), 2);
		BOOST_REQUIRE(t.phys.BufferEquals(f2.GetBuffer(), f2.GetSize()));
		t.phys.SignalSendSuccess();
		BOOST_REQUIRE_EQUAL(t.phys.NumWrites(), 2);


	}
	
	/// Test that the second bind fails when a non-unique address is added
	BOOST_AUTO_TEST_CASE(MultiAddressBindError){
		AsyncLinkLayerRouterTest t;
		MockFrameSink mfs;
		t.router.AddContext(&mfs, 1024);
		BOOST_REQUIRE_THROW(t.router.AddContext(&mfs, 1024), ArgumentException);
	}

	/// Test that the second bind fails when a non-unique context is added
	BOOST_AUTO_TEST_CASE(MultiContextBindError){
		AsyncLinkLayerRouterTest t;
		MockFrameSink mfs;
		t.router.AddContext(&mfs, 1024);
		BOOST_REQUIRE_THROW(t.router.AddContext(&mfs, 2048), ArgumentException);
	}

	/// Test that router correctly buffers and sends frames from multiple contexts
	BOOST_AUTO_TEST_CASE(MultiContextSend){
		AsyncLinkLayerRouterTest t;
		MockFrameSink mfs1;
		MockFrameSink mfs2;
		t.router.AddContext(&mfs1, 1024);
		t.router.AddContext(&mfs2, 2048);
		LinkFrame f1; f1.FormatAck(true, false, 1, 1024);
		LinkFrame f2; f2.FormatAck(true, false, 1, 2048);
		t.router.Start(); t.phys.SignalOpenSuccess();
		t.router.Transmit(f1);
		t.router.Transmit(f2);
		BOOST_REQUIRE_EQUAL(t.phys.NumWrites(), 1);			
		t.phys.SignalSendSuccess();			
		BOOST_REQUIRE_EQUAL(t.phys.NumWrites(), 2);
		t.phys.SignalSendSuccess();			
		BOOST_REQUIRE_EQUAL(t.phys.NumWrites(), 2);
	}
	


BOOST_AUTO_TEST_SUITE_END()
