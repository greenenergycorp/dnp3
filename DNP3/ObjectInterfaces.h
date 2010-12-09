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
#ifndef __OBJECT_INTERFACES_H_
#define __OBJECT_INTERFACES_H_

#include <APL/Types.h>

#include <assert.h>
#include <stddef.h>
#include <cstring>
#include <string>

namespace apl {
	class VTOData;
	class CopyableBuffer;
}

/*
	Object hierarchy


				 ___ PlaceHolderObject
				|						 -- FixedObject --- StreamObject<T>
				|						|
	ObjectBase -|--- IndexedObject -----|
				|						|-- BitFieldObject
				|___ VariableObject		|
										|__ VariableByVariationObject

*/
namespace apl { namespace dnp {


	enum ObjectTypes
	{
		OT_PLACEHOLDER,
		OT_FIXED,
		OT_BITFIELD,
		OT_VARIABLE,
		OT_VARIABLE_BY_VARIATION
	};

	class ObjectBase
	{
		public:
			virtual ~ObjectBase(){}
			virtual ObjectTypes GetType() const = 0;
			virtual int GetGroup() const = 0;
			virtual int GetVariation() const = 0;
			virtual bool UseCTO() const { return false; }
			virtual bool IsEvent() const { return false; }

			static ObjectBase* Get(int aGroup, int aVariation);

			bool Equals(ObjectBase* pObj) const
			{ return (pObj->GetGroup() == this->GetGroup() && pObj->GetVariation() == this->GetVariation()); }

			virtual std::string Name() const = 0;
	};

	//Common base class for all objects that must have an index associated with them
	class IndexedObject : public ObjectBase {};

	class PlaceHolderObject : public ObjectBase
	{
		public:
			virtual ObjectTypes GetType() const { return OT_PLACEHOLDER; }
			size_t GetSize() { return 0; }
	};

	class FixedObject : public IndexedObject
	{
		public:
			virtual ObjectTypes GetType() const { return OT_FIXED; }
			virtual size_t GetSize() const = 0;
	};

	template <class T>
	class StreamObject : public FixedObject
	{
		public:
			virtual void Write(apl::byte_t*, const T&) const = 0;
			virtual T Read(const apl::byte_t*) const = 0;
			
			virtual bool HasQuality() const { return false; }

			typedef T DataType;
	};

	template <class T>
	class CommandObject : public StreamObject<T>
	{
		public:
			virtual apl::CopyableBuffer GetValueBytes(const apl::byte_t*) const = 0;
	};

	class BitfieldObject : public IndexedObject
	{
		public:
			virtual ObjectTypes GetType() const { return OT_BITFIELD; }

			size_t GetSize(size_t aNumValues) const
			{
				size_t ret = (aNumValues >> 3); //integer division by eight
				if( (aNumValues & 0x07) != 0) ++ret; //if it's not an even multiple of 8 add an extra byte
				return ret;
			}

			void Zero(apl::byte_t* apPos, size_t aNumValues) const
			{
				size_t num_bytes = GetSize(aNumValues);
				for(size_t i=0; i<num_bytes; i++)
				{
					*(apPos++) = 0;
				}
			}

			bool Read(const apl::byte_t* apPos,size_t aStartIndex, size_t aIndex) const
			{ return StaticRead(apPos, aStartIndex, aIndex); }

			void Write(apl::byte_t* apPos, size_t aStartIndex, size_t aIndex, bool aValue) const
			{ StaticWrite(apPos, aStartIndex, aIndex, aValue); }

			static bool StaticRead(const apl::byte_t* apPos,size_t aStartIndex, size_t aIndex)
			{
				assert(aIndex >= aStartIndex);
				size_t pos =  aIndex - aStartIndex;
				apPos += (pos >> 3); //figure out which byte you are on and advance the pointer
				return ((*apPos) & (1 << (pos & 0x07))) != 0;
			}

			static void StaticWrite(apl::byte_t* apPos, size_t aStartIndex, size_t aIndex, bool aValue)
			{
				assert(aIndex >= aStartIndex);
				size_t pos =  aIndex - aStartIndex;
				apPos += (pos >> 3); //figure out which byte you are on and advance the pointer
				size_t bit_mask = 1 << (pos & 0x07);
				if (aValue) *apPos |= bit_mask;
				else *apPos &= ~bit_mask;
			}
	};

	class VariableByVariationObject : public IndexedObject
	{
		public:
			virtual ObjectTypes GetType() const { return OT_VARIABLE_BY_VARIATION; }

			bool Read(const apl::byte_t* apPos, size_t aVariation, apl::byte_t* apOut) const
			{
				assert(aVariation <= 255);
				memcpy(apOut, apPos, aVariation);
				return true;
			}

			void Write(apl::byte_t* apPos, size_t aVariation, const apl::byte_t* apIn) const
			{
				assert(aVariation <= 255);
				memcpy(apPos, apIn, aVariation);
			}
	};

}}

#endif
