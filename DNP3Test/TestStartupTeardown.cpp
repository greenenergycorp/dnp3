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

#include <boost/test/unit_test.hpp>
#include <APLTestTools/TestHelpers.h>

#include "AsyncStartupTeardownTest.h"

#include <boost/asio.hpp>

using namespace std;
using namespace apl;
using namespace apl::dnp;

BOOST_AUTO_TEST_SUITE(AsyncIntegrationSuite)

	/// This test aggressively starts and stops stacks
	/// while the io_service is running
	BOOST_AUTO_TEST_CASE(StackTearDown)
	{
		const size_t NUM_STACKS = 10;
		const size_t NUM_PORTS = 10;

		FilterLevel lev = LEV_WARNING;

		AsyncStartupTeardownTest t(lev, true); //autostart = true
	
		for(size_t i=0; i<NUM_PORTS; ++i) {
			ostringstream port;
			port << "port" << i;
			
			//since auto is true, the stack starts executing as soon as the first stack is added
			t.CreatePort(port.str(), lev);
		
			for(size_t i=0; i<NUM_STACKS; ++i) {
				ostringstream stack;
				stack << port.str() << " - stack" << i;
				
				t.AddMaster(stack.str(), port.str(), static_cast<uint_16_t>(i), LEV_WARNING);	
			}
		}
			
		// shutdown starts when the destructor is called.
	}

BOOST_AUTO_TEST_SUITE_END()

