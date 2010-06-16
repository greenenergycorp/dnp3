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
#ifndef __ASYNC_STACK_MANAGER_H_
#define __ASYNC_STACK_MANAGER_H_


#include <map>
#include <vector>

#include <APL/Loggable.h>
#include <APL/TimerSourceASIO.h>
#include <APL/Threadable.h>
#include <APL/Thread.h>
#include <APL/DataInterfaces.h>
#include <APL/IPhysMonitor.h>
#include <APL/PhysicalLayerManager.h>
#include <APL/AsyncTaskScheduler.h>
#include <APL/Lock.h>
#include <APL/IOService.h>

namespace apl {
	class IPhysicalLayerAsync;
	class Logger;
	class ICommandAcceptor;
	class IDataObserver;
}

namespace apl { namespace dnp {

class AsyncPort;
class AsyncStack;

struct SlaveStackConfig;
struct MasterStackConfig;

/**
	The interface for C++ projects for dnp3. Provides aninterface for starting/stopping
	master/slave protocol stacks. Any method may be called while the system is running.
	Methods should only be called from a single thread at at a time.
*/
class AsyncStackManager : private Threadable, private Loggable
{
	public:
		/**
			@param apLogger - Logger to use for all other loggers
			@param aAutoRun	- Stacks begin execution immediately after being added
		*/
		AsyncStackManager(Logger* apLogger, bool aAutoRun = false);
		~AsyncStackManager();

		// All the io_service marshalling now occurs here. It's now safe to add/remove while the manager is running.

		/// Adds a TCPClient port, excepts if the port already exists
		void AddTCPClient(const std::string& arName, PhysLayerSettings, const std::string& arAddr, uint_16_t aPort);

		/// Adds a TCPServer port, excepts if the port already exists
		void AddTCPServer(const std::string& arName, PhysLayerSettings, uint_16_t aPort);

		/// Adds a Serial port, excepts if the port already exists
		void AddSerial(const std::string& arName, PhysLayerSettings, SerialSettings);

		/**
			Adds a master stack - Stack will automatically start running if Start() has been called or aAutoRun == true

			@param arPortName	Unique name of the port to with which to associate the stack. Will except if port doesn't exist
			@param arStackName	Unique name of the stack. Will except if the name already exists
			@param aLevel		Log filter level to use
			@param apPublisher	Interface to callback with measurements. The callback comes from an unknown network thread and should not be blocked.
			@param arCfg		Configuration data for the master stack
			@return				Threadsafe interface to use for dispatching commands to the master.
		*/
		ICommandAcceptor* AddMaster(const std::string& arPortName,
									const std::string& arStackName,
									FilterLevel aLevel,
									IDataObserver* apPublisher,
									const MasterStackConfig& arCfg);

		/**
			Adds a slave stack - Stack will automatically start running if Start() has been called or aAutoRun == true

			@param arPortName		Unique name of the port to with which to associate the stack. Will except if port doesn't exist
			@param arStackName		Unique name of the stack. Will except if the name already exists
			@param aLevel			Log filter level to use
			@param apCmdAcceptor	Interface to callback with measurements. The callback comes from an unknown network thread and should not be blocked.
			@param arCfg			Configuration data for the slave stack
			@return					Threadsafe interface to use for writing new measurement values to the slave
		*/
		IDataObserver* AddSlave(const std::string& arPortName,
								const std::string& arStackName,
								FilterLevel aLevel,
								ICommandAcceptor* apCmdAcceptor,
								const SlaveStackConfig&);

		/// Remove a port and all associated stacks
		void RemovePort(const std::string& arPortName);

		/// Remove only a single stack
		void RemoveStack(const std::string& arStackName);

		/// @return a vector of all the stack names
		std::vector<std::string> GetStackNames();

		/// @return a vector of all the port names
		std::vector<std::string> GetPortNames();

		/// Start the thead if it isn't running
		void Start();

		/// Stop the thread, doesn't delete anything until the stack manager destructs
		void Stop();

	private:

		AsyncPort* AllocatePort(const std::string& arName);
		AsyncPort* CreatePort(const std::string& arName, IPhysicalLayerAsync* apPhys, Logger* apLogger, millis_t aOpenDelay);
		AsyncPort* GetPort(const std::string& arName);
		AsyncPort* GetPortByStackName(const std::string& arStackName);
		AsyncPort* GetPortPointer(const std::string& arName);

		void Run();

		/// Remove a stack
		void SeverStack(AsyncPort* apPort, const std::string& arStackName);

		void OnAddStack(const std::string& arStackName, AsyncStack* apStack, AsyncPort* apPort, uint_16_t aAddress);
		void CheckForJoin();

		bool mRunASIO;
		bool mRunning;
		size_t NumStacks() { return mStackToPort.size(); }

		std::vector<std::string> StacksOnPort(const std::string& arPortName);

	protected:
		IOService mService;
		TimerSourceASIO mTimerSrc;

	private:
		PhysicalLayerManager mMgr;
		AsyncTaskScheduler mScheduler;
		Thread mThread;

		typedef std::map<std::string, AsyncPort*> PortMap;
		PortMap mStackToPort;		/// maps a stack name a port instance
		PortMap mPortToPort;		/// maps a port name to a port instance

		typedef std::map<std::string, size_t> mPortCount;	/// how many stacks per port
};

}}

#endif
