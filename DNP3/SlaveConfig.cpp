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
#include "SlaveConfig.h"


#include "DNPConstants.h"


namespace apl { namespace dnp {

	SlaveConfig::SlaveConfig() : 
	
	mMaxControls(1),
	
	mDisableUnsol(false),
			
	mAllowTimeSync(false),
	mTimeSyncPeriod(10*60*1000), //every 10 min

	mUnsolPackDelay(200),
	mUnsolRetryDelay(2000),

	mMaxFragSize(DEFAULT_FRAG_SIZE),
	mMaxBinaryEvents(1000),
	mMaxAnalogEvents(1000),
	mMaxCounterEvents(1000),

	mStaticBinary(GrpVar(1,2)), 
	mStaticAnalog(GrpVar(30,1)),
	mStaticCounter(GrpVar(20,1)),
	mStaticSetpointStatus(GrpVar(40,1)),
	mEventBinary(GrpVar(2,1)),
	mEventAnalog(GrpVar(32,1)),
	mEventCounter(GrpVar(22,1))
	{
		
	}

}}

