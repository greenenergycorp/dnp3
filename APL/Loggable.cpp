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
#include "Loggable.h"


#include <assert.h>
#include "Log.h"
#include "Logger.h"

namespace apl
{

Loggable::Loggable( Logger* apLogger, bool aMultiThreaded ) 
: mpLogger(apLogger)
{
#ifdef CHECK_LOGGER_OWNERSHIP
	if(aMultiThreaded) mpLogger->DisableThreadChecking();
#endif
}

/*
bool Loggable::CheckLogLevel(apl::FilterLevel aLevel)
{
	if(mIsLevelSet){
		std::cerr << "WARNING: Log level already set, probably exception in LOG_BLOCK()" << std::endl;
	}

	this->mIsLevelSet = mpLogger->IsEnabled(aLevel);

	if(mIsLevelSet) mLevel = aLevel;

	return mIsLevelSet;
}

void Loggable::Log( const std::string& aLocation, const std::string& aMessage, int aErrorCode)
{	
	assert(mIsLevelSet);

	if ( mpLogger != NULL )
		mpLogger->Log( mLevel, aLocation, aMessage, aErrorCode);

	mIsLevelSet = false;
}
*/

}

