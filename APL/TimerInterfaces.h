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
#ifndef __TIMER_INTERFACES_H_
#define __TIMER_INTERFACES_H_


#include "Types.h"
#include "TimeTypes.h"
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace apl {

/**
 * This is a wrapper for ASIO timers that are used to post events
 * on a queue. Events can be posted for immediate consumption or
 * some time in the future. Events can be consumbed by the posting
 * thread or another thread.
 *
 * @section Class Goals
 *
 * Decouple APL code form ASIO so ASIO could be replace if need be.
 *
 * There is a problem with ASIO. When cancel is called, an event is
 * posted. We wanted a cancel that does not generate any events.
 *
 * @see TimerASIO
 */
class ITimer
{
	public:
	virtual ~ITimer() {}
	virtual void Cancel() = 0;
	virtual boost::posix_time::ptime ExpiresAt() = 0;
};

/** @section desc Parameterless signature for Expiration callbacks */
typedef boost::function<void ()> ExpirationHandler;


/**
 * Interface for posting events to a queue.  Events can be posted for
 * immediate consumption or some time in the future.  Events can be consumbed
 * by the posting thread or another thread.
 *
 * @section Usage
 *
 * \code
 * 	 asio::io_service srv;
 * 	 TimerSourceASIO timers(&srv);
 * \endcode
 *
 * @see TimerASIO
 */
class ITimerSource
{
	public:
	virtual ~ITimerSource() {}

	ITimer* StartInfinite(const ExpirationHandler& arHandler)
	{
		boost::posix_time::ptime t(boost::date_time::max_date_time);		
		return this->Start(t, arHandler);
	}

	/** Returns a new timer based on a relative time */
	virtual ITimer* Start(millis_t, const ExpirationHandler&) = 0;

	/** Returns a new timer based on an absolute time */
	virtual ITimer* Start(const boost::posix_time::ptime&, const ExpirationHandler&) = 0;

	/** Thread-safe way to post an event to handled asynchronously */
	virtual void Post(const ExpirationHandler&) = 0;
};

}

#endif
