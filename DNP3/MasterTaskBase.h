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
#ifndef __MASTER_TASK_BASE_H_
#define __MASTER_TASK_BASE_H_

#include <string>
#include <APL/Loggable.h>

namespace apl { class ITask; }

namespace apl { namespace dnp {

class APDU;
class IINField;

enum TaskResult {
	TR_FAIL,		// The task fails, further responses are ignored
	TR_SUCCESS,		// The tasks is successful and complete
	TR_CONTINUE,    // The task is not yet complete. If OnFinalResponse returns CONTINUE, it's a multi request task
};

/** A generic interface for defining master request/response style tasks
*/
class MasterTaskBase : public Loggable
{
	public:

		MasterTaskBase(Logger* apLogger);

		/// Sets the task completion handler and calls
		/// the overiddable _Init() function
		virtual void Init() {}
		
		/// Configure a request APDU
		virtual void ConfigureRequest(APDU& arAPDU) = 0;
		
		/** Handler for non-FIN responses, performs common validation 
		 *  and delegates to _OnPartialResponse
		 *
		 *	@return TaskResult enumeration
		 */
		TaskResult OnPartialResponse(const APDU&);
		
		/** Handler for FIN responses, performs common validation 
		 *  and delegates to _OnFinalResponse
		 *
		 *	@return True if a valid response, false otherwise
		 */
		TaskResult OnFinalResponse(const APDU&);

		/** Overridable handler for timeouts, layer closes, etc
		 */
		virtual void OnFailure() {}
		
		/// Name of the Task
		virtual std::string Name() const = 0;

	
	protected:

		/** Handler for non-FIN responses
		 *	@return True if a valid response, false otherwise
		 */
		virtual TaskResult _OnPartialResponse(const APDU&) = 0;
		
		/** Handler for FIN responses
		 *	@return True if a valid response, false otherwise
		 */
		virtual TaskResult _OnFinalResponse(const APDU&) = 0;
		
	private:

		TaskResult ProcessResult(TaskResult);

		bool ValidateIIN(const IINField& GetIIN) const;
};

/**
All non-read tasks that only return a single fragment can inherit from this task
*/
class SingleRspBase : public MasterTaskBase
{
	public:
		SingleRspBase(Logger*);
		TaskResult _OnPartialResponse(const APDU&);
};

class SimpleRspBase : public SingleRspBase
{
	public:
		SimpleRspBase(Logger*);
		TaskResult _OnFinalResponse(const APDU&);
};



}} //ens ns

#endif
