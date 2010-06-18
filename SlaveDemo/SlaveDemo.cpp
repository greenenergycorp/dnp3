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

#include <APL/Log.h>
#include <APL/IOServiceThread.h>
#include <DNP3/AsyncStackManager.h>
#include <APL/CommandQueue.h>
#include <APL/FlexibleDataObserver.h>
#include <APL/IPhysicalLayerAsync.h>
#include <APL/IOService.h>
#include <APL/TimerSourceASIO.h>
#include <DNP3/SlaveStackConfig.h>
#include <APL/PostingNotifierSource.h>
#include <APL/LogToStdio.h>

#include "SlaveDemo.h"

using namespace std;
using namespace apl;
using namespace apl::dnp;


/**
	This class demonstrates how to receive and handle commands from a
	communication master. Upon construction, we tell the stack to notify us
	(by calling OnCommandNotify()) when any command is received. In the notify method we
	then ask the ICommandSource to send this object the received commands using
	the ICommandHandler interface.
 */
class SlaveDemoBase : public ICommandHandler, public IOServiceThread
{
public:
	SlaveDemoBase(ICommandSource* apCommandSource, IOService* apService) 
		: IOServiceThread(apService->Get()),
			mTimerSource(apService->Get()),
			mpCommandNotifier(NULL),
			mpCommandSource(apCommandSource)
	{
		// Start a timer that will do nothing but keep the boost asio service from returning
		mpInfiniteTimer = mTimerSource.StartInfinite(boost::bind(&SlaveDemoBase::Timeout, this));

		// Register the event call marshalling to call OnCommandNotify when controls arrive
		mpCommandNotifier = mPostSource.Get(boost::bind(&SlaveDemoBase::OnCommandNotify, this), &mTimerSource); 

		// Make us the one who is notified of incomming commands.
		mpCommandSource->SetNotifier(mpCommandNotifier);
	}
	
	
	void Shutdown() 
	{
		mpInfiniteTimer->Cancel();
		this->Stop();
	}

private:

	// Keep executing commands until ExecuteCommand returns false,
	// meaning we've consumed all of the commands.
	//
	// ExecuteCommand grabs a command off the queue and calls the ICommandHandler
	// interface methods (implemented in SlaveDemoApp, below). It waits for the 
	// command to finish before returning.
	//
	// OnCommandNotify has been marshalled to the application thread.
	//
	void OnCommandNotify()
	{
		while(mpCommandSource->ExecuteCommand(this));
	}


	// Placeholder method for the boost timer system.
	void Timeout() {}

	PostingNotifierSource mPostSource;	//!< bridge between Notifiers and IO services that marshalls method calls between threads
	TimerSourceASIO mTimerSource;		//!< boost timer source, interface into the io service system
	INotifier* mpCommandNotifier;		//!< event marshalling notifier for the command source
	ITimer* mpInfiniteTimer;			//!< timer used to keep the boost io service alive

	
	ICommandSource* mpCommandSource; //!< The source for getting and executing commands.
};


/** 
	This class demonstrates application logic, using APL scada types to respond to controls
	by updating data. In addition to the ICommandSource/ICommandHandler system, the
	IDataObserver interface gives push capability to the stack's database.
*/
class SlaveDemoApp : public SlaveDemoBase
{
public:
	SlaveDemoApp(IDataObserver* apDataObserver, ICommandSource* apCommandSource, IOService* apService)
		: SlaveDemoBase(apCommandSource, apService),
		mCountSetPoints(0),
		mCountBinaryOutput(0),
		mpDataObserver(apDataObserver)
	{}

	/**
		The master is requesting that we set a point to the specified value.
		This is where the user code will do some work. In this example, we'll
		just update the slave database.
		After we return from this method, the master will see the updates
		however it has requested. The master could be configured to do
		polling or the master may have directed the slave to send all
		updates unsolicited. Regardless, the effect is that the commands
		are echoed back to the master.
	 */
	apl::CommandStatus HandleControl(apl::Setpoint& aControl, size_t aIndex){
		std::cout << "Received " << aControl.ToString() << " on index: " << aIndex << std::endl;

		// In this example, we'll echo the control back to the master along
		// with an additional counter.


		// Create a control point that has the same value as the one we were
		// given from the master. Configure it with the current time and good
		// quality
		//
		apl::Analog a;
		a.SetQuality(apl::AQ_ONLINE); a.SetToNow();
		a.SetValue(aControl.GetValue());

		// Create an additional counter to let the master know how many setpoints
		// we receieved.
		//
		apl::Counter c;
		c.SetQuality(apl::CQ_ONLINE); c.SetToNow();
		c.SetValue(++mCountSetPoints);

		{
			// We would like all updates to be sent at one time. To do this, we
			// construct a Transaction object. When the Transaction object goes
			// out of scope, all updates will be sent in one block.
			//
			apl::Transaction t(mpDataObserver);

			// Push the prepared datapoints to the database of this slave. After
			// we return from this method, the master will see the updates however
			// it has requested. The master could be configured to do polling or
			// the master may have directed the slave to send all updates
			// unsolicited.
			//
			mpDataObserver->Update(a, aIndex);
			mpDataObserver->Update(c, 0);
		}

		// When this method returns, the stack notifies the master whether this
		// control point request was successfull or not. See , if you wanted to
		// indicate a failure of some apl::CommandStatus for other status codes.
		return apl::CS_SUCCESS;
	}


	/*
		The master is requesting that we set a point to the specified value.
		This is where the user code will do some work. In this example, we'll
		just update the slave database.

		After we return from this method, the master will see the updates
		however it has requested. The master could be configured to do
		polling or the master may have directed the slave to send all
		updates unsolicited. Regardless, the effect is that the commands
		are echoed back to the master.
	 */
	apl::CommandStatus HandleControl(apl::BinaryOutput& aControl, size_t aIndex){
		std::cout << "Received " << aControl.ToString() << " on index: " << aIndex << std::endl;
	
		// In this example, we'll echo the control back to the master along
		// with an additional counter. See the above method for more
		// documentation.

		apl::Binary b;
		b.SetQuality(apl::BQ_ONLINE); b.SetToNow();

		// set the binary to ON if the command  code was LATCH_ON, otherwise set it off (LATCH_OFF)
		b.SetValue(aControl.GetCode() == apl::CC_LATCH_ON);

		apl::Counter c;
		c.SetQuality(apl::CQ_ONLINE); c.SetToNow();

		// count how many BinaryOutput commands we recieve
		c.SetValue(++mCountBinaryOutput);

		{
			apl::Transaction t(mpDataObserver);
			mpDataObserver->Update(b, aIndex);
			mpDataObserver->Update(c, 1);
		}
		return apl::CS_SUCCESS;
	}

private:
	int mCountSetPoints;    //!< count how many setpoints we recieve to demonstrate counters
	int mCountBinaryOutput; //!< count how many binary controls we recieve to demonstrate counters

	IDataObserver* mpDataObserver;  //!< The data sink for updating the slave database.
};


int runDemo(int argc, char* argv[])
{
	// Create a log object for the stack to use. 
	EventLog log;
	log.AddLogSubscriber(LogToStdio::Inst());	// print all log entries to standard out

	// Specify a FilterLevel for the stack/physical layer to use. Log statements with a lower priority
	// will not be logged.
	const FilterLevel LOG_LEVEL = LEV_INFO;

	// Individual components are given Logger objects to push log entries to. 
	Logger* pStackLogger = log.GetLogger(LOG_LEVEL, "mgr");

	// The AsyncStackManager object instantiates master/slave DNP stacks, as well as their physical layers.
	AsyncStackManager mgr(pStackLogger);

	// Physical layers configured using the PhysLayerSettings struct, which is a connection timeout and a log level.
	const size_t PHYS_TIMEOUT_MS = 3000;
	PhysLayerSettings plSet(LOG_LEVEL, PHYS_TIMEOUT_MS);
	
	// Using the configuration struct, the stack manager will instantiate a Boost ASIO TCP/IP server connection.
	const string SLAVE_TCP = "server";	// Physical layer name is referenced by the stack configuration
	const size_t SLAVE_PORT = 4999;		
	mgr.AddTCPServer(SLAVE_TCP, plSet, SLAVE_PORT);

	// The SlaveStackConfig struct is composed of the various configuration structs necessary to create a slave stack.
	// In this case the link layer is set to slave mode, and the link addressing is configured.
	SlaveStackConfig stackConfig;
	stackConfig.link.IsMaster = false;
	stackConfig.link.LocalAddr = 1;
	stackConfig.link.RemoteAddr = 100;

	// The DeviceTemplate struct specifies the structure of the slave's database, as well as the index range of controls
	// and setpoints it accepts. 
	DeviceTemplate device(5, 5, 5, 5, 5, 5, 5);
	stackConfig.device = device;

	// The CommandQueue object serves as an inter-thread cache of control/setpoint events. It implements the ICommandAcceptor
	// interface, which is used by the slave to push controls and setpoints to the client.
	CommandQueue commandQueue;

	// Using configuration info, and providing a command acceptor, the stack manager is told to instantiate a DNP slave.
	// An IDataObserver* interface is returned, which serves as the client interface to the slave database.
	IDataObserver* pDataObserver = mgr.AddSlave(SLAVE_TCP, "slave", LOG_LEVEL, &commandQueue, stackConfig);
	
	// create the demo class that handles commands and demonstrates how to publish data
	//CommandHandlerExample cp(&commandQueue, pDataObserver);
	IOService appService;
	SlaveDemoApp app(pDataObserver, &commandQueue, &appService);

	// start the stack manager (this starts up 1 or more threads that handle the communications)
	// when the object is destructed it automatically shuts down its threads and cleans up
	mgr.Start();

	// Start the example application.
	app.Run();

	return 0;
}
