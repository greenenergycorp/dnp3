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
#ifndef __QUEUEING_FDO_H_
#define __QUEUEING_FDO_H_


#include "DataInterfaces.h"
#include "Lock.h"
#include "SubjectBase.h"
#include "Util.h"
#include <APL/FlexibleDataObserver.h>

#include <iostream>
#include <map>
#include <queue>
#include <string>

namespace apl
{

	/** Extension to the FlexibleDataObserver that keeps track of new data updates.
	*/
	class QueueingFDO : public FlexibleDataObserver
	{
	public:

		std::queue<std::string> updates;
	private:

		virtual void _Update(const Binary& arPoint, size_t aIndex) { OnUpdate(arPoint, mBinaryMap, aIndex); }
		virtual void _Update(const Analog& arPoint, size_t aIndex) { OnUpdate(arPoint, mAnalogMap, aIndex); }
		virtual void _Update(const Counter& arPoint, size_t aIndex) { OnUpdate(arPoint, mCounterMap,  aIndex); }
		virtual void _Update(const ControlStatus& arPoint, size_t aIndex) { OnUpdate(arPoint, mControlStatusMap, aIndex); }
		virtual void _Update(const SetpointStatus& arPoint, size_t aIndex) { OnUpdate(arPoint, mSetpointStatusMap, aIndex); }

		
		template <class T>
		void OnUpdate(const T& arPoint, typename PointMap<T>::Type& arMap, size_t aIndex)
		{
			T& current = arMap[aIndex];
			if ( current.GetValue() != arPoint.GetValue() || current.GetQuality() != arPoint.GetQuality() )
			{
				std::ostringstream oss;
				oss << "Update: " << GetDataTypeName(T::MeasEnum) << " (" << aIndex << ")\t\t";
				oss << current.GetValue() << " [" << T::QualConverter::GetSymbolString(current.GetQuality()) << "]";
				oss << " --> " ;
				oss << arPoint.GetValue() << " [" << T::QualConverter::GetSymbolString(arPoint.GetQuality()) << "]";
				updates.push(oss.str());
			}

			Load(arPoint, arMap, aIndex);
		}

	};
}

#endif
