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
#ifndef __ASYNC_PORT_H_
#define __ASYNC_PORT_H_


#include "AsyncLinkLayerRouter.h"

#include <APL/Loggable.h>
#include <vector>

namespace apl {
	class Logger;
	class IPhysicalLayerAsync;
	class ITimerSource;
	class AsyncTaskGroup;
}

namespace apl { namespace dnp {

class AsyncStack;
class AsyncStackManager;


template <class T, class U>
static std::vector<U> GetKeys(T& arMap)
{
	std::vector<U> ret;
	for(typename T::iterator i = arMap.begin(); i != arMap.end(); ++i) { ret.push_back(i->first); }
	return ret;
}

class AsyncPort : public Loggable, public IPhysMonitor
{
	struct StackRecord
	{
		StackRecord() : pStack(NULL), mLocalAddress(0)
		{}

		StackRecord(AsyncStack* apStack, uint_16_t aLocalAddress) :
		pStack(apStack) , mLocalAddress(aLocalAddress)
		{}

		AsyncStack* pStack;
		uint_16_t mLocalAddress;
	};

	public:
	AsyncPort(const std::string& arName, Logger*, AsyncTaskGroup*, ITimerSource* apTimerSrc, IPhysicalLayerAsync*, millis_t aOpenDelay);
	~AsyncPort();


	AsyncTaskGroup* GetGroup() { return mpGroup; }
	void Associate(const std::string& arStackName, AsyncStack* apStack, uint_16_t aLocalAddress);
	void Disassociate(const std::string& arStackName);

	std::string Name() { return mName; }

	void Release();

	//Events from the router
	void OnStateChange(IPhysMonitor::State);

	private:

	std::string mName;
	AsyncLinkLayerRouter mRouter;
	AsyncTaskGroup* mpGroup;
	IPhysicalLayerAsync* mpPhys;
	bool mRelease;

	private:

	typedef std::map<std::string, StackRecord> StackMap;
	StackMap mStackMap;

};

}}


#endif

