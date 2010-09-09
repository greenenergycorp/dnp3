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

#ifndef __MASTER_SCHEDULE_H_
#define __MASTER_SCHEDULE_H_

#include "MasterConfig.h"

namespace apl { 
	class AsyncTaskGroup;
	class AsyncTaskBase;
}

namespace apl { namespace dnp {

class AsyncMaster;

/// Create all the tasks required for the master from the
/// TaskGroup. Defines the types and the dependencies between them.
class MasterSchedule
{
	public:

	AsyncTaskBase* mpCommandTask;		 /// Task to read the command queue
	AsyncTaskBase* mpTimeTask;			 /// Task to synchronize the time on outstation
	AsyncTaskBase* mpClearRestartTask;	 /// Task to clear the restart IIN bit

	/// Enables all of the tasks that run when the layer is Open
	void EnableOnlineTasks();

	/// Disables all of the tasks that run when the layer is Open
	void DisableOnlineTasks();
	
	/// Resets all of the tasks that run on startup. This is typically done after a failure
	void ResetStartupTasks();

	/// Returns a configured MasterSchedule
	static MasterSchedule GetSchedule(MasterConfig aCfg, AsyncMaster*, AsyncTaskGroup*);

	private:

	MasterSchedule(AsyncTaskGroup*);
	
	AsyncTaskGroup* mpGroup;

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
};

}}

#endif
