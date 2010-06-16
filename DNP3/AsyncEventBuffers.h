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
#ifndef __ASYNC_EVENT_BUFFERS_H_
#define __ASYNC_EVENT_BUFFERS_H_


#include "BufferSetTypes.h"
#include "AsyncEventBufferBase.h"

#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <limits>

namespace apl { namespace dnp {

	/** Event buffer that only stores one event per Index:

		Note: EventType must have the public property mIndex.
		*/
	template <class EventType>
	class AsyncSingleEventBuffer : public AsyncEventBufferBase<EventType, IndexSet< EventType > >
	{
	public:

		AsyncSingleEventBuffer(size_t aMaxEvents);

		void _Update(const EventType& arEvent);
	};

	/** Event buffer that stores all changes to all points in the order. */
	template <class EventType>
	class AsyncTimeOrderedEventBuffer : public AsyncEventBufferBase<EventType, TimeMultiSet< EventType > >
	{
	public:

		AsyncTimeOrderedEventBuffer(size_t aMaxEvents);
	};

	/** Event buffer that stores all changes to all points in the order. */
	template <class EventType>
	class AsyncInsertionOrderedEventBuffer : public AsyncEventBufferBase<EventType, InsertionOrderSet2< EventType > >
	{
	public:

		AsyncInsertionOrderedEventBuffer(size_t aMaxEvents);
	};


	template <class EventType>
	AsyncSingleEventBuffer<EventType> :: AsyncSingleEventBuffer(size_t aMaxEvents) :
	AsyncEventBufferBase< EventType, IndexSet< EventType > >(aMaxEvents)
	{}

	template <class EventType>
	AsyncTimeOrderedEventBuffer<EventType> :: AsyncTimeOrderedEventBuffer(size_t aMaxEvents) :
	AsyncEventBufferBase <EventType, TimeMultiSet< EventType > >(aMaxEvents)
	{}

	template <class EventType>
	AsyncInsertionOrderedEventBuffer<EventType> :: AsyncInsertionOrderedEventBuffer(size_t aMaxEvents) :
	AsyncEventBufferBase<EventType, InsertionOrderSet2< EventType > >(aMaxEvents)
	{}

	template <class EventType>
	void AsyncSingleEventBuffer<EventType> :: _Update(const EventType& arEvent)
	{
		typename IndexSet< EventType >::Type::iterator i = this->mEventSet.find(arEvent);

		if(i != this->mEventSet.end() )
		{
			if(arEvent.mValue.GetTime() >= i->mValue.GetTime())
			{
				this->mEventSet.erase(i);
				this->mEventSet.insert(arEvent); //new event
			}
		}
		else
		{
			this->mEventSet.insert(arEvent); //new event
			this->mCounter.IncrCount(arEvent.mClass);
		}
	}

}} //end NS

#endif
