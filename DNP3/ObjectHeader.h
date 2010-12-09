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
#ifndef __OBJECT_HEADER_H_
#define __OBJECT_HEADER_H_


#include <APL/Types.h>
#include <APL/PackingUnpacking.h>
#include <APL/Singleton.h>
#include <APL/Exception.h>
#include <limits>

#include <assert.h>

//irritating windows macro interferes with the <limits> numeric_limits<T>::max()
#undef max


#include "APDUConstants.h"

namespace apl { namespace dnp {

	class ObjectBase;

	enum ObjectHeaderMasks
	{
		OHM_OBJECT_PREFIX = 0x70,
		OHM_RANGE_SPECIFIER = 0x0F
	};

	struct ObjectHeaderField
	{
		ObjectHeaderField(){};
		ObjectHeaderField(apl::byte_t aGroup, apl::byte_t aVariation, QualifierCode aQualifier) :
		Group(aGroup),
		Variation(aVariation),
		Qualifier(aQualifier)
		{}

		apl::byte_t Group;
		apl::byte_t Variation;
		QualifierCode Qualifier;
	};

	enum ObjectHeaderTypes
	{
		OHT_ALL_OBJECTS,
		OHT_RANGED_2_OCTET,
		OHT_RANGED_4_OCTET,
		OHT_RANGED_8_OCTET,
		OHT_COUNT_1_OCTET,
		OHT_COUNT_2_OCTET,
		OHT_COUNT_4_OCTET
	};

	class IObjectHeader
	{
		public:
			virtual ~IObjectHeader(){}
			virtual size_t GetSize() const = 0; // depends on the subtype, default size is 3
			virtual ObjectHeaderTypes GetType() const = 0;

			void Get(const apl::byte_t* apStart, ObjectHeaderField& arData) const;
			void Set(apl::byte_t* apStart, byte_t aGrp, byte_t aVar, QualifierCode aQual) const;

			static QualifierCode ByteToQualifierCode(apl::byte_t aCode);
	};

	struct RangeInfo
	{
		size_t Start;
		size_t Stop;
	};

	class IRangeHeader : public IObjectHeader
	{
		public:
			virtual void GetRange(const apl::byte_t* apStart, RangeInfo& arInfo) const = 0;
			virtual void SetRange(apl::byte_t* apStart, const RangeInfo& arInfo) const = 0;
	};

	class ICountHeader : public IObjectHeader
	{
		public:
			virtual size_t GetCount(const apl::byte_t* apStart) const = 0;
			virtual void SetCount(apl::byte_t* apStart, size_t aCount) const = 0;
	};

	class AllObjectsHeader : public IObjectHeader
	{
		MACRO_SINGLETON_INSTANCE(AllObjectsHeader)

		size_t GetSize() const { return 3; }
		ObjectHeaderTypes GetType() const { return OHT_ALL_OBJECTS; }
	};

	template <class T, ObjectHeaderTypes U>
	class RangedHeader : public IRangeHeader
	{
		MACRO_SINGLETON_INSTANCE(RangedHeader)

		size_t GetSize() const { return Size; }
		ObjectHeaderTypes GetType() const { return U; }

		void GetRange(const apl::byte_t* apStart, RangeInfo& arInfo) const
		{
			arInfo.Start = T::Read(apStart+3);
			arInfo.Stop = T::Read(apStart+3+T::Size);
		}

		void SetRange(apl::byte_t* apStart, const RangeInfo& arInfo) const
		{
			if(arInfo.Start > arInfo.Stop) throw ArgumentException(LOCATION, "stop > start");
			if(arInfo.Stop > T::Max) throw ArgumentException(LOCATION, "stop > max");

			T::Write(apStart+3, static_cast<typename T::Type>(arInfo.Start));
			T::Write(apStart+3+T::Size, static_cast<typename T::Type>(arInfo.Stop));
		}

		static size_t MaxRange()
		{ return T::Max; }

		const static size_t Size = 3+2*T::Size;
	};

	template <class T, ObjectHeaderTypes U>
	RangedHeader<T,U> RangedHeader<T,U>::mInstance;

	template <class T, ObjectHeaderTypes U>
	class CountHeader : public ICountHeader
	{
		MACRO_SINGLETON_INSTANCE(CountHeader)

		size_t GetSize() const { return Size; }
		ObjectHeaderTypes GetType() const { return U; }

		size_t GetCount(const apl::byte_t* apStart) const
		{ return T::Read(apStart+3); }

		void SetCount(apl::byte_t* apStart, size_t aCount) const
		{
			if(aCount > T::Max) throw ArgumentException(LOCATION);
			T::Write(apStart+3, static_cast<typename T::Type>(aCount));
		}

		static size_t MaxCount()
		{ return T::Max; }

		const static size_t Size = 3 + T::Size;
	};

	template <class T, ObjectHeaderTypes U>
	CountHeader<T,U> CountHeader<T,U>::mInstance;

	//Typedefs so you don't have to direcly use the templates
	typedef RangedHeader<apl::UInt8, OHT_RANGED_2_OCTET>	Ranged2OctetHeader;
	typedef RangedHeader<apl::UInt16LE, OHT_RANGED_4_OCTET> Ranged4OctetHeader;
	typedef RangedHeader<apl::UInt32LE, OHT_RANGED_8_OCTET> Ranged8OctetHeader;

	typedef CountHeader<apl::UInt8, OHT_COUNT_1_OCTET>		Count1OctetHeader;
	typedef CountHeader<apl::UInt16LE, OHT_COUNT_2_OCTET>	Count2OctetHeader;
	typedef CountHeader<apl::UInt32LE, OHT_COUNT_4_OCTET>	Count4OctetHeader;

}}

#endif
