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
#ifndef __DATA_POLL_H_
#define __DATA_POLL_H_

#include "MasterTaskBase.h"

namespace apl {
	class IDataObserver;
	class ITimeSource;
}

namespace apl { namespace dnp {

/** Startup task that acquires the full static state of the remote outstation
*/
class DataPoll : public MasterTaskBase
{	
	public:
		DataPoll(Logger*, ITaskCompletion*, IDataObserver*, ITimeSource*);

	private:

		void ReadData(const APDU&);

		//Implement MasterTaskBase
		TaskResult _OnPartialResponse(const APDU&);
		TaskResult _OnFinalResponse(const APDU&);

		IDataObserver* mpObs;
		ITimeSource* mpTimeSrc;
};

/** Startup task that acquires the full static state of the remote outstation
*/
class IntegrityPoll : public DataPoll
{
	public:
		IntegrityPoll(Logger*, ITaskCompletion*, IDataObserver*, ITimeSource*);
		
		//Implement MasterTaskBase
		void _ConfigureRequest(APDU& arAPDU);						
		virtual std::string Name() const { return "Integrity Poll"; }
};

/** Startup task that acquires the full static state of the remote outstation
*/
class ExceptionPoll : public DataPoll
{
	public:
		ExceptionPoll(Logger*, ITaskCompletion*, IDataObserver*, ITimeSource*, int aClassMask);
		
		//Implement MasterTaskBase
		void _ConfigureRequest(APDU& arAPDU);						
		virtual std::string Name() const { return "Exception Poll"; }

	private:
		int mClassMask;
};

}} //ens ns

#endif
