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
#include "AsyncPhysLayerMonitor.h"

#include "IPhysicalLayerAsync.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <assert.h>
#include "Logger.h"

namespace apl {

AsyncPhysLayerMonitor::AsyncPhysLayerMonitor(Logger* apLogger, IPhysicalLayerAsync* apPhys, ITimerSource* apTimerSrc, millis_t aOpenRetry) :
Loggable(apLogger),
IHandlerAsync(apLogger),
mpPhys(apPhys),
mpTimerSrc(apTimerSrc),
mpOpenTimer(NULL),
mPortState(apLogger, "port_state"),
mOpening(false),
mOpen(false),
mStopOpenRetry(false),
M_OPEN_RETRY(aOpenRetry),
mpMonitor(NULL)
{
	mpPhys->SetHandler(this);
}

AsyncPhysLayerMonitor::~AsyncPhysLayerMonitor()
{}

bool AsyncPhysLayerMonitor::IsRunning()
{
	return mOpen || mOpening || (mpOpenTimer != NULL);
}

bool AsyncPhysLayerMonitor::IsOpen()
{
	return mOpen;
}

void AsyncPhysLayerMonitor::SetMonitor(IPhysMonitor* apMonitor)
{
	mpMonitor = apMonitor;
}

void AsyncPhysLayerMonitor::Notify(IPhysMonitor::State aState)
{
	mPortState.Set(aState);
	if(mpMonitor) mpMonitor->OnStateChange(aState);
}

void AsyncPhysLayerMonitor::Start()
{
	assert(!mOpening);
	if(mpOpenTimer) mpOpenTimer = NULL;
	mOpening = true;
	mStopOpenRetry = false;
	mpPhys->AsyncOpen();
	this->Notify(IPhysMonitor::Opening);
}

void AsyncPhysLayerMonitor::Stop()
{
	mStopOpenRetry = true;
	
	if(!this->IsRunning()) {	
		this->Notify(IPhysMonitor::Stopped);
	}
	else {
		if(mOpen || mOpening) {
			mpPhys->AsyncClose();
		}
		if(mpOpenTimer) {
			mpOpenTimer->Cancel();
			mpOpenTimer = NULL;
		}
	}
}

void AsyncPhysLayerMonitor::_OnOpenFailure()
{
	mOpening = false;
	if(mStopOpenRetry) {
		this->Notify(IPhysMonitor::Stopped); //we're done!
	}
	else {
		this->Notify(IPhysMonitor::Waiting);
		mpOpenTimer = mpTimerSrc->Start(M_OPEN_RETRY, boost::bind(&AsyncPhysLayerMonitor::Start, this));
	}
}

void AsyncPhysLayerMonitor::_OnLowerLayerUp()
{
	assert(mOpening); mOpening = false; mOpen = true;
	this->Up();
	this->Notify(IPhysMonitor::Open);	
}

void AsyncPhysLayerMonitor::_OnLowerLayerDown()
{
	mOpen = false;
		
	this->Down();
	this->Notify(IPhysMonitor::Closed);

	if(mStopOpenRetry) {
		this->Notify(IPhysMonitor::Stopped);
	}
	else this->Start();
}

}
