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
#include "PhysicalLayerManager.h"

#include "PhysicalLayerFactory.h"
#include "PhysLayerSettings.h"
#include "Logger.h"
#include "Log.h"
#include "Util.h"

#include <boost/bind.hpp>

using namespace std;

namespace apl
{
	PhysicalLayerManager :: PhysicalLayerManager(Logger* apBaseLogger, bool aOwnsLayers) :
	PhysicalLayerMap(apBaseLogger),
	mOwnsLayers(aOwnsLayers)
	{
		
	}

	PhysicalLayerManager :: ~PhysicalLayerManager()
	{
		if(mOwnsLayers) {
			for ( InstanceMap::iterator itr = this->mInstanceMap.begin(); itr != mInstanceMap.end(); itr++ ){
				itr->second.Release();
			}
		}
	}

	void PhysicalLayerManager::Remove(const std::string& arName)
	{
		InstanceMap::iterator i = mInstanceMap.find(arName);
		if(i == mInstanceMap.end()) throw ArgumentException(LOCATION, "Unknown layer");
		if(mOwnsLayers) i->second.Release();
		mInstanceMap.erase(i);
		mSettingsMap.erase(arName);
	}

	void PhysicalLayerManager ::AddTCPClient(const std::string& arName, PhysLayerSettings s, const std::string& arAddr, uint_16_t aPort)
	{		
		IPhysicalLayerAsyncFactory fac = PhysicalLayerFactory::GetTCPClientAsync(arAddr, aPort);
		PhysLayerInstance pli(fac);
		this->AddLayer(arName, s, pli);
	}

	void PhysicalLayerManager ::AddTCPServer(const std::string& arName, PhysLayerSettings s, uint_16_t aPort)
	{		
		IPhysicalLayerAsyncFactory fac = PhysicalLayerFactory::GetTCPServerAsync(aPort);
		PhysLayerInstance pli(fac);
		this->AddLayer(arName, s, pli);
	}

	void PhysicalLayerManager ::AddSerial(const std::string& arName, PhysLayerSettings s, SerialSettings aSerial)
	{
		IPhysicalLayerAsyncFactory fac = PhysicalLayerFactory::GetSerialAsync(aSerial);
		PhysLayerInstance pli(fac);
		this->AddLayer(arName, s, pli);
	}

}
