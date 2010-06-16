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
#include "AsyncPort.h"

#include "AsyncStack.h"
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <APL/Logger.h>
#include <APL/IPhysicalLayerAsync.h>
#include <APL/AsyncTaskGroup.h>

namespace apl { namespace dnp {

AsyncPort::AsyncPort(const std::string& arName, Logger* apLogger, AsyncTaskGroup* apGroup, ITimerSource* apTimerSrc, IPhysicalLayerAsync* apPhys, millis_t aOpenDelay) :
Loggable(apLogger->GetSubLogger("port")),
mName(arName),
mRouter(apLogger, apPhys, apTimerSrc, aOpenDelay),
mpGroup(apGroup),
mpPhys(apPhys),
mRelease(false)
{
	mRouter.SetMonitor(this);
}

//ports own their physical layers
AsyncPort::~AsyncPort()
{
	delete mpPhys;
	delete mpGroup;

}

//Once we're sure the router is done with 
void AsyncPort::Release() //do nothing right now
{
	if(mRouter.IsRunning()) mRelease = true;
	else { 
		delete this;
	}
}

void AsyncPort::OnStateChange(IPhysMonitor::State aState)
{
	switch(aState) {
		case(IPhysMonitor::Stopped):
			if(mRelease) delete this; //when the router stops, delete ourselves
			break;
		default:
			break; 
	}	
}

void AsyncPort::Associate(const std::string& arStackName, AsyncStack* apStack, uint_16_t aLocalAddress)
{
	LOG_BLOCK(LEV_DEBUG, "Linking stack to port: " << aLocalAddress);	
	mStackMap[arStackName] = StackRecord(apStack, aLocalAddress);	
	apStack->mLink.SetRouter(&mRouter);
	mRouter.AddContext(&apStack->mLink, aLocalAddress);
	if(!mRouter.IsRunning()) {
		LOG_BLOCK(LEV_DEBUG, "Starting router");
		mRouter.Start();
	}
}

void AsyncPort::Disassociate(const std::string& arStackName)
{	
	StackMap::iterator i = mStackMap.find(arStackName);	
	StackRecord r = i->second;
	LOG_BLOCK(LEV_DEBUG, "Unlinking stack from port: " << r.mLocalAddress);
	mRouter.RemoveContext(r.mLocalAddress);		// decouple the stack from the router and tell the stack to go offline if the it was previously online
	delete r.pStack;							// delete the stack
	if(mRouter.IsRunning() && mRouter.NumContext() == 0) {
		LOG_BLOCK(LEV_DEBUG, "Stopping router");
		mRouter.Stop();
	}
	mStackMap.erase(i);	
}


}}
