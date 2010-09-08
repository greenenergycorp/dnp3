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
#ifndef __ASYNC_MASTER_H_
#define __ASYNC_MASTER_H_


#include <APL/Loggable.h>
#include <APL/CommandTypes.h>
#include <APL/CommandQueue.h>
#include <APL/TimeSource.h>
#include <APL/PostingNotifierSource.h>
#include <APL/CachedLogVariable.h>

#include "APDU.h"
#include "AsyncAppInterfaces.h"
#include "ObjectReadIterator.h"
#include "MasterConfig.h"
#include "ObjectInterfaces.h"


namespace apl {
	class IDataObserver;
	class ITaskCompletion;
	class AsyncTaskGroup;
	class ITimerSource;
	class ITimeSource;
	class AsyncTaskContinuous;
	class AsyncTaskBase;
	class CopyableBuffer;
}

namespace apl { namespace dnp {

class AMS_Base;

/** DNP3 master. The tasks functions can perform all the various things that a master might need to do.

	Coordination of tasks is handled by a higher level task scheduler.
*/
class AsyncMaster : public Loggable, public IAsyncAppUser, public ICommandAcceptor
{
	enum AsyncMasterPriority
	{
		AMP_POLL,
		AMP_TIME_SYNC,
		AMP_CLEAR_RESTART,
		AMP_UNSOL_CHANGE,
		AMP_COMMAND
	};

	enum TaskTypes
	{
		ONLINE_ONLY_TASKS = 1,
		START_UP_TASKS = 2
	};

	enum CommsStatus
	{
		COMMS_DOWN = 0,
		COMMS_UP = 2
	};

	friend class AMS_Base;

	public:

	AsyncMaster(Logger*, MasterConfig aCfg, IAsyncAppLayer*, IDataObserver*, AsyncTaskGroup*, ITimerSource*, ITimeSource* apTimeSrc = TimeSource::Inst());
	virtual ~AsyncMaster() {}

	ICommandAcceptor* GetCmdAcceptor() { return &mCommandQueue; }

	/* Public task functions used for scheduling */

	void WriteIIN(ITaskCompletion* apTask);
	void IntegrityPoll(ITaskCompletion* apTask);
	void EventPoll(ITaskCompletion* apTask, int aClassMask);
	void ChangeUnsol(ITaskCompletion* apTask, bool aEnable, int aClassMask);
	void SyncTime(ITaskCompletion* apTask);
	void ExecuteCommand();

	/* Implement IAsyncAppUser - callbacks from the app layer */

	void OnLowerLayerUp();
	void OnLowerLayerDown();

	void OnSolSendSuccess();
	void OnSolFailure();

	void OnUnsolSendSuccess();
	void OnUnsolFailure();

	// override the response functions
	void OnPartialResponse(const APDU&);
	void OnFinalResponse(const APDU&);
	void OnUnsolResponse(const APDU&);

	bool IsMaster() { return true; }

	/* Implement ICommandAcceptor. Used to dispatch command out of the command queue */

	void AcceptCommand(const BinaryOutput&, size_t, int aSequence, IResponseAcceptor* apRspAcceptor);
	void AcceptCommand(const Setpoint&, size_t, int aSequence, IResponseAcceptor* apRspAcceptor);

	private:

	IINField mLastIIN;									/// last IIN received from the outstation

	void ProcessIIN(const IINField& arIIN);				/// Analyze IIN bits and react accordingly

	void EnableOnlineTasks();							/// enable all tasks flaged ONLINE_ONLY
	void DisableOnlineTasks();							/// disable ''
	void CompleteCommandTask(CommandStatus aStatus);	/// finalize the execution of the command task

	void ProcessDataResponse(const APDU&);	/// Read data output of solicited or unsolicited response and publish

	PostingNotifierSource mNotifierSource;	/// way to get special notifiers for the command queue / vto
	CommandQueue mCommandQueue;				/// Threadsafe queue for buffering command requests
	AMS_Base* mpState;						/// Pointer to active state, start in TLS_Closed
	ITaskCompletion* mpTask;				/// Pointer to active task, NULL if no task

	APDU mRequest;							/// APDU that gets reused for requests

	IAsyncAppLayer* mpAppLayer;				 /// lower application layer
	IDataObserver* mpPublisher;				 /// where the measurement are pushed
	AsyncTaskGroup* mpTaskGroup;			 /// How task execution is controlled
	ITimerSource* mpTimerSrc;				 /// Controls the posting of events to marshall across threads
	ITimeSource* mpTimeSrc;					 /// Access to UTC, normally system time but can be a mock for testing
	AsyncTaskContinuous* mpCommandTask;		 /// Task to read the command queue
	AsyncTaskContinuous* mpTimeTask;		 /// Task to synchronize the time on outstation
	AsyncTaskContinuous* mpClearRestartTask; /// Task to clear the restart IIN bit

	CommandData mCmdInfo;
	CachedLogVariable mCommsStatus;


};


}}

#endif
