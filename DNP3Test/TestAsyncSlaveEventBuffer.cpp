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

#include <APL/Exception.h>
#include <DNP3/AsyncSlaveEventBuffer.h>
#include <limits>

using namespace std;
using namespace apl;
using namespace apl::dnp;


	BOOST_AUTO_TEST_SUITE(AsyncSlaveEventBufferSuite)
		void PushEvents(AsyncSlaveEventBuffer& b, size_t aNumEvent, size_t aNumIndices)
		{
			for(size_t i=0; i<aNumEvent; ++i) {
				b.Update(Analog(i), PC_CLASS_1, i%aNumIndices);
			}
		}

		template <class T>
		void OverflowTest(const T& first, const T& second)
		{
			AsyncSlaveEventBuffer b(1, 1, 1);
			b.Update(first, PC_CLASS_1, 0);
			BOOST_REQUIRE_FALSE(b.IsOverflow());
			BOOST_REQUIRE(b.HasEventData());
			BOOST_REQUIRE_EQUAL(b.NumType(T::MeasEnum), 1);
			b.Update(second, PC_CLASS_1, 1);
			BOOST_REQUIRE(b.IsOverflow());
		}

		BOOST_AUTO_TEST_CASE(AnalogInsertion)
		{
			const size_t NUM_EVENT = 100;
			const size_t NUM_INDICES = 10;

			AsyncSlaveEventBuffer b(0, NUM_INDICES, 0);
			
			PushEvents(b, NUM_EVENT, NUM_INDICES); //push lots of events but only 
			BOOST_REQUIRE_FALSE(b.IsOverflow());
			BOOST_REQUIRE_EQUAL(b.NumType(DT_ANALOG), NUM_INDICES);
			
			b.Select(DT_ANALOG, PC_CLASS_1); //select all the events
			BOOST_REQUIRE_EQUAL(b.NumType(DT_ANALOG), NUM_INDICES);
			PushEvents(b, NUM_EVENT, NUM_INDICES);
			BOOST_REQUIRE_FALSE(b.IsOverflow());
			BOOST_REQUIRE_EQUAL(b.NumType(DT_ANALOG), 2*NUM_INDICES);

			//now deselect
			b.Deselect();
			BOOST_REQUIRE_EQUAL(b.NumType(DT_ANALOG), NUM_INDICES);
			BOOST_REQUIRE_FALSE(b.IsOverflow()); //still shouldn't be 

		}

		BOOST_AUTO_TEST_CASE(OverflowAnalog)
		{
			OverflowTest(Analog(5), Analog(6));
		}
		BOOST_AUTO_TEST_CASE(OverflowBinary)
		{
			OverflowTest(Binary(true), Binary(false));
		}
		BOOST_AUTO_TEST_CASE(OverflowCounter)
		{
			OverflowTest(Counter(1), Counter(6));
		}

		BOOST_AUTO_TEST_CASE(ClassSelect)
		{
			AsyncSlaveEventBuffer b(10, 10, 10);
			b.Update(Binary(true), PC_CLASS_1, 0);
			b.Update(Binary(false), PC_CLASS_1, 0);
			b.Update(Analog(5), PC_CLASS_1, 0);
			b.Update(Analog(6), PC_CLASS_1, 1);
			b.Update(Counter(1), PC_CLASS_1, 0);
			b.Update(Counter(3), PC_CLASS_1, 1);
			
			b.Select(PC_CLASS_1, 1);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_BINARY), 1);
			b.Deselect();

			b.Select(PC_CLASS_1, 3);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_BINARY), 2);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_ANALOG), 1);
			b.Deselect();

			b.Select(PC_CLASS_1, 5);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_BINARY), 2);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_ANALOG), 2);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_COUNTER), 1);
			b.Deselect();

			b.Select(PC_CLASS_1, 6);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_BINARY), 2);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_ANALOG), 2);
			BOOST_REQUIRE_EQUAL(b.NumSelected(DT_COUNTER), 2);
		}

		BOOST_AUTO_TEST_CASE(TypeExceptions)
		{
			AsyncSlaveEventBuffer b(10, 10, 10);
			BOOST_REQUIRE_THROW(b.NumType(DT_CONTROL_STATUS), ArgumentException);
			BOOST_REQUIRE_THROW(b.NumSelected(DT_CONTROL_STATUS), ArgumentException);
		}
	BOOST_AUTO_TEST_SUITE_END()
