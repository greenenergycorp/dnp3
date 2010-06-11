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
#include "Logger.h"
#include <assert.h>
#include "Log.h"
#include <iostream>
using namespace std;


namespace apl
{
	Logger::Logger(EventLog* apLog, FilterLevel aFilter, const std::string& aName)
		: 
		mLevel(0),
		mpLog(apLog),
		mName(aName),
		mVarName(aName)
	{ 
		this->SetFilterLevel(aFilter);
	}

	void Logger::SetFilterLevel(FilterLevel aFilter)
	{
		//since FilterLevel is a power of 2 (single bit), subtracting 1 will
		//set all the bits below the set bit.
		//set the filter bit and all the bits below it
		mLevel = aFilter | (aFilter-1);
	}

	Logger* Logger::GetSubLogger(std::string aSubName, int aFilterBits)
	{
		std::ostringstream oss;
		oss << mName << "-" << aSubName;
		Logger* pLogger = mpLog->GetLogger(LEV_WARNING, oss.str());
		pLogger->SetVarName(aSubName);
		pLogger->mLevel = aFilterBits;
		return pLogger;
	}
	Logger* Logger::GetSubLogger(std::string aSubName, FilterLevel aFilter)
	{
		std::ostringstream oss;
		oss << mName << "-" << aSubName;
		Logger* pLogger = mpLog->GetLogger(LEV_WARNING, oss.str());
		pLogger->SetVarName(aSubName);
		pLogger->SetFilterLevel(aFilter);
		return pLogger;
	}

	Logger* Logger::GetSubLogger(std::string aSubName)
	{
		return this->GetSubLogger(aSubName, this->mLevel);
	}

	void Logger::Log( FilterLevel aFilterLevel, const std::string& aLocation, const std::string& aMessage, int aErrorCode)
	{
		if(this->IsEnabled(aFilterLevel)) {
			mpLog->Log(aFilterLevel, mName, aLocation, aMessage, aErrorCode);
		}
	}

	void Logger::Set(const std::string& aVar, int aValue)
	{
		mpLog->SetVar(mVarName, aVar, aValue);	
	}
		
}

