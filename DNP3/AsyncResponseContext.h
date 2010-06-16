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
#ifndef __ASYNC_RESPONSE_CONTEXT_H_
#define __ASYNC_RESPONSE_CONTEXT_H_

#include <APL/Loggable.h>

#include "DNPDatabaseTypes.h"
#include "APDU.h"
#include "AsyncDatabase.h"
#include "AsyncSlaveEventBuffer.h"
#include "ClassMask.h"

#include <queue>
#include <boost/function.hpp>


namespace apl { namespace dnp {

class AsyncDatabase;
class AsyncSlaveEventBuffer;
class ObjectBase;
class SlaveResponseTypes;

template <class T>
struct WriteFunc
{
	typedef boost::function<void (byte_t*, const T& arValue, size_t aIndex)> Type;
};

/**
 Builds and tracks the state of responses. Interprets FC_READ requests or can be prompted for an unsolicited response fragment.

 Coordinates the AsyncDatabase and AsyncSlaveEventBuffer.
*/
class AsyncResponseContext : public Loggable
{
	enum Mode {
		UNDEFINED,
		SOLICITED,
		UNSOLICITED
	};

	public:
	AsyncResponseContext(Logger*, AsyncDatabase*, SlaveResponseTypes* apRspTypes, size_t aMaxBinary, size_t aMaxAnalog, size_t aMaxCounter);

	Mode GetMode() { return mMode; }

	IAsyncEventBuffer* GetBuffer() { return &mBuffer; }

	/// Setup the response context with a new read request
	IINField Configure(const APDU& arRequest);

	/// Configure the APDU with response data for the next fragment
	void LoadResponse(APDU&);

	bool HasEvents(ClassMask aMask);

	/** Configure the APDU with a FIR/FIN unsol packet based on
		current state of the event buffer
	*/
	bool LoadUnsol(APDU&, const IINField& arIIN, ClassMask aMask);

	/// @return TRUE is all of the response data has already been written
	bool IsComplete() { return IsEmpty(); }

	/// Reset the state of the object to the initial state
	void Reset();

	/// Tell the buffer to reset written events
	void ClearWritten();

	/// Clear written events and reset the state of the object
	void ClearAndReset();


	private:

	// configure the state for unsol, return true of events exist
	bool SelectUnsol(ClassMask aMask);


	AsyncSlaveEventBuffer mBuffer;

	Mode mMode;

	/// @return TRUE if all of the data has been written
	bool LoadStaticData(APDU&);


	/** @param arEventsLoaded Set to true if events were written to the APDU
		@param arAPDU Events are loaded into this fragment
		@return TRUE if all of the data has been written
	*/
	bool LoadEventData(APDU& arAPDU, bool& arEventsLoaded);

	void FinalizeResponse(APDU&, bool aHasEventData, bool aFIN);
	bool IsEmpty();

	bool IsStaticEmpty();
	bool IsEventEmpty();

	void AddIntegrityPoll();

	//bool WriteCTO(const TimeStamp_t& arTime, APDU& arAPDU);

	AsyncDatabase* mpDB;				/// Pointer to the database for static data
	bool mFIR;
	bool mFIN;
	SlaveResponseTypes* mpRspTypes;

	IINField mTempIIN;

	template <class T>
	struct IterRecord
	{
		IterRecord() : pObject(NULL) {}

		typename StaticIter<T>::Type first;				/// Begining of iteration
		typename StaticIter<T>::Type last;				/// Last element of iteration
		StreamObject<typename T::MeasType>* pObject;	/// Type to use to write
	};

	template<class T>
	struct EventRequest
	{
		EventRequest(const StreamObject<T>* apObj, size_t aCount = std::numeric_limits<size_t>::max()) :
		pObj(apObj),
		count(aCount)
		{}

		const StreamObject<T>* pObj;	/// Type to use to write
		size_t count;					/// Number of events to read

	};

	typedef std::deque< IterRecord<BinaryInfo> >			BinaryIterQueue;
	typedef std::deque< IterRecord<AnalogInfo> >			AnalogIterQueue;
	typedef std::deque< IterRecord<CounterInfo> >			CounterIterQueue;
	typedef std::deque< IterRecord<ControlStatusInfo> >		ControlIterQueue;
	typedef std::deque< IterRecord<SetpointStatusInfo> >	SetpointIterQueue;

	typedef std::deque< EventRequest<Binary> > BinaryEventQueue;
	typedef std::deque< EventRequest<Analog> > AnalogEventQueue;
	typedef std::deque< EventRequest<Counter> > CounterEventQueue;

	//these queues track what static point ranges were requested so that we can split the response over multiple fragments
	BinaryIterQueue mStaticBinaries;
	AnalogIterQueue mStaticAnalogs;
	CounterIterQueue mStaticCounters;
	ControlIterQueue mStaticControls;
	SetpointIterQueue mStaticSetpoints;

	//these queues track what events have been requested
	BinaryEventQueue mBinaryEvents;
	AnalogEventQueue mAnalogEvents;
	CounterEventQueue mCounterEvents;

	template <class T>
	void AddIntegrity(std::deque< IterRecord<T> >& arQueue, StreamObject<typename T::MeasType>* apObject);

	template <class T>
	bool LoadEvents(APDU& arAPDU, std::deque< EventRequest<T> >& arQueue, bool& arEventsLoaded);

	bool LoadStaticBinaries(APDU&);
	bool LoadStaticAnalogs(APDU&);
	bool LoadStaticCounters(APDU&);
	bool LoadStaticControlStatii(APDU&);
	bool LoadStaticSetpointStatii(APDU&);

	//wrappers that select the event buffer and add to the event queues
	void SelectEvents(PointClass aClass, size_t aNum = std::numeric_limits<size_t>::max());
	template <class T>
	size_t SelectEvents(PointClass aClass, const StreamObject<T>* apObj, std::deque< EventRequest<T> >& arQueue, size_t aNum = std::numeric_limits<size_t>::max());

	// templated function for writing APDUs
	/*template <class T>
	bool IterateContiguous(IterRecord<T>& arIters, APDU& arAPDU, typename WriteFunc<typename T::MeasType>::Type& arWriter);
*/
	template <class T>
	bool IterateContiguous(IterRecord<T>& arIters, APDU& arAPDU);

	// T is the event type
	template <class T>
	size_t IterateIndexed(EventRequest<T>& arIters, typename EvtItr< EventInfo<T> >::Type& arIter, APDU& arAPDU);

	template <class T>
	size_t IterateCTO(const StreamObject<T>* apObj, size_t aCount, typename EvtItr< EventInfo<T> >::Type& arIter, APDU& arAPDU);

	template <class T>
	size_t CalcPossibleCTO(typename EvtItr< EventInfo<T> >::Type aIter, size_t aMax);
};

template <class T>
size_t AsyncResponseContext::SelectEvents(PointClass aClass, const StreamObject<T>* apObj, std::deque< EventRequest<T> >& arQueue, size_t aNum)
{
	size_t num = mBuffer.Select(T::MeasEnum, aClass, aNum);
	if(num > 0) {
		EventRequest<T> r(apObj, aNum);
		arQueue.push_back(r);
	}

	return num;
}

template <class T>
void AsyncResponseContext::AddIntegrity(std::deque< IterRecord<T> >& arQueue, StreamObject<typename T::MeasType>* apObject)
{
	size_t num = mpDB->NumType(T::MeasType::MeasEnum);
	if(num > 0)
	{
		IterRecord<T> record;
		mpDB->Begin(record.first); record.last = record.first + (num-1);
		record.pObject = apObject;
		arQueue.push_back(record);
	}
}

template <class T>
bool AsyncResponseContext::LoadEvents(APDU& arAPDU, std::deque< EventRequest<T> >& arQueue, bool& arEventsLoaded)
{
	typename EvtItr< EventInfo<T> >::Type itr;
	mBuffer.Begin(itr);
	size_t remain = mBuffer.NumSelected(T::MeasEnum);

	while(arQueue.size() > 0) {
		EventRequest<T>& r = arQueue.front();					// how many were requested

		if(r.count > remain) r.count = remain;

		size_t written = r.pObj->UseCTO() ? this->IterateCTO<T>(r.pObj, r.count, itr, arAPDU) : this->IterateIndexed<T>(r, itr, arAPDU);
		remain -= written;

		if(written > 0) arEventsLoaded = true;					// at least 1 event was loaded

		if(written == r.count) arQueue.pop_front();				// all events written, we're done with this request
		else { //incomplete write
			r.count -= written;
			return false;
		}
	}

	return true;	// the queue has been exhausted on this iteration
}

/*
template <class T>
bool AsyncResponseContext::IterateContiguous(IterRecord<T>& arIters, APDU& arAPDU, typename WriteFunc<typename T::MeasType>::Type& arWriter)
{
	size_t start = arIters.first->mIndex;
	size_t stop = arIters.last->mIndex;

	ObjectWriteIterator owi = arAPDU.WriteContiguous(arIters.pObject, start, stop);

	for(size_t i=start; i<=stop; ++i)
	{
		if(owi.IsEnd()) return false; // out of space in the fragment
		arWriter(*owi, arIters.first->mValue, i);
		++arIters.first; //increment the iterators
		++owi;
	}

	return true;
}*/

template <class T>
bool AsyncResponseContext::IterateContiguous(IterRecord<T>& arIters, APDU& arAPDU)
{
	size_t start = arIters.first->mIndex;
	size_t stop = arIters.last->mIndex;
	StreamObject<typename T::MeasType>* pObj = arIters.pObject;

	ObjectWriteIterator owi = arAPDU.WriteContiguous(arIters.pObject, start, stop);

	for(size_t i=start; i<=stop; ++i)
	{
		if(owi.IsEnd()) return false; // out of space in the fragment
		pObj->Write(*owi, arIters.first->mValue);
		++arIters.first; //increment the iterators
		++owi;
	}

	return true;
}




// T is the point info type
template <class T>
size_t AsyncResponseContext::IterateIndexed(EventRequest<T>& arRequest, typename EvtItr< EventInfo<T> >::Type& arIter, APDU& arAPDU)
{
	size_t max_index = mpDB->MaxIndex(T::MeasEnum);
	IndexedWriteIterator write = arAPDU.WriteIndexed(arRequest.pObj, arRequest.count, max_index);

	for(size_t i = 0; i < arRequest.count; ++i)
	{
		if(write.IsEnd()) return i;										//that's all we can get into this fragment

		write.SetIndex(arIter->mIndex);
		arRequest.pObj->Write(*write, arIter->mValue);					// do the write
		arIter->mWritten = true;										// flag it as written
		++arIter;														// advance the read iterator
		++write;														// advance the write iterator
	}

	return arRequest.count; // all requested events were written
}

template <class T>
size_t AsyncResponseContext::CalcPossibleCTO(typename EvtItr< EventInfo<T> >::Type aIter, size_t aMax)
{
	millis_t start = aIter->mValue.GetTime();

	size_t num = 0;
	while(num < aMax) {
		if((aIter->mValue.GetTime() - start) > UInt16LE::Max) break;
		++num;
		++aIter;
	}

	return num;
}

// T is the point info type
template <class T>
size_t AsyncResponseContext::IterateCTO(const StreamObject<T>* apObj, size_t aCount, typename EvtItr< EventInfo<T> >::Type& arIter, APDU& arAPDU)
{
	size_t max_index = mpDB->MaxIndex(T::MeasEnum);

	millis_t start = arIter->mValue.GetTime();

	// first try to write a CTO object for the first value that we're pushing
	ObjectWriteIterator itr = arAPDU.WriteContiguous(Group51Var1::Inst(), 0, 0, QC_1B_CNT);
	if(itr.IsEnd()) return 0;
	else Group51Var1::Inst()->mTime.Set(*itr, start);

	// predetermine how many results you're going to be able to fit given the time differences
	size_t num = this->CalcPossibleCTO<T>(arIter, aCount);
	IndexedWriteIterator write = arAPDU.WriteIndexed(apObj, num, max_index); //start the object write

	for(size_t i = 0; i < num; ++i)
	{
		if(write.IsEnd()) return i;										// that's all we can get into this fragment

		T tmp = arIter->mValue;											// make a copy and adjust the time
		tmp.SetTime(tmp.GetTime()-start);

		write.SetIndex(arIter->mIndex);
		apObj->Write(*write, tmp);										// do the write, with the tmp
		arIter->mWritten = true;										// flag it as written
		++arIter;														// advance the read iterator
		++write;														// advance the write iterator
	}

	if(num == aCount) return num;
	else return num + this->IterateCTO(apObj, aCount - num, arIter, arAPDU); //recurse, and do another CTO header
}


}}

#endif

