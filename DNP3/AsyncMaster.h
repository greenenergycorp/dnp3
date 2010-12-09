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
#include "MasterSchedule.h"

// includes for tasks
#include "StartupTasks.h"
#include "DataPoll.h"
#include "ControlTasks.h"

namespace apl {
	class IDataObserver;
	class ITask;
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
class AsyncMaster : public Loggable, public IAsyncAppUser
{
	enum CommsStatus
	{
		COMMS_DOWN = 0,
		COMMS_UP = 2
	};

	friend class AMS_Base;
	friend class AMS_Idle;
	friend class AMS_OpenBase;
	friend class AMS_Waiting;
	friend class MasterSchedule;

	public:

	AsyncMaster(Logger*, MasterConfig aCfg, IAsyncAppLayer*, IDataObserver*, AsyncTaskGroup*, ITimerSource*, ITimeSource* apTimeSrc = TimeSource::Inst());
	virtual ~AsyncMaster() {}

	ICommandAcceptor* GetCmdAcceptor() { return &mCommandQueue; }

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

	private:

	/* Task functions used for scheduling */

	void WriteIIN(ITask* apTask);
	void IntegrityPoll(ITask* apTask);
	void EventPoll(ITask* apTask, int aClassMask);
	void ChangeUnsol(ITask* apTask, bool aEnable, int aClassMask);
	void SyncTime(ITask* apTask);
	void ProcessCommand(ITask* apTask);

	IINField mLastIIN;						/// last IIN received from the outstation

	void ProcessIIN(const IINField& arIIN);	/// Analyze IIN bits and react accordingly
	void ProcessDataResponse(const APDU&);	/// Read data output of solicited or unsolicited response and publish
	void StartTask(MasterTaskBase*, bool aInit);		/// Starts a task running

	CachedLogVariable mCommsStatus;			/// notifies the outside world of status

	PostingNotifierSource mNotifierSource;	/// way to get special notifiers for the command queue / vto
	CommandQueue mCommandQueue;				/// Threadsafe queue for buffering command requests
	
	APDU mRequest;							/// APDU that gets reused for requests

	IAsyncAppLayer* mpAppLayer;				 /// lower application layer
	IDataObserver* mpPublisher;				 /// where the measurement are pushed
	AsyncTaskGroup* mpTaskGroup;			 /// How task execution is controlled
	ITimerSource* mpTimerSrc;				 /// Controls the posting of events to marshall across threads
	ITimeSource* mpTimeSrc;					 /// Access to UTC, normally system time but can be a mock for testing

	AMS_Base* mpState;						 /// Pointer to active state, start in TLS_Closed
	MasterTaskBase* mpTask;					 /// The current master task
	ITask* mpScheduledTask;						 /// The current scheduled task

	/* --- Task plumbing --- */

	MasterSchedule mSchedule;				 /// The machinery needed for scheduling
		
	ClassPoll mClassPoll;					 /// used to perform integrity/exception scans
	ClearRestartIIN mClearRestart;			 /// used to clear the restart 
	ConfigureUnsol mConfigureUnsol;			 /// manipulates how the outstation does unsolictied reporting
	TimeSync mTimeSync;						 /// performs time sync on the outstation
	BinaryOutputTask mExecuteBO;			 /// task for executing binary output
	SetpointTask mExecuteSP;				 /// task for executing setpoint

	
};


}}

#endif
