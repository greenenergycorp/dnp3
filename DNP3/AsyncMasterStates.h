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
#ifndef __ASYNC_MASTER_STATES_H_
#define __ASYNC_MASTER_STATES_H_


#include <string>
#include <APL/Singleton.h>
#include <APL/CommandInterfaces.h>

#include "ObjectInterfaces.h"

namespace apl
{
	class ITaskCompletion;
	class Logger;
	class BinaryOutput;
	class Setpoint;
	class ITask;
}

namespace apl { namespace dnp {

class AsyncMaster;
class MasterTaskBase;
class APDU;

class AMS_Base
{
	public:

	// called when a new task should be started
	virtual void StartTask(AsyncMaster* c, ITask*, MasterTaskBase*);

	/* Events from application layer */

	virtual void OnLowerLayerUp(AsyncMaster*);
	virtual void OnLowerLayerDown(AsyncMaster*);

	virtual void OnSendSuccess(AsyncMaster*);
	virtual void OnFailure(AsyncMaster*);

	virtual void OnPartialResponse(AsyncMaster*, const APDU&);
	virtual void OnFinalResponse(AsyncMaster*, const APDU&);

	virtual void OnUnsolResponse(AsyncMaster*, const APDU&);

	virtual std::string Name() const = 0;

	protected:

	void ChangeState(AsyncMaster*, AMS_Base*);
	void ChangeTask(AsyncMaster*, MasterTaskBase*);
};

/* AMS_Closed */

class AMS_Closed : public AMS_Base
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_Closed);

	void OnLowerLayerUp(AsyncMaster*);
};

/* AMS_OpenBase */

class AMS_OpenBase : public AMS_Base
{
	public:
	void OnUnsolResponse(AsyncMaster*, const APDU&);
	virtual void OnLowerLayerDown(AsyncMaster* c);
};

/* AMS_Idle */

/// The only state from which a task can be started
class AMS_Idle : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_Idle);

	void StartTask(AsyncMaster* c, ITask*, MasterTaskBase*);	
};

/* AMS_Waiting */

/// Wait for responses or failure. Manipulates
/// the current task instance.
class AMS_Waiting : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_Waiting);

	void OnFailure(AsyncMaster*);
	void OnPartialResponse(AsyncMaster*, const APDU&);
	void OnFinalResponse(AsyncMaster*, const APDU&);

	void OnLowerLayerDown(AsyncMaster* c);
};

}} //ens ns

#endif
