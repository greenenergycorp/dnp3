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
#ifndef __FLEXIBLE_DATA_OBSERVER_H_
#define __FLEXIBLE_DATA_OBSERVER_H_


#include "DataInterfaces.h"
#include "Lock.h"
#include "SubjectBase.h"
#include "Util.h"

#include <iostream>
#include <map>

namespace apl
{
	/** Simple data obsever that stores the current value of anything it receives. SubjectBase implictly
	notifies observers of any updates.

	Check functionality allows it to be used for testing.
	*/
	class FlexibleDataObserver : public apl::IDataObserver, public SubjectBase<SigLock>
	{
		public:

			FlexibleDataObserver();

			template <typename T>
			struct PointMap
			{ typedef std::map<size_t, T> Type; };
			
			// allow direct access to the maps
			PointMap<Binary>::Type mBinaryMap;
			PointMap<Analog>::Type mAnalogMap;
			PointMap<Counter>::Type mCounterMap;
			PointMap<ControlStatus>::Type mControlStatusMap;
			PointMap<SetpointStatus>::Type mSetpointStatusMap;

			bool Check(bool aValue, BinaryQuality aQuality, size_t aIndex) 
			{ 
				byte_t qual = aQuality;
				if(aValue) qual |= BQ_STATE;
				return Check<Binary, bool>(mBinaryMap, aValue, qual, aIndex);
			}
			bool Check(int_32_t aValue, AnalogQuality aQuality, size_t aIndex)
			{ return Check<Analog, int_32_t>(mAnalogMap, aValue, aQuality, aIndex); }

			bool Check(double aValue, AnalogQuality aQuality, size_t aIndex)
			{
				PointMap<Analog>::Type::iterator i = mAnalogMap.find(aIndex);
				if(i == mAnalogMap.end()) return false;
				if(i->second.GetQuality() != aQuality) return false;
				return apl::FloatEqual(aValue, i->second.GetValue());				
			}


			bool Check(uint_32_t aValue, CounterQuality aQuality, size_t aIndex)
			{ return Check<Counter, uint_32_t>(mCounterMap, aValue, aQuality, aIndex); }
			bool Check(bool aValue, ControlQuality aQuality, size_t aIndex)
			{ 
				byte_t qual = aQuality;
				if(aValue) qual |= TQ_STATE;
				return Check<ControlStatus, bool>(mControlStatusMap, aValue, qual, aIndex); 
			}
			bool Check(int_32_t aValue, SetpointQuality aQuality, size_t aIndex)
			{ return Check<SetpointStatus, int_32_t>(mSetpointStatusMap, aValue, aQuality, aIndex); }

			bool Check(bool aValue, BinaryQuality aQuality, size_t aIndex, TimeStamp_t aTime) 
			{ 
				byte_t qual = aQuality;
				if(aValue) qual |= BQ_STATE;
				return Check<Binary, bool>(mBinaryMap, aValue, qual, aTime, aIndex);
			}
			bool Check(int_32_t aValue, AnalogQuality aQuality, size_t aIndex, TimeStamp_t aTime)
			{ return Check<Analog, int_32_t>(mAnalogMap, aValue, aQuality,  aTime, aIndex); }
			bool Check(uint_32_t aValue, CounterQuality aQuality, size_t aIndex, TimeStamp_t aTime)
			{ return Check<Counter, uint_32_t>(mCounterMap, aValue, aQuality,  aTime, aIndex); }
			bool Check(bool aValue, ControlQuality aQuality, size_t aIndex, TimeStamp_t aTime)
			{ 
				byte_t qual = aQuality;
				if(aValue) qual |= TQ_STATE;
				return Check<ControlStatus, bool>(mControlStatusMap, aValue, qual,  aTime, aIndex); 
			}
			bool Check(int_32_t aValue, SetpointQuality aQuality, size_t aIndex, TimeStamp_t aTime)
			{ return Check<SetpointStatus, int_32_t>(mSetpointStatusMap, aValue, aQuality, aTime, aIndex); }

			bool CheckQual(BinaryQuality aQuality, size_t aIndex){return CheckQual<Binary>(mBinaryMap, aQuality, aIndex);}
			bool CheckQual(AnalogQuality aQuality, size_t aIndex){return CheckQual<Analog>(mAnalogMap, aQuality, aIndex);}
			bool CheckQual(CounterQuality aQuality, size_t aIndex){return CheckQual<Counter>(mCounterMap, aQuality, aIndex);}
			bool CheckQual(ControlQuality aQuality, size_t aIndex){return CheckQual<ControlStatus>(mControlStatusMap, aQuality, aIndex);}
			bool CheckQual(SetpointQuality aQuality, size_t aIndex){return CheckQual<SetpointStatus>(mSetpointStatusMap, aQuality, aIndex);}

			void Print();
			void Clear();

			size_t GetTotalCount()
			{ return mBinaryMap.size() + mAnalogMap.size() + mCounterMap.size() + mControlStatusMap.size() + mSetpointStatusMap.size(); }

			/// The two data observers have the exact same contents
			static bool StrictEquality(const FlexibleDataObserver& arLHS, const FlexibleDataObserver& arRHS);

			/// The RHS is a strict subset of the LHS... i.e. everything in the RHS can be found in the LHS
			static bool IsSubsetOf(const FlexibleDataObserver& arLHS, const FlexibleDataObserver& arRHS);

		private:

			size_t mCommsLostCount;
			size_t mLastCommsLostCheck;
			bool mNewData;
			SigLock mLock;

			void _Start()	{ mLock.Lock(); }
			void _End()		
			{
				bool notify = mNewData;
				mNewData = false;
				mLock.Unlock(); 
				if(notify) this->NotifyAll(); 
			}

			void _Update(const Binary& arPoint, size_t aIndex) { Load(arPoint, mBinaryMap, aIndex); }
			void _Update(const Analog& arPoint, size_t aIndex) { Load(arPoint, mAnalogMap, aIndex); }
			void _Update(const Counter& arPoint, size_t aIndex) { Load(arPoint, mCounterMap, aIndex); }
			void _Update(const ControlStatus& arPoint, size_t aIndex) { Load(arPoint, mControlStatusMap, aIndex); }
			void _Update(const SetpointStatus& arPoint, size_t aIndex) { Load(arPoint, mSetpointStatusMap, aIndex); }
						
			
			template <class T, class U>
			bool Check(typename PointMap<T>::Type& arMap, U aValue, byte_t aQual, size_t aIndex);

			template <class T, class U>
			bool Check(typename PointMap<T>::Type& arMap, U aValue, byte_t aQual, TimeStamp_t aTime, size_t aIndex);

			template <class T>
			bool CheckQual(typename PointMap<T>::Type& arMap, byte_t aQual, size_t aIndex);

			template <class T>
			void SetQuality(byte_t aQuality, typename PointMap<T>::Type& arMap);

			template <class T>
			void Load(const T& arPoint, typename PointMap<T>::Type& arMap, size_t aIndex);

			template <class T>
			void Print(typename PointMap<T>::Type& arMap);

			/// Both maps have the exact same contents
			template<class T>
			static bool StrictEquality(const T& arMap1, const T& arMap2);

			/// Map2 contains everything found in Map1. Map2 is a subset of Map1.
			template<class T>
			static bool IsSubsetOf(const T& arMap1, const T& arMap2);
	};

	template <class T, class U>
	bool FlexibleDataObserver::Check(typename PointMap<T>::Type& arMap, U aValue, byte_t aQual, size_t aIndex)
	{
		typename PointMap<T>::Type::iterator i = arMap.find(aIndex);
		if(i == arMap.end()) return false;
		return i->second.GetValue() == aValue && i->second.GetQuality() == aQual;
	}

	template <class T, class U>
	bool FlexibleDataObserver::Check(typename PointMap<T>::Type& arMap, U aValue, byte_t aQual, TimeStamp_t aTime, size_t aIndex)
	{
		typename PointMap<T>::Type::iterator i = arMap.find(aIndex);
		if(i == arMap.end()) return false;
		return i->second.GetValue() == aValue && i->second.GetQuality() == aQual && i->second.GetTime() == aTime;
	}

	template <class T>
	bool FlexibleDataObserver::CheckQual(typename PointMap<T>::Type& arMap, byte_t aQual, size_t aIndex)
	{
		typename PointMap<T>::Type::iterator i = arMap.find(aIndex);
		if(i == arMap.end()) return false;
		return i->second.GetQuality() == aQual;
	}

	template <class T>
	void FlexibleDataObserver::SetQuality(byte_t aQuality, typename PointMap<T>::Type& arMap)
	{
		typename PointMap<T>::Type::iterator i = arMap.begin();
		for(; i!=arMap.end(); ++i) i->second.SetQuality(aQuality);
	}

	template <class T>
	void FlexibleDataObserver::Load(const T& arPoint, typename PointMap<T>::Type& arMap, size_t aIndex)
	{
		mNewData = true; arMap[aIndex] = arPoint;
	}

	template <class T>
	void FlexibleDataObserver::Print(typename PointMap<T>::Type& arMap)
	{
		int j = 0;
		typename FlexibleDataObserver::PointMap<T>::Type::iterator i = arMap.begin();
		for(;i != arMap.end(); ++i)
		{ std::cout << j << ", " << i->second.GetValue() << ", " << static_cast<int>(i->second.GetQuality()) << std::endl; ++j; }
	}

	

}

#endif

