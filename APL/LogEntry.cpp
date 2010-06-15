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
			<< GetFilterString( mFilterLevel ) << " - " 
			<< mDeviceName << " - "
			<< mMessage;

		if(this->GetErrorCode() != -1) oss << " - " << this->GetErrorCode();
				
		return oss.str();
	}

	std::string GetLevelCode(int aLevel)
	{
		ostringstream oss;

		if( (aLevel&LEV_DEBUG) != 0) oss << "D";
		if( (aLevel&LEV_COMM) != 0)	 oss << "C";
		if( (aLevel&LEV_INTERPRET) != 0) oss << "P";
		if( (aLevel&LEV_INFO) != 0) oss << "I";
		if( (aLevel&LEV_WARNING) != 0) oss << "W";
		if( (aLevel&LEV_ERROR) != 0) oss << "E";
		if( (aLevel&LEV_EVENT) != 0) oss << "V";

		return oss.str();
	}

	bool ParseLevelCode(std::string aFilters, int& aLevel)
	{
		toUpperCase(aFilters);

		int level = 0;

		if(aFilters.size() == 1)
		{
			if(aFilters[0] == 'A')
			{
				aLevel = LogEntry::MASK_ALL_LEVELS;
				return true;
			}
			if(aFilters[0] == 'N')
			{
				aLevel = 0;
				return true;
			}
		}

		for(size_t i=0; i<aFilters.size(); i++)
		{
			switch(aFilters[i])
			{
				case('D'):
					level |= LEV_DEBUG;
					break;
				case('C'):
					level |= LEV_COMM;
					break;
				case('P'):
					level |= LEV_INTERPRET;
					break;
				case('I'):
					level |= LEV_INFO;
					break;
				case('W'):
					level |= LEV_WARNING;
					break;
				case('E'):
					level |= LEV_ERROR;
					break;
				case('V'):
					level |= LEV_EVENT;
					break;
				default:
					return false;
			}
		}

		aLevel = level;

		return true;
	}
	
	string GetFilterString( FilterLevel aLevel )
	{
		switch(aLevel)
		{
		case(LEV_DEBUG):
			return "DEBUG";
		case(LEV_COMM):
			return "COMM";
		case(LEV_INTERPRET):
			return "INTERPRET";
		case(LEV_INFO):
			return "INFO";
		case(LEV_WARNING):
			return "WARNING";
		case(LEV_ERROR):
			return "ERROR";
		case(LEV_EVENT):
			return "EVENT";
		default:
			return "UNKNOWN";
		}		
	}
	


}

