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
#ifndef __RESPONSE_LOADER_H_
#define __RESPONSE_LOADER_H_


#include <APL/Types.h>
#include <APL/DataInterfaces.h>
#include <APL/Loggable.h>
#include <APL/Logger.h>

#include "CTOHistory.h"
#include "ObjectReadIterator.h"

namespace apl { namespace dnp {

class HeaderReadIterator;
class IVTODataSink;

/** Dedicated class for processing response data in the master.
*/
class ResponseLoader : Loggable
{
	public:
		ResponseLoader(Logger*, IDataObserver*);

		void Process(HeaderReadIterator&);

	private:

		void ProcessData(HeaderReadIterator&, int aGrp, int aVar);

		template <class T>
		void ReadCTO(HeaderReadIterator& arIter);

		template <class T>
		void Read(HeaderReadIterator& arIter, StreamObject<T>* apObj);

		template <class T>
		void ReadBitfield(HeaderReadIterator& arHeader);

		IDataObserver* mpPublisher;
		Transaction mTransaction;		
		CTOHistory mCTO;
};

template <class T>
void ResponseLoader::ReadCTO(HeaderReadIterator& arIter)
{
	ObjectReadIterator i = arIter.BeginRead();

	if(i.Count() != 1)
	{
		LOG_BLOCK(LEV_WARNING, "Invalid number of CTO objects");
		return;
	}

	TimeStamp_t t = static_cast<TimeStamp_t>(T::Inst()->mTime.Get(*i));
	mCTO.SetCTO(t);
}

template <class T>
void ResponseLoader::Read(HeaderReadIterator& arIter, StreamObject<T>* apObj)
{
	TimeStamp_t t(0); //base time
	if(apObj->UseCTO() && !mCTO.GetCTO(t)) {
				LOG_BLOCK(LEV_ERROR, "No CTO for relative time type " << apObj->Name());
				return;
	}

	ObjectReadIterator obj = arIter.BeginRead();
	LOG_BLOCK(LEV_INTERPRET, "Converting " << obj.Count() << " " << apObj->Name() << " To " << typeid(T).name());

	for( ; !obj.IsEnd(); ++obj) {
		size_t index = obj->Index();
		T value = apObj->Read(*obj);

		// Make sure the value has time information
		if(apObj->UseCTO()) value.SetTime(t+value.GetTime());
				
		// Make sure the value has quality information
		if(!apObj->HasQuality()) value.SetQuality(T::ONLINE);

		mpPublisher->Update(value, index);
	}
}

template <class T>
void ResponseLoader::ReadBitfield(HeaderReadIterator& arIter)
{
	Binary b; b.SetQuality(Binary::ONLINE);

	ObjectReadIterator obj = arIter.BeginRead();
	LOG_BLOCK(LEV_INTERPRET, "Converting " << obj.Count() << " " << T::Inst()->Name() << " To " << typeid(b).name());

	for(; !obj.IsEnd(); ++obj)
	{
		bool val = BitfieldObject::StaticRead(*obj, obj->Start(), obj->Index());
		b.SetValue(val);
		mpPublisher->Update(b, obj->Index());
	}
}



}}

#endif
