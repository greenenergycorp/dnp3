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
#ifndef __ASYNC_SLAVE_EVENT_BUFFER_H_
#define __ASYNC_SLAVE_EVENT_BUFFER_H_

#include <APL/DataTypes.h>
#include "AsyncEventBuffers.h"
#include "DNPDatabaseTypes.h"
#include "AsyncDatabaseInterfaces.h"

namespace apl { namespace dnp {


	/** Manager for DNP3 data events composed of per-type event buffer implementations. Events are selected based on
	classification (data type, class) and then read using an iterator system. The behavior is transactional such that
	failed deliveries put events back into the buffer.

	All selections can be limited by a desired event count.
	*/
class AsyncSlaveEventBuffer : public IAsyncEventBuffer
{
	public:
		AsyncSlaveEventBuffer(size_t aMaxBinary, size_t aMaxAnalog, size_t aMaxCounter);

		void Update(const Binary& arEvent, PointClass aClass, size_t aIndex);
		void Update(const Analog& arEvent, PointClass aClass, size_t aIndex);
		void Update(const Counter& arEvent, PointClass aClass, size_t aIndex);

		size_t NumSelected(DataTypes aType);
		size_t NumSelected();

		size_t NumType(DataTypes aType);

		void Begin(BinaryEventIter& arIter)		{ arIter = mBinaryEvents.Begin(); }
		void Begin(AnalogEventIter& arIter)		{ arIter = mAnalogEvents.Begin(); }
		void Begin(CounterEventIter& arIter)	{ arIter = mCounterEvents.Begin(); }

		bool IsOverflow();
		bool HasClassData(PointClass aClass);
		bool HasEventData();

		size_t Select(DataTypes aType, PointClass aClass, size_t aMaxEvent = std::numeric_limits<size_t>::max());
		size_t Select(PointClass aClass, size_t aMaxEvent = std::numeric_limits<size_t>::max());

		/// Put all selected events back into the event buffer
		void Deselect();
		void ClearWritten();

	private:

		AsyncTimeOrderedEventBuffer<BinaryEvent>		mBinaryEvents;	/// Multiple events for the same time, ordered by time of occurrence
		AsyncSingleEventBuffer<AnalogEvent>				mAnalogEvents;	/// Single event per point, previous events overridden
		AsyncSingleEventBuffer<CounterEvent>			mCounterEvents; /// Single event per point, previous events overridden

		bool mChange;
};

}}

#endif
