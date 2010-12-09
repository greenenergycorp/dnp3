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
#ifndef __ASYNC_SLAVE_STATES_H_
#define __ASYNC_SLAVE_STATES_H_


#include <string>
#include <APL/Singleton.h>
#include <APL/CommandInterfaces.h>
#include <DNP3/AsyncAppInterfaces.h>

namespace apl
{
	class ITaskCompletion;
	class Logger;
	class BinaryOutput;
	class Setpoint;
}

namespace apl { namespace dnp {

class AsyncSlave;
class APDU;

/** @section desc
Base class for all slave transaction states
*/
class AS_Base
{
	public:

	/* Events from application layer */

	virtual void OnLowerLayerUp(AsyncSlave*);
	virtual void OnLowerLayerDown(AsyncSlave*);

	virtual void OnSolSendSuccess(AsyncSlave*);
	virtual void OnSolFailure(AsyncSlave*);

	virtual void OnUnsolSendSuccess(AsyncSlave*);
	virtual void OnUnsolFailure(AsyncSlave*);

	virtual void OnRequest(AsyncSlave*, const APDU&, SequenceInfo);

	virtual void OnUnknown(AsyncSlave*);

	/* Events produced from the user layer */

	/// Called when a data update is received from the user layer
	virtual void OnDataUpdate(AsyncSlave*);

	/// Called when a data update is received from the user layer
	virtual void OnUnsolExpiration(AsyncSlave*);

	/// @return The name associated with the state
	virtual std::string Name() const = 0;

	virtual bool AcceptsDeferredRequests() { return false; }
	virtual bool AcceptsDeferredUpdates()  { return false; }
	virtual bool AcceptsDeferredUnsolExpiration()  { return false; }
	virtual bool AcceptsDeferredUnknown()  { return false; }

	protected:

	void SwitchOnFunction(AsyncSlave*, AS_Base* apNext, const APDU& arRequest, SequenceInfo aSeqInfo);
	void DoUnsolSuccess(AsyncSlave*);
	void DoRequest(AsyncSlave* c, AS_Base* apNext, const APDU& arAPDU, SequenceInfo aSeqInfo);

	//Work functions

	void ChangeState(AsyncSlave*, AS_Base*);
};

/** @section desc
The application layer has not informed the slave yet that it is up
*/
class AS_Closed : public AS_Base
{
	MACRO_STATE_SINGLETON_INSTANCE(AS_Closed);

	void OnLowerLayerUp(AsyncSlave*);
	void OnDataUpdate(AsyncSlave*);

	bool AcceptsDeferUpdates() { return true; }
};

class AS_OpenBase : public AS_Base
{
	public:
	void OnLowerLayerDown(AsyncSlave*);
};

/** @section desc
The app layer is online, but the slave is not doing or waiting on anything
*/
class AS_Idle : public AS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AS_Idle);

	void OnRequest(AsyncSlave*, const APDU&, SequenceInfo);
	void OnDataUpdate(AsyncSlave*);
	void OnUnsolExpiration(AsyncSlave*);
	void OnUnknown(AsyncSlave*);

	bool AcceptsDeferredRequests() { return true; }
	bool AcceptsDeferredUpdates() { return true; }
	bool AcceptsDeferredUnsolExpiration()  { return true; }

};


/** @section desc
The slave is waiting for a response to complete
*/
class AS_WaitForRspSuccess : public AS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AS_WaitForRspSuccess);

	void OnRequest(AsyncSlave*, const APDU&, SequenceInfo);
	void OnSolFailure(AsyncSlave*);
	void OnSolSendSuccess(AsyncSlave*);
};

/** @section desc
The slave is waiting for an unsolicited response to complete
*/
class AS_WaitForUnsolSuccess : public AS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AS_WaitForUnsolSuccess);

	void OnRequest(AsyncSlave*, const APDU&, SequenceInfo);
	void OnUnsolFailure(AsyncSlave*);
	void OnUnsolSendSuccess(AsyncSlave*);
};

/** @section desc
The slave is waiting for an unsolicited response and a solicited response to complete
*/
class AS_WaitForSolUnsolSuccess : public AS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AS_WaitForSolUnsolSuccess);

	void OnRequest(AsyncSlave*, const APDU&, SequenceInfo);
	void OnSolFailure(AsyncSlave*);
	void OnSolSendSuccess(AsyncSlave*);
	void OnUnsolFailure(AsyncSlave*);
	void OnUnsolSendSuccess(AsyncSlave*);
};


}} //ens ns

#endif
