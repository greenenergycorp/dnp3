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
#include "PackingUnpacking.h"


#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "Util.h"
#include <memory>

namespace apl
{
	const byte_t UInt8::Max = std::numeric_limits<byte_t>::max();
	const byte_t UInt8::Min = std::numeric_limits<byte_t>::min();


	/* ARM has a word alignment issue when using reinterpret cast. The float/double read/write routines
	now uses an intermediate buffer that the compiler word aligns. */
	float SingleFloat::Read(const apl::byte_t* apStart)
	{
		#ifdef ARM
			byte_t buff[sizeof(float)];
			memcpy(buff, apStart, sizeof(float));
			return Float<float>::NaiveRead(buff);
		#else
			return Float<float>::NaiveRead(apStart);
		#endif
	}
	
	void SingleFloat::Write(apl::byte_t* apStart, float aValue)
	{
		#ifdef ARM
			byte_t buff[sizeof(float)];
			Float<float>::NaiveWrite(buff, aValue);
			memcpy(apStart, buff, sizeof(float));
		#else
			Float<float>::NaiveWrite(apStart, aValue);
		#endif
	}

	double DoubleFloat::Read(const apl::byte_t* apStart)
	{ 
		#ifdef ARM
			byte_t buff[sizeof(double)];
			memcpy(buff, apStart, sizeof(double));
			return FlipWord32(Float<double>::NaiveRead(buff));
		#else
			return Float<double>::NaiveRead(apStart);
		#endif
	}

	void DoubleFloat::Write(apl::byte_t* apStart, double aValue)
	{
		#ifdef ARM
			byte_t buff[sizeof(double)];
			Float<double>::NaiveWrite(buff, FlipWord32(aValue));
			memcpy(apStart, buff, sizeof(double));
		#else
			Float<double>::NaiveWrite(apStart, aValue);
		#endif		
	}

	#ifdef ARM
	double DoubleFloat::FlipWord32(double aValue)
	{
		volatile double x = aValue;
		volatile uint_32_t* p = reinterpret_cast<volatile uint_32_t*>(&x);		
		uint_32_t tmp = p[0];
		p[0] = p[1];
		p[1] = tmp;				
		return x;
	}
	#endif

}
