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
#include "ASIOIncludes.h"

#include "TimerSourceASIO.h"

#include "TimerASIO.h"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

namespace apl {

TimerSourceASIO::TimerSourceASIO(boost::asio::io_service* apService) :
mpService(apService)
{}

TimerSourceASIO::~TimerSourceASIO()
{
	BOOST_FOREACH(TimerASIO* pTimer, mAllTimers) { delete pTimer; }
}

ITimer* TimerSourceASIO::Start(millis_t aDelay, const ExpirationHandler& arCallback)
{
	TimerASIO* pTimer = GetTimer();
	pTimer->mTimer.expires_from_now(boost::posix_time::milliseconds(aDelay));
	this->StartTimer(pTimer, arCallback);
	return pTimer;
}

ITimer* TimerSourceASIO::Start(const boost::posix_time::ptime& arTime, const ExpirationHandler& arCallback)
{
	TimerASIO* pTimer = GetTimer();
	pTimer->mTimer.expires_at(arTime);
	this->StartTimer(pTimer, arCallback);
	return pTimer;
}

void TimerSourceASIO::Post(const ExpirationHandler& arHandler)
{
	mpService->post(arHandler);
}

TimerASIO* TimerSourceASIO::GetTimer()
{
	TimerASIO* pTimer;
	if(mIdleTimers.size() == 0) {
		pTimer = new TimerASIO(*mpService);
		mAllTimers.push_back(pTimer);
	}
	else {
		pTimer = mIdleTimers.front();
		mIdleTimers.pop_front();
	}

	pTimer->mCanceled = false;
	return pTimer;
}

void TimerSourceASIO::StartTimer(TimerASIO* apTimer, const ExpirationHandler& arCallback)
{
	apTimer->mTimer.async_wait(boost::bind(&TimerSourceASIO::OnTimerCallback, this, _1, apTimer, arCallback));
}

void TimerSourceASIO::OnTimerCallback(const boost::system::error_code& ec, TimerASIO* apTimer, ExpirationHandler aCallback)
{
	mIdleTimers.push_back(apTimer);
	if(! (ec || apTimer->mCanceled) ) aCallback();
}

} //end namespace

