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
#ifndef __ASYNC_DATABASE_TEST_OBJECT_H_
#define __ASYNC_DATABASE_TEST_OBJECT_H_

#include <queue>
#include <DNP3/AsyncDatabase.h>
#include <APL/Log.h>

namespace apl { namespace dnp {

class MockAsyncEventBuffer : public IAsyncEventBuffer
{
	public:

	virtual ~MockAsyncEventBuffer() {}
	
	void Update(const Binary& arEvent, PointClass aClass, size_t aIndex)
	{ 
		BinaryInfo v(arEvent, aClass, aIndex);
		mBinaryEvents.push_back(v);		
	}
	
	void Update(const Analog& arEvent, PointClass aClass, size_t aIndex)
	{
		AnalogInfo v(arEvent, aClass, aIndex);
		mAnalogEvents.push_back(v);		
	}
	
	void Update(const Counter& arEvent, PointClass aClass, size_t aIndex)
	{
		CounterInfo v(arEvent, aClass, aIndex);
		mCounterEvents.push_back(v);		
	}
	
	std::deque<BinaryInfo> mBinaryEvents;
	std::deque<AnalogInfo> mAnalogEvents;
	std::deque<CounterInfo> mCounterEvents;
};

class AsyncDatabaseTestObject
{
	public:
	AsyncDatabaseTestObject(FilterLevel aLevel = LEV_INFO) : 
	db(log.GetLogger(aLevel, "test"))
	{
		db.SetEventBuffer(&buffer);
	}
	
	EventLog log;
	MockAsyncEventBuffer buffer;
	AsyncDatabase db;
};

}}

#endif

