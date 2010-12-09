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
#include "PhysicalLayerMap.h"

#include "PhysLayerSettings.h"
#include "Logger.h"

namespace apl
{
	PhysicalLayerMap::PhysicalLayerMap(Logger* apBaseLogger) : mpBaseLogger(apBaseLogger)
	{
		
	}

	PhysLayerSettings PhysicalLayerMap ::GetSettings(const std::string& arName)
	{
		if(mSettingsMap.find(arName) == mSettingsMap.end())
			throw ArgumentException(LOCATION, "Layer with that name doesn't exist");
		
		return mSettingsMap[arName];
	}

	Logger* PhysicalLayerMap ::MakeLogger(const std::string& arName, FilterLevel aLevel)
	{
		Logger* ret = mpBaseLogger->GetSubLogger(arName);
		//ret->SetVarName(arName);
		ret->SetFilterLevel(aLevel);
		return ret;
	}

	IPhysicalLayerAsync* PhysicalLayerMap ::GetLayer(const std::string& arName, boost::asio::io_service* apService)
	{
		PhysLayerSettings s = this->GetSettings(arName);
		return mInstanceMap[arName].GetAsync(this->MakeLogger(arName, s.LogLevel), apService);
	}

	void PhysicalLayerMap ::AddLayer(const std::string& arName, PhysLayerSettings aSettings, PhysLayerInstance aInstance)
	{
		if(mSettingsMap.find(arName) != mSettingsMap.end())
			throw Exception(LOCATION, "Layer with that name already exists");
		
		mSettingsMap[arName] = aSettings;
		mInstanceMap[arName] = aInstance;
	}


}
