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
#include "AsyncTaskScheduler.h"

#include "AsyncTaskGroup.h"
#include "AsyncTaskBase.h"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <vector>

using namespace std;
using namespace boost;
using namespace boost::posix_time;

namespace apl {

AsyncTaskScheduler::AsyncTaskScheduler(ITimerSource* apTimerSrc, ITimeSource* apTimeSrc) : 
mpTimerSrc(apTimerSrc),
mpTimeSrc(apTimeSrc)
{

}

AsyncTaskScheduler::~AsyncTaskScheduler()
{	
	BOOST_FOREACH(AsyncTaskGroup* p, mGroupSet) { delete p; }	
}

AsyncTaskGroup* AsyncTaskScheduler::NewGroup()
{
	AsyncTaskGroup* pGroup = new AsyncTaskGroup(mpTimerSrc, mpTimeSrc);
	mGroupSet.insert(pGroup);
	return pGroup;
}

AsyncTaskGroup* AsyncTaskScheduler::Sever(AsyncTaskGroup* apGroup)
{
	GroupSet::iterator i = mGroupSet.find(apGroup);
	if( i != mGroupSet.end() ) {				
		AsyncTaskGroup* pGroup = *i;
		mGroupSet.erase(i);
		return pGroup;
	}
	else return NULL;
}

void AsyncTaskScheduler::Release(AsyncTaskGroup* apGroup)
{
	delete Sever(apGroup);	
}

} //end ns
