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
#include "AsyncTestObject.h"

#include <APL/Exception.h>
#include <APL/Thread.h>
#include <APL/TimingTools.h>
#include <sstream>

using namespace std;

namespace apl {

bool AsyncTestObject::ProceedUntil(const EvalFunc& arFunc, millis_t aTimeout)
{
	Timeout t(aTimeout);
	
	do
	{
		if(arFunc()) return true;
		else this->Next();
	}
	while(!t.IsExpired());

	return false;
}

bool AsyncTestObject::ProceedUntilFalse(const EvalFunc& arFunc, millis_t aTimeout)
{
	return ProceedUntil(boost::bind(&AsyncTestObject::Negate, arFunc), aTimeout);
}

void AsyncTestObject::Next(boost::asio::io_service* apSrv, millis_t aSleep)
{
	boost::system::error_code ec;
	size_t num = apSrv->poll_one(ec);
	if(ec) throw Exception(LOCATION, ec.message());			
	if(num == 0) { Thread::SleepFor(aSleep); }
	apSrv->reset();
}

}
