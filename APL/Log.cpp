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

#include <iostream>
#include <sstream>
#include <exception>
#include "Log.h"
#include "Thread.h"
#include "Exception.h"

using namespace std;

namespace apl
{
	
	EventLog::~EventLog()
	{	
		for(LoggerMap::iterator i = mLogMap.begin(); i != mLogMap.end(); i++) {
			delete (i->second);
		}
	}

	void EventLog::Log( FilterLevel aFilterLevel, const std::string& aDeviceName, const std::string& aLocation, const std::string& aMessage, int aErrorCode)
	{
		std::set<ILogBase*>::iterator i = mSubscribers.begin();		
		for(; i != mSubscribers.end(); ++i){
			(*i)->Log(aFilterLevel, aDeviceName, aLocation, aMessage, aErrorCode);
		}
	}

	void EventLog::SetVar(const std::string& aSource, const std::string& aVarName, int aValue)
	{
		std::set<ILogBase*>::iterator i = mSubscribers.begin();		
		for(; i != mSubscribers.end(); ++i){
			(*i)->SetVar(aSource, aVarName, aValue);
		}
	}
	

	Logger* EventLog::GetLogger(FilterLevel aFilter, const std::string& arName)
	{ 
		Logger* pLogger = GetExistingLogger(arName);

		if(pLogger != NULL) return pLogger;

		{
			CriticalSection cs(&mLock);
			assert(mLogMap.find(arName) == mLogMap.end());

			pLogger = new Logger(this, aFilter, arName);
			mLogMap[arName] = pLogger;
		}

		return pLogger;
	}

	Logger* EventLog::GetExistingLogger( const std::string& arLoggerName )
	{		
		CriticalSection cs(&mLock);
		Logger* pLogger = NULL;
		LoggerMap::iterator i = mLogMap.find(arLoggerName);
		if(i != mLogMap.end()) pLogger = i->second;		
		return pLogger;
	}

	void EventLog::GetAllLoggers( std::vector<Logger*>& apLoggers)
	{
		apLoggers.empty();
		CriticalSection cs(&mLock);
		for(LoggerMap::iterator i = mLogMap.begin(); i != mLogMap.end(); i++)
		{
			apLoggers.push_back(i->second);
		}
	}

	void EventLog :: AddLogSubscriber(ILogBase* apBase)
	{
		mSubscribers.insert(apBase);
	}
	
	void EventLog :: RemoveLogSubscriber(ILogBase* apBase)
	{ 
		std::set<ILogBase*>::iterator i = mSubscribers.find(apBase);
		if(i != mSubscribers.end()) mSubscribers.erase(i);
	}

}



