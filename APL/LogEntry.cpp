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
#include "LogEntry.h"


#include <sstream>
#include "Util.h"

using namespace std;

namespace apl
{
	

	LogEntry::LogEntry( FilterLevel aLevel, const std::string& aDeviceName, const std::string& aLocation, const std::string& aMessage, int aErrorCode) 
		:
		mFilterLevel(aLevel),
		mDeviceName(aDeviceName),
		mLocation(aLocation),
		mMessage(aMessage),
		mTime(TimeStamp::GetUTCTimeStamp()),
		mErrorCode(aErrorCode)
	{
	}

	string LogEntry :: LogString()
	{
		ostringstream oss;
		oss << GetTimeString() << " - "
			<< LogTypes::GetLevelString( mFilterLevel ) << " - " 
			<< mDeviceName << " - "
			<< mMessage;

		if(this->GetErrorCode() != -1) oss << " - " << this->GetErrorCode();
				
		return oss.str();
	}

}

