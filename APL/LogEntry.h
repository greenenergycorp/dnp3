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
#ifndef __LOG_ENTRY_H_
#define __LOG_ENTRY_H_

#include "TimingTools.h"
#include "LogBase.h"

namespace apl
{
	
	std::string GetFilterString( FilterLevel aLevel );

	std::string GetLevelCode(int aLevel);

	bool ParseLevelCode(std::string aFilters, int& aLevel);

	class LogEntry
	{
		public:

			const static int MASK_ALL_LEVELS = LEV_DEBUG | LEV_INFO | LEV_COMM | LEV_INTERPRET | LEV_WARNING | LEV_ERROR | LEV_EVENT;

			LogEntry():mTime(TimeStamp::GetUTCTimeStamp()){};

			LogEntry( FilterLevel aLevel, const std::string& aDeviceName, const std::string& aLocation, const std::string& aMessage, int aErrorCode);

			const std::string&	GetDeviceName() { return mDeviceName; }
			const std::string&	GetLocation() { return mLocation; }
			const std::string&	GetMessage() { return mMessage; }
			FilterLevel			GetFilterLevel() { return mFilterLevel; }
			std::string			GetTimeString(){ return TimeStamp::UTCTimeStampToString(mTime);}
			//const apl::Time&	GetTime() { return mTime; }
			int					GetErrorCode(){return mErrorCode; }

			std::string			LogString();

		private:

			FilterLevel		mFilterLevel;
			std::string		mDeviceName;
			std::string		mLocation;
			std::string		mMessage;
			apl::UTCTimeStamp_t		mTime;
			int				mErrorCode;
	};

	

}

#endif
