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
#ifndef __ASYNC_EVENT_BUFFER_BASE_H_
#define __ASYNC_EVENT_BUFFER_BASE_H_


#include "ClassCounter.h"
#include "EventTypes.h"

namespace apl { namespace dnp {


	/** Base class for the AsyncEventBuffer classes (with templating and
		virtual function for Update to alter event storage behavior)

		Single-threaded for asynchronous/event-based model.
	*/
	template <class EventType, class SetType>
	class AsyncEventBufferBase
	{
	public:
		AsyncEventBufferBase(size_t aMaxEvents, bool aDropFirst = true);
		virtual ~AsyncEventBufferBase() {}

		/**
			Adds an event to the buffer, tracking insertion order. Calls
			_Update which can be overriden to do special types of insertion.

			@param arVal Event update to add the to the buffer
			@param aClass Class of the measurement
			@param aIndex Index of the measurement
		*/
		void Update(const typename EventType::MeasType& arVal, PointClass aClass, size_t aIndex);

		bool HasClassData(PointClass aClass) { return mCounter.GetNum(aClass) > 0; }

		/**
			@param aClass PointClass to select (i.e. PC_CLASS_1, 2, 3) into the selection buffer
			@param aMaxEvent Maximum number of events to select
			@return Number of events selected
		*/
		size_t Select(PointClass aClass, size_t aMaxEvent = std::numeric_limits<size_t>::max());

		/**
			Transfer any unwritten events back into the event set

			@return Number of events returned to the event set
		*/
		size_t Deselect();


		/**
			Clear events flagged with mWritten = true from the selection buffer

			@return Number of events cleared
		*/
		size_t ClearWrittenEvents();

		typename EvtItr< EventType >::Type Begin();

		size_t NumSelected() { return mSelectedEvents.size(); }
		size_t NumUnselected() { return mEventSet.size(); }
		size_t Size() { return mSelectedEvents.size() + mEventSet.size(); }

		bool IsOverflown();

	protected:

		bool IsFull() { return NumUnselected() >= M_MAX_EVENTS; }


		void Update(EventType& arEvent, bool aNewValue);

		/**
			Overridable NVII function called by Update. Default implementation does
			a simple insert into mEventSet


			@param arEvent Event update to add the to the buffer
		*/
		virtual void _Update(const EventType& arEvent);

		ClassCounter mCounter;		/// counter for class events
		const size_t M_MAX_EVENTS;	/// max number of events to accept before setting overflow
		size_t mSequence;			/// used to track the insertion order of events into the buffer
		bool mIsOverflown;			/// flag that tracks when an overflow occurs
		bool mDropFirst;			/// on overflow, do we drop the first or last element?

		/// vector to hold all selected events until they are cleared or failed back into mEventSet
		typename std::vector< EventType > mSelectedEvents;

		/// store to keep and order incoming events
		typename SetType::Type mEventSet;
	};


	template <class EventType, class SetType>
	AsyncEventBufferBase <EventType, SetType> :: AsyncEventBufferBase(size_t aMaxEvents, bool aDropFirst) :
	M_MAX_EVENTS(aMaxEvents),
	mSequence(0),
	mIsOverflown(false),
	mDropFirst(aDropFirst)
	{

	}

	template <class EventType, class SetType>
	void AsyncEventBufferBase<EventType,SetType> :: Update(const typename EventType::MeasType& arVal, PointClass aClass, size_t aIndex)
	{
		EventType evt(arVal, aClass, aIndex);
		this->Update(evt, true);

		if(this->NumUnselected() > M_MAX_EVENTS) { //we've overflown and we've got to drop an event
			mIsOverflown = true;
			if(mDropFirst) mEventSet.erase(mEventSet.begin());
			else mEventSet.erase((++mEventSet.rbegin()).base());
		}
	}

	template <class EventType, class SetType>
	void AsyncEventBufferBase<EventType,SetType> :: Update(EventType& arEvent, bool aNewValue)
	{
		// prevents numerical overflow of the increasing sequence number
		if(this->Size() == 0) mSequence = 0;

		if(aNewValue) arEvent.mSequence = mSequence++;

		this->_Update(arEvent); // call the overridable NVII function
	}

	template <class EventType, class SetType>
	void AsyncEventBufferBase<EventType, SetType> :: _Update(const EventType& arEvent)
	{
		this->mCounter.IncrCount(arEvent.mClass);
		this->mEventSet.insert(arEvent);
	}

	template <class EventType, class SetType>
	size_t AsyncEventBufferBase<EventType, SetType> :: Deselect()
	{
		size_t num = mSelectedEvents.size();

		// put selected events back into the event buffer
		for(size_t i=0; i < num; i++) this->Update(mSelectedEvents[i], false);

		mSelectedEvents.clear();

		return num;
	}

	template <class EventType, class SetType>
	bool AsyncEventBufferBase <EventType, SetType> :: IsOverflown()
	{
		// if the buffer previously overflowed, but is no longer full, reset the flag
		if(mIsOverflown && this->Size() < M_MAX_EVENTS) mIsOverflown = false;

		return mIsOverflown;
	}

	template <class EventType, class SetType>
	size_t AsyncEventBufferBase<EventType, SetType> :: ClearWrittenEvents()
	{
		typename std::vector<EventType>::iterator itr = this->mSelectedEvents.begin();

		size_t num = 0;
		while(itr != this->mSelectedEvents.end() && itr->mWritten) {
			++itr;
			++num;
		}

		this->mSelectedEvents.erase(this->mSelectedEvents.begin(), itr);
		return num;
	}

	template <class EventType, class SetType>
	typename EvtItr< EventType >::Type AsyncEventBufferBase<EventType, SetType> :: Begin()
	{
		return mSelectedEvents.begin();
	}

	template <class EventType, class SetType>
	size_t AsyncEventBufferBase <EventType, SetType> :: Select(PointClass aClass, size_t aMaxEvent)
	{
		typename SetType::Type::iterator i = mEventSet.begin();

		size_t count = 0;

		while( i != mEventSet.end() && count < aMaxEvent)
		{
			if( ( i->mClass & aClass) != 0 )
			{
				mCounter.DecrCount(i->mClass);
				mSelectedEvents.push_back(*i);
				mEventSet.erase(i++);
				++count;
				mSelectedEvents.back().mWritten = false;
			}
			else ++i;
		}

		return count;
	}

}} //end NS

#endif
