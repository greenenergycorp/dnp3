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
}

namespace apl { namespace dnp {

class AsyncMaster;
class APDU;

class AMS_Base
{
	public:

	/* Task requests from the scheduler */

	virtual void WriteIIN(AsyncMaster*, ITaskCompletion* apTask);
	virtual void IntegrityPoll(AsyncMaster*, ITaskCompletion* apTask);
	virtual void EventPoll(AsyncMaster*, ITaskCompletion* apTask, int aClassMask);
	virtual void ChangeUnsol(AsyncMaster*, ITaskCompletion* apTask, bool aEnable, int aClassMask);
	virtual void SyncTime(AsyncMaster*, ITaskCompletion* apTask);
	virtual void Execute(AsyncMaster*, const BinaryOutput&, size_t);
	virtual void Execute(AsyncMaster*, const Setpoint&, size_t);

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

	//Work functions

	void SetTask(AsyncMaster*, ITaskCompletion*);
	void ChangeState(AsyncMaster*, AMS_Base*);
	void ConfigureClearIIN(AsyncMaster*);
	void ConfigureIntegrityPoll(AsyncMaster*);
	void ConfigureChangeUnsol(AsyncMaster*, bool aEnable, int aClassMask);
	void ConfigureEventPoll(AsyncMaster*, int aClassMask);
	void ConfigureDelayMeas(AsyncMaster*);
	void ConfigureWriteDelay(AsyncMaster*, millis_t aDelay);
	void ConfigureCommand(AsyncMaster*, const BinaryOutput&, size_t aIndex, bool aIsSelect);
	void ConfigureCommand(AsyncMaster*, const Setpoint&, size_t aIndex, bool aIsSelect);
	void SetToOperate(AsyncMaster*);
	CommandStatus Validate(AsyncMaster*, const APDU&);
	bool ValidateDelayMeas(AsyncMaster*, const APDU&, millis_t&);

	void SendRequest(AsyncMaster*);
	void CompleteTask(AsyncMaster*, bool aSuccess);
	Logger* GetLogger(AsyncMaster*);
	void ProcessDataResponse(AsyncMaster*, const APDU&);
	void CompleteCommandTask(AsyncMaster*, CommandStatus);
	CommandObject<Setpoint>* GetOptimalEncoder(SetpointEncodingType aType);
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

class AMS_Idle : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_Idle);

	void WriteIIN(AsyncMaster*, ITaskCompletion* apTask);
	void IntegrityPoll(AsyncMaster*, ITaskCompletion* apTask);
	void Execute(AsyncMaster* c, const BinaryOutput& arCmd, size_t aIndex);
	void Execute(AsyncMaster* c, const Setpoint& arCmd, size_t aIndex);
	void EventPoll(AsyncMaster*, ITaskCompletion* apTask, int aClassMask);
	void ChangeUnsol(AsyncMaster*, ITaskCompletion* apTask, bool aEnable, int aClassMask);
	void SyncTime(AsyncMaster*, ITaskCompletion* apTask);
};

/* AMS_WaitForSimpleRsp */

class AMS_WaitForSimpleRsp : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_WaitForSimpleRsp);

	void OnFailure(AsyncMaster*);
	void OnPartialResponse(AsyncMaster*, const APDU&);
	void OnFinalResponse(AsyncMaster*, const APDU&);
};

/* AMS_WaitForDelayMeasRsp */

class AMS_WaitForDelayMeasRsp : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_WaitForDelayMeasRsp);

	void OnFailure(AsyncMaster*);
	void OnPartialResponse(AsyncMaster*, const APDU&);
	void OnFinalResponse(AsyncMaster*, const APDU&);
};

/* AMS_WaitForRspToPoll */

class AMS_WaitForRspToPoll : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_WaitForRspToPoll);

	void OnFailure(AsyncMaster*);
	void OnPartialResponse(AsyncMaster*, const APDU&);
	void OnFinalResponse(AsyncMaster*, const APDU&);
};

/* AMS_WaitForRspToSelect */

class AMS_WaitForRspToSelect : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_WaitForRspToSelect);

	void OnFailure(AsyncMaster*);
	void OnPartialResponse(AsyncMaster*, const APDU&);
	void OnFinalResponse(AsyncMaster*, const APDU&);
	void OnLowerLayerDown(AsyncMaster*);
};

/* AMS_WaitForRspToOperate */

class AMS_WaitForRspToOperate : public AMS_OpenBase
{
	MACRO_STATE_SINGLETON_INSTANCE(AMS_WaitForRspToOperate);

	void OnFailure(AsyncMaster*);
	void OnPartialResponse(AsyncMaster*, const APDU&);
	void OnFinalResponse(AsyncMaster*, const APDU&);
	void OnLowerLayerDown(AsyncMaster*);
};


}} //ens ns

#endif
