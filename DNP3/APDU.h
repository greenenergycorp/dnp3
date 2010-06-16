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
#ifndef __APDU_H_
#define __APDU_H_

#include <APL/Types.h>
#include <APL/Exception.h>
#include <APL/CopyableBuffer.h>

#include "AppHeader.h"
#include "ObjectHeader.h"
#include "Objects.h"
#include "APDUConstants.h"
#include "HeaderReadIterator.h"
#include "ObjectWriteIterator.h"
#include "IndexedWriteIterator.h"

#include <vector>
#include <string>
#include <sstream>

namespace apl { namespace dnp {


	/**
		Class for reading and writing APDUs. Interpret method and read iterators form the read interface,
		Set method and object write iterators form the write interface.
	*/
	class APDU
	{

		public:


			APDU(size_t aFragSize = DEFAULT_FRAG_SIZE);

			/// Parse the buffer. Throws exception if malformed data are encountered
			void Interpret();

			/// Parse the header only. Throws exception if header is malformed
			void InterpretHeader();

			/// @return The current size of the fragment in bytes
			size_t Size() const { return mFragmentSize; }

			/// @return pointer to the buffer
			const apl::byte_t* GetBuffer() const { return mBuffer; }

			/// @return The maximum size of the fragment in bytes
			size_t MaxSize() { return mBuffer.Size(); }

			/// Initialize the size to zero.
			void Reset();

			/// Reset and write new data into the buff
			void Write(const apl::byte_t* apStart, size_t aLength);

			/* Getter functions */

			FunctionCodes GetFunction() const;
			void SetFunction(FunctionCodes aCode);

			AppControlField GetControl() const;
			void SetControl(const AppControlField& arControl);

			IINField GetIIN() const;  // throws an exception if FUNC != RSP or UNSOL

			//Overloaed helper that allows you to directly set the control values
			void SetControl(bool aFIR, bool aFIN, bool aCON = false, bool aUNS = false, int aSEQ = 0)
			{
				AppControlField f(aFIR, aFIN, aCON, aUNS, aSEQ);
				this->SetControl(f);
			}

			void Set(FunctionCodes aCode, bool aFIR = true, bool aFIN = true, bool aCON = false, bool aUNS = false, int aSEQ = 0)
			{
				this->Reset();
				this->SetFunction(aCode);
				this->SetControl(aFIR, aFIN, aCON, aUNS, aSEQ);
			}

			void SetIIN(const IINField& arIIN); // throws an exception if FUNC != RSP or UNSOL

			HeaderReadIterator BeginRead() const; // returns an iterator to the first header

			/* Write iterators */

			ObjectWriteIterator WriteContiguous(const FixedObject* apObj, size_t aStart, size_t aStop, QualifierCode aCode = QC_UNDEFINED);
			ObjectWriteIterator WriteContiguous(const BitfieldObject* apObj, size_t aStart, size_t aStop, QualifierCode aCode = QC_UNDEFINED);

			IndexedWriteIterator WriteIndexed(const FixedObject* apObj, size_t aCount, size_t aMaxIndex);
			IndexedWriteIterator WriteIndexed(const FixedObject* apObj, size_t aCount, QualifierCode aCode);

			IndexedWriteIterator WriteIndexed(const VariableByVariationObject* apObj, size_t aSize, size_t aIndex);
			IndexedWriteIterator WriteIndexed(const VariableByVariationObject* apObj, size_t aSize, QualifierCode aCode);


			// Placeholder writes do not need an iterator
			bool DoPlaceholderWrite(ObjectBase* apObj);

			static bool HasData(FunctionCodes aCode);

			std::string ToString() const;

			bool operator==(const APDU& rhs);
			bool operator!=(const APDU& rhs) { return !(*this == rhs); }

		private:

			void CheckWriteState(const ObjectBase*);

			IAppHeader* ParseHeader() const;
			size_t Remainder() { return mBuffer.Size() - mFragmentSize; }

			IndexedWriteIterator WriteCountHeader(size_t aObjectSize, size_t aPrefixSize, byte_t aGrp, byte_t aVar, size_t aCount, QualifierCode aQual);
			void WriteContiguousHeader(IObjectHeader* apHdr, byte_t* apPos, size_t aStart, size_t aStop);

			// Interpreted Information
			bool mIsInterpreted;
			IAppHeader* mpAppHeader;					/// uses a singleton so auto copy is safe
			std::vector<HeaderInfo> mObjectHeaders;

			CopyableBuffer mBuffer;		/// This makes it dynamically sizable without the need for a special copy constructor.
			size_t mFragmentSize;		/// Number of bytes written to the buffer

			QualifierCode GetContiguousQualifier(size_t aStart, size_t aStop);
			QualifierCode GetIndexedQualifier(size_t aMaxIndex, size_t aCount);

			ICountHeader* GetCountHeader(QualifierCode aCode);

			//////////////////////////////////////////////////////////////
			// Private Functions for Interpreting Frames
			//////////////////////////////////////////////////////////////

			IObjectHeader* GetObjectHeader(QualifierCode aCode);

			size_t ReadObjectHeader(size_t aOffset, size_t aRemainder);

			size_t GetPrefixSizeAndValidate(QualifierCode aCode, ObjectTypes aType);
			size_t GetNumObjects(const IObjectHeader* apHeader, const apl::byte_t* pStart);

			std::string GetSizeString(size_t aSize) const
			{
				std::ostringstream oss;
				oss << "Insufficient data for object header: " << aSize;
				return oss.str();
			}
	};

}}

#endif
