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
#ifndef __ASYNC_SLAVE_STACK_H_
#define __ASYNC_SLAVE_STACK_H_

#include "AsyncStack.h"
#include "AsyncSlave.h"
#include "AsyncDatabase.h"
#include "DNPCommandMaster.h"
#include "SlaveStackConfig.h"
#include <APL/TimeSource.h>

namespace apl { namespace dnp {

/** @section desc A stack object for a master */
class AsyncSlaveStack : public AsyncStack
{
	public:

	/**
		@param apLogger			Logger used by the stack
		@param apTimerSrc		Timer source used by the slave for asynchronous eventing
		@param apCmdAcceptor	Command acceptor interface used for dispatching commands to the outside world
		@param arCfg			Configuration struct that holds parameters for the stack
	*/
	AsyncSlaveStack(
		Logger* apLogger,
		ITimerSource* apTimerSrc,
		ICommandAcceptor* apCmdAcceptor,
		const SlaveStackConfig& arCfg);

	TimeSourceSystemOffset mTimeSource;
	AsyncDatabase mDB;				/// The database holds static event data and forwards to an event buffer
	DNPCommandMaster mCmdMaster;	/// Controls the execution of commands
	AsyncSlave mSlave;				/// The dnp3 outstation class
};

}}

#endif
