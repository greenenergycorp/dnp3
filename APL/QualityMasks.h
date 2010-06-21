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
#ifndef __QUALITY_MASKS_H_
#define __QUALITY_MASKS_H_

#include "Types.h"
#include <string>
#include <sstream>

#define QUAL_INFO_DECL(name, convert, num) \
	class name \
	{ \
	public: \
		static byte_t masks[]; \
		static std::string names[]; \
		static char symbols[]; \
	}; \
	typedef QualityConverter<name, num> convert; 

namespace apl
{

	template <class T, int N>
	class QualityConverter
	{
	public:
		static char GetSymbol(byte_t aMask);
		static std::string GetName(byte_t aMask);
		static byte_t GetMask(char aSymbol);
		static std::string GetSymbolString(byte_t aQual);
		static std::string GetNameString(byte_t aQual);

		static std::string GetAllSymbols();
	};

	
	/**
		Bitmasks that make make up the quality field for binaries. The first 5 field are common across all of the data types.
	 */
	enum BinaryQuality
	{
		BQ_ONLINE  = 0x01,				//!< set when the data is "good", meaning that rest of the system can trust the value
		BQ_RESTART = 0x02,				//!< the quality all points get before we have established communication (or populated) the point
		BQ_COMM_LOST = 0x04,			//!< this means the communication has been lost with the source of the data (after establishing contact)
		BQ_REMOTE_FORCED_DATA = 0x08,	//!< the value is being forced to a "fake" value somewhere in the system
		BQ_LOCAL_FORCED_DATA = 0x10,	//!< the value is being forced to a "fake" value on the original device
		BQ_CHATTER_FILTER = 0x20,		//!< set when the value is osciallating very quickly and some events are being suppressed
		BQ_RESERVED = 0x40,				//!< reserved
		BQ_STATE = 0x80,				//!< the actual value of the binary
	};

	QUAL_INFO_DECL(BinaryQualInfo, BinaryQualConverter, 6);


	/**
		Bitmasks that make make up the quality field for analogs. See BinaryQuality for common (unlabeled) bitmasks.
	 */
	enum AnalogQuality
	{
		AQ_ONLINE = 0x01,
		AQ_RESTART= 0x02,
		AQ_COMM_LOST = 0x04,
		AQ_REMOTE_FORCED_DATA = 0x08,
		AQ_LOCAL_FORCED_DATA = 0x10,
		AQ_OVERRANGE = 0x20,			//!< if a hardware input etc. is out of range and we are using a place holder value
		AQ_REFERENCE_CHECK = 0x40,		//!< meaning we may have lost calibration or refrence voltage so readings are questionable
		AQ_RESERVED = 0x80
	};

	QUAL_INFO_DECL(AnalogQualInfo, AnalogQualConverter, 7);

	/**
		Bitmasks that make make up the quality field for counters. See BinaryQuality for common (unlabeled) bitmasks.
	 */
	enum CounterQuality
	{
		CQ_ONLINE = 0x01,
		CQ_RESTART= 0x02,
		CQ_COMM_LOST = 0x04,
		CQ_REMOTE_FORCED_DATA = 0x08,
		CQ_LOCAL_FORCED_DATA = 0x10,
		CQ_ROLLOVER = 0x20,				//!< used to indicate that the counter filled up and rolledover, cleared automatically after reading
		CQ_DISCONTINUITY = 0x40,		//!< indicates an unusual change in value
		CQ_RESERVED = 0x80
	};

	QUAL_INFO_DECL(CounterQualInfo, CounterQualConverter, 7);

	/**
		Bitmasks that make make up the quality field for control statuses. See BinaryQuality for common (unlabeled) bitmasks.
	 */
	enum ControlQuality
	{
		TQ_ONLINE = 0x01,
		TQ_RESTART = 0x02,
		TQ_COMM_LOST = 0x04,
		TQ_REMOTE_FORCED_DATA = 0x08,
		TQ_LOCAL_FORCED_DATA = 0x10,
		TQ_RESERVED_1 = 0x20,
		TQ_RESERVED_2 = 0x40,
		TQ_STATE = 0x80
	};
	
	QUAL_INFO_DECL(ControlQualInfo, ControlQualConverter, 5);

	/**
		Bitmasks that make make up the quality field for setpoint statuses. See BinaryQuality for common (unlabeled) bitmasks.
	 */
	enum SetpointQuality
	{
		PQ_ONLINE = 0x01,
		PQ_RESTART = 0x02,
		PQ_COMM_LOST = 0x04,
		PQ_REMOTE_FORCED_DATA = 0x08,
		PQ_RESERVED_1 = 0x10,
		PQ_RESERVED_2 = 0x20,
		PQ_RESERVED_3 = 0x40,
		PQ_RESERVED_4 = 0x80,
	};

	QUAL_INFO_DECL(SetpointQualInfo, SetpointQualConverter, 4);

	std::string SetpointStatusQualToString(byte_t aQual);

	/**
		Bitmasks that make make up the quality field for vto data. See BinaryQuality for common (unlabeled) bitmasks.
	 */
	enum VTOQuality
	{
		VQ_ONLINE = 0x01,
		VQ_RESTART = 0x02,
		VQ_COMM_LOST = 0x04,
		VQ_REMOTE_FORCED_DATA = 0x08,
		VQ_RESERVED_1 = 0x10,
		VQ_RESERVED_2 = 0x20,
		VQ_RESERVED_3 = 0x40,
		VQ_RESERVED_4 = 0x80,
	};


	template<class T, int N>
	char QualityConverter<T,N>::GetSymbol(byte_t aMask)
	{
		for ( int i = 0; i < N; i++ )
			if ( T::masks[i] == aMask )
				return T::symbols[i];
		return '.';
	}
	template<class T, int N>
	std::string QualityConverter<T,N>::GetName(byte_t aMask)
	{
		for ( int i = 0; i < N; i++ )
			if ( T::masks[i] == aMask )
				return T::names[i];
		return "Reserved";
	}
	template<class T, int N>
	byte_t QualityConverter<T,N>::GetMask(char aSymbol)
	{
		for ( int i = 0; i < N; i++ )
			if ( T::symbols[i] == aSymbol )
				return T::masks[i];
		return 0;
	}
	template<class T, int N>
	std::string QualityConverter<T,N>::GetSymbolString(byte_t aQual)
	{
		std::ostringstream oss;
		for ( int i = 0; i < N; i++ )
		{
			if ( aQual & T::masks[i] )
				oss << T::symbols[i];
			else
				oss << '.';
		}
		for ( int i = 0; i < (8-N); i++ )
			oss << '.';
		return oss.str();
	}
	template<class T, int N>
	std::string QualityConverter<T,N>::GetNameString(byte_t aQual)
	{
		std::ostringstream oss;
		for ( int i = 0; i < N; i++ )
			if ( aQual & T::masks[i] )
				oss << " " << T::names[i];
		return oss.str();
	}

	template<class T, int N>
	std::string QualityConverter<T,N>::GetAllSymbols()
	{
		std::ostringstream oss;
		for ( int i = 0; i < N; i++ )
			oss << T::symbols[i];
		return oss.str();
	}

}

#endif
