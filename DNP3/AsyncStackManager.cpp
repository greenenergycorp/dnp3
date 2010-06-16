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
#include <APL/ASIOIncludes.h>
#include "AsyncStackManager.h"
#include "AsyncPort.h"

#include <boost/foreach.hpp>
#include <APL/TimerSourceASIO.h>
#include <APL/Exception.h>
#include <APL/Logger.h>

#include <DNP3/AsyncMasterStack.h>
#include <DNP3/AsyncSlaveStack.h>
#include <DNP3/DeviceTemplate.h>

#include <iostream>
#include <boost/bind.hpp>

using namespace std;

namespace apl { namespace dnp {

AsyncStackManager::AsyncStackManager(Logger* apLogger, bool aAutoRun) :
Loggable(apLogger),
mRunASIO(aAutoRun),
mRunning(false),
mService(),
mTimerSrc(mService.Get()),
mMgr(apLogger->GetSubLogger("ports", LEV_WARNING), false),	// the false here is important!!!												  
mScheduler(&mTimerSrc),								        // it means that any layer we retrieve from the manager, we will delete
mThread(this)
{
	
}

AsyncStackManager::~AsyncStackManager()
{	
	this->Stop(); // tell every port to stop, join on the thread	
	this->Run(); // run out the io_service, to make sure all the deletes are called, if the io_service wasn't running
}

std::vector<std::string> AsyncStackManager::GetStackNames() { return GetKeys<PortMap, string>(mStackToPort); }
std::vector<std::string> AsyncStackManager::GetPortNames()  {  return GetKeys<PortMap, string>(mPortToPort); }

void AsyncStackManager::AddTCPClient(const std::string& arName, PhysLayerSettings aSettings, const std::string& arAddr, uint_16_t aPort)
{	
	mMgr.AddTCPClient(arName, aSettings, arAddr, aPort);
}

void AsyncStackManager::AddTCPServer(const std::string& arName, PhysLayerSettings aSettings, uint_16_t aPort)
{	
	mMgr.AddTCPServer(arName, aSettings, aPort);
}

void AsyncStackManager::AddSerial(const std::string& arName, PhysLayerSettings aSettings, SerialSettings aSerial)
{
	mMgr.AddSerial(arName, aSettings, aSerial);
}

ICommandAcceptor* AsyncStackManager::AddMaster( const std::string& arPortName, const std::string& arStackName, FilterLevel aLevel, IDataObserver* apPublisher,
											    const MasterStackConfig& arCfg)
{
	AsyncPort* pPort = this->AllocatePort(arPortName);
	Logger* pLogger = mpLogger->GetSubLogger(arStackName, aLevel);
	AsyncMasterStack* pMaster = new AsyncMasterStack(pLogger, &mTimerSrc, apPublisher, pPort->GetGroup(), arCfg);	
	this->OnAddStack(arStackName, pMaster, pPort, arCfg.link.LocalAddr);
	return pMaster->mMaster.GetCmdAcceptor();
}

IDataObserver* AsyncStackManager::AddSlave( const std::string& arPortName, const std::string& arStackName, FilterLevel aLevel, ICommandAcceptor* apCmdAcceptor,
											const SlaveStackConfig& arCfg)
{
	AsyncPort* pPort = this->AllocatePort(arPortName);
	Logger* pLogger = mpLogger->GetSubLogger(arStackName, aLevel);
	AsyncSlaveStack* pSlave = new AsyncSlaveStack(pLogger, &mTimerSrc, apCmdAcceptor, arCfg);	
	this->OnAddStack(arStackName, pSlave, pPort, arCfg.link.LocalAddr);
	return pSlave->mSlave.GetDataObserver();
}

/// Remove a port and all associated stacks
void AsyncStackManager::RemovePort(const std::string& arPortName)
{	
	AsyncPort* pPort = this->GetPort(arPortName);
	vector<string> stacks = this->StacksOnPort(arPortName);
	BOOST_FOREACH(string s, stacks) { this->SeverStack(pPort, s); }		
	mPortToPort.erase(arPortName);
	
	mScheduler.Sever(pPort->GetGroup());	// this tells the scheduler that we'll delete the group
	mTimerSrc.Post(boost::bind(&AsyncPort::Release, pPort));	
		
	// remove the physical layer from the list
	// The ports own the layers themselves, so deleting the port will delete the layer
	mMgr.Remove(arPortName); 

	this->CheckForJoin();
}

std::vector<std::string> AsyncStackManager::StacksOnPort(const std::string& arPortName)
{
	std::vector<std::string> ret;
	for(PortMap::iterator i = this->mStackToPort.begin(); i!=mStackToPort.end(); ++i) {
		if(i->second->Name() == arPortName) ret.push_back(i->first);
	}
	return ret;
}

void AsyncStackManager::RemoveStack(const std::string& arStackName)
{
	AsyncPort* pPort = this->GetPortByStackName(arStackName);
	this->SeverStack(pPort, arStackName);
	this->CheckForJoin();
}

void AsyncStackManager::SeverStack(AsyncPort* apPort, const std::string& arStackName)
{	
	mTimerSrc.Post(boost::bind(&AsyncPort::Disassociate, apPort, arStackName)); 
	mStackToPort.erase(arStackName);
}

AsyncPort* AsyncStackManager::GetPortByStackName(const std::string& arStackName)
{
	PortMap::iterator i = mStackToPort.find(arStackName);
	if(i == mStackToPort.end()) throw ArgumentException(LOCATION, "Unknown stack");
	return i->second;
}

void AsyncStackManager::Stop()
{	
	LOG_BLOCK(LEV_DEBUG, "enter stop");
	vector<string> ports = this->GetPortNames();
	BOOST_FOREACH(string s, ports) { 
		LOG_BLOCK(LEV_DEBUG, "Removing port: " << s);
		this->RemovePort(s);
		LOG_BLOCK(LEV_DEBUG, "Done removing Port: " << s);
	}
	if(mRunning) {
		LOG_BLOCK(LEV_DEBUG, "Joining on io_service thread");
		mThread.WaitForStop();
		mRunning = false;
		LOG_BLOCK(LEV_DEBUG, "Done joining");
	}
	LOG_BLOCK(LEV_DEBUG, "exit stop");
}

void AsyncStackManager::Start()
{
	if(this->NumStacks() > 0 && !mRunning) {
		mRunning = true;
		mThread.Start();
	}
}

AsyncPort* AsyncStackManager::AllocatePort(const std::string& arName)
{
	AsyncPort* pPort = this->GetPortPointer(arName);
	if(pPort == NULL) {
		PhysLayerSettings s = mMgr.GetSettings(arName);
		IPhysicalLayerAsync* pPhys = mMgr.GetLayer(arName, mService.Get());
		pPort = this->CreatePort(arName, pPhys, mpLogger->GetSubLogger(arName, s.LogLevel), s.RetryTimeout);		
	}
	return pPort;
}

AsyncPort* AsyncStackManager::GetPort(const std::string& arName)
{
	AsyncPort* pPort = this->GetPortPointer(arName);
	if(pPort == NULL) throw ArgumentException(LOCATION, "Port doesn't exist");
	return pPort;
}


AsyncPort* AsyncStackManager::CreatePort(const std::string& arName, IPhysicalLayerAsync* apPhys, Logger* apLogger, millis_t aOpenDelay)
{
	if(GetPortPointer(arName) != NULL) throw ArgumentException(LOCATION, "Port already exists");
	AsyncPort* pPort = new AsyncPort(arName, apLogger, mScheduler.NewGroup(), &mTimerSrc, apPhys, aOpenDelay);
	mPortToPort[arName] = pPort;
	return pPort;
}

AsyncPort* AsyncStackManager::GetPortPointer(const std::string& arName)
{
	PortMap::iterator i = mPortToPort.find(arName);
	return (i==mPortToPort.end()) ? NULL : i->second;
}

void AsyncStackManager::Run()
{
	size_t num = 0;

	do {
		try {			
			num = mService.Get()->run();
		}		
		catch(const std::exception& ex) {
			std::cout << "Unhandled exception: " << ex.what() << std::endl;
		}
	}
	while(num > 0);

	mService.Get()->reset();
}

void AsyncStackManager::OnAddStack(const std::string& arStackName, AsyncStack* apStack, AsyncPort* apPort, uint_16_t aAddress)
{	
	// marshall the linking to the io_service
	mStackToPort[arStackName] = apPort; //map the stack to a portname
	mTimerSrc.Post(boost::bind(&AsyncPort::Associate, apPort, arStackName, apStack, aAddress)); 
	if(!mRunning && mRunASIO) {
		mRunning = true;
		mThread.Start();
	}
}

void AsyncStackManager::CheckForJoin()
{	
	if(mRunning && this->NumStacks() == 0) {
		LOG_BLOCK(LEV_DEBUG, "Check For join: joining on io_service thread");
		mThread.WaitForStop();	//join on the thread, ASIO will exit when there's no more work to be done
		mRunning = false;		
		LOG_BLOCK(LEV_DEBUG, "Check For join: complete");
	}	
}


}}
