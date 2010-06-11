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
#ifndef __EVENT_LOG_H_
#define __EVENT_LOG_H_


#include <assert.h>
#include <set>
#include <map>
#include <vector>

#include "LogBase.h"
#include "TimeTypes.h"
#include "Logger.h"
#include "EventLock.h"
#include "Uncopyable.h"
#include "LogEntryCircularBuffer.h"

namespace apl
{

	class EventLog : public ILogBase, private Uncopyable
	{
		public:

			/** Immediate printing to minimize effect of debugging output on execution timing. */
			//EventLog();
			virtual ~EventLog();

			Logger* GetLogger( FilterLevel aFilter, const std::string& aLoggerID );
			Logger* GetExistingLogger( const std::string& aLoggerID );
			void GetAllLoggers( std::vector<Logger*>& apLoggers);

			void AddLogSubscriber(ILogBase* apBase);
			void RemoveLogSubscriber(ILogBase* apBase);

			//implement the log function from ILogBase
			void Log( FilterLevel aFilterLevel, const std::string& aDeviceName, const std::string& aLocation, const std::string& aMessage, int aErrorCode);
			void SetVar(const std::string& aSource, const std::string& aVarName, int aValue);
			
		private:
			SigLock mLock;
			
			//holds pointers to the loggers that have been distributed
			typedef std::map<std::string, Logger*> LoggerMap;
			LoggerMap mLogMap;
			std::set<ILogBase*> mSubscribers;
		
	};
	
	 
}

#endif

