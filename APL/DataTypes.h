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
#ifndef __DATA_TYPES_H_
#define __DATA_TYPES_H_


#include "QualityMasks.h"
#include "Types.h"

#include <sstream>
#include <ostream>
#include <limits>
#include <math.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace apl
{
	template <class T>
	struct MaxMinWrapper
	{
		static T Max() { return std::numeric_limits<T>::max(); }
		static T Min() { return std::numeric_limits<T>::min(); }
	};

	template <>
	struct MaxMinWrapper<double> //partial specialization for double
	{
		static double Max() { return std::numeric_limits<double>::max(); }
		static double Min() { return -std::numeric_limits<double>::max(); }
	};

	// useful template for pairing a value with an index
	template <class T>
	struct Change
	{
		Change(){}
		Change(const T& arVal, size_t aIndex) :
		mValue(arVal),
		mIndex(aIndex)
		{}

		T mValue;
		size_t mIndex;
	};

	enum DataTypes
	{
		DT_BINARY,
		DT_ANALOG,
		DT_COUNTER,
		DT_CONTROL_STATUS,
		DT_SETPOINT_STATUS
	};

	std::string GetDataTypeName(DataTypes aType);

	/**
	  Base class shared by all of the DataPoint types. There are 5 major data types and they all have
	  a value, timestamp and a quality field. The timestamp should reflect when the value was measured
	  or calculated. The quality field should be set approriately depending on the data type. Each datatype
	  has a its own defintion of the quality field that indicate specific conditions but all of the
	  datatypes define an XX_ONLINE bit, that is the default "nominal" value. This quality field is not
	  for applying alarming information, that needs to be done with Binaries or in other channels.
	*/
	class DataPoint
	{
		public:
		virtual ~DataPoint(){}

		DataTypes GetType() const;
		TimeStamp_t GetTime() const;

		virtual byte_t GetQuality() const;
		bool CheckQualityBit(byte_t aQualMask) const;

		virtual void SetQuality(byte_t aQuality);
		void SetTime(const TimeStamp_t arTime);
		void SetToNow();

		virtual std::string ToString() const = 0;

		protected:

		//These constructors can only be invoked by super classes
		DataPoint(byte_t aQuality, DataTypes aType);

		byte_t mQuality;	//	bitfield that stores type specific quality information
		TimeStamp_t mTime;		//	the time that the measurement was made

		private:
		DataPoint();
		DataTypes mType;
	};

	// Inlined definitions from DataPoint
	inline DataTypes DataPoint::GetType() const { return mType; }
	inline TimeStamp_t DataPoint::GetTime() const { return mTime; }
	inline byte_t  DataPoint::GetQuality() const { return mQuality; }
	inline bool DataPoint::CheckQualityBit(byte_t aQualMask) const{ return (aQualMask & mQuality) != 0; }
	inline void DataPoint::SetTime(const TimeStamp_t arTime) { mTime = arTime; }
	inline void DataPoint::SetQuality(byte_t aQuality) { mQuality = aQuality; }

	/**
       Base class for Binary and ControlStatus data types, shouldn't be used directly.
	*/
	class BoolDataPoint : public DataPoint
	{
		public:

		bool GetValue() const;
		void SetValue(bool aValue);

		byte_t GetQuality() const;
		void SetQuality(byte_t aQuality);

		void SetQualityValue(byte_t aFlag);

		bool ShouldGenerateEvent(const BoolDataPoint& arRHS, double aDeadband, uint_32_t aLastReportedVal) const;

		std::string ToString() const;

		bool operator==(const BoolDataPoint& rhs)
		{ return GetValue() == rhs.GetValue() && GetQuality() == rhs.GetQuality(); }

		protected:
		//BoolDataPoint(const BoolDataPoint& arRHS);
		BoolDataPoint(byte_t aQuality, DataTypes aType, byte_t aValueMask);

		private:
		BoolDataPoint();
		// bool data points store their value as a bit in the quality field
		byte_t mValueMask;
	};

	inline void BoolDataPoint::SetValue(bool aValue)
	{
		mQuality = (aValue) ? (mQuality|mValueMask) : (mQuality&(~mValueMask));
	}
	inline bool BoolDataPoint::GetValue() const { return (mQuality&mValueMask) != 0; }

	inline byte_t BoolDataPoint::GetQuality() const { return mQuality; }

	inline void BoolDataPoint::SetQualityValue(byte_t aFlag)
	{
		mQuality = aFlag;
	}

	inline void BoolDataPoint::SetQuality(byte_t aQuality)
	{
		mQuality = (mQuality & (mValueMask));
		mQuality |= aQuality;
	}

	inline bool BoolDataPoint::ShouldGenerateEvent(const BoolDataPoint& arRHS, double /*aDeadband*/, uint_32_t /*aLastReportedVal*/) const
	{ return mQuality != arRHS.mQuality; }

	template <class T>
	bool ExceedsDeadband(const T& val1, const T& val2, double aDeadband)
	{
		//T can be unsigned data type so std::abs won't work since it only directly supports signed data types
		//If one uses std::abs and T is unsigned one will get an ambiguous override error.
		uint_32_t diff;

		if (val1 < val2){
			diff = val2 - val1;
		}else{
			diff = val1 - val2;
		}

		return (diff > aDeadband);
	}

	template <>
	bool ExceedsDeadband<double>(const double& val1, const double& val2, double aDeadband);

	//Common subclass to analogs and counters
	template <class T>
	class TypedDataPoint : public DataPoint
	{
		public:

		T GetValue() const { return mValue; }
		void SetValue(T aValue) { mValue = aValue; }

		bool ShouldGenerateEvent(const TypedDataPoint<T>& arRHS, double aDeadband, T aLastReportedVal) const;

		typedef T Type;

		static const T MAX_VALUE;
		static const T MIN_VALUE;

		std::string ToString() const;

		bool operator==(const TypedDataPoint<T>& rhs)
		{ return GetValue() == rhs.GetValue() && GetQuality() == rhs.GetQuality(); }

		protected:
		// IntDataPoints have seperate fields for quality and value
		//IntDataPoint(const IntDataPoint& arRHS);
		TypedDataPoint(byte_t aQuality, DataTypes aType);
		T mValue;

		private:
		TypedDataPoint();
	};

	template <class T>
	const T TypedDataPoint<T>::MAX_VALUE = MaxMinWrapper<T>::Max();

	template <class T>
	const T TypedDataPoint<T>::MIN_VALUE = MaxMinWrapper<T>::Min();

	template <class T>
	TypedDataPoint<T>::TypedDataPoint(byte_t aQuality, DataTypes aType) :
	DataPoint(aQuality, aType),
	mValue(0)
	{

	}

	template <class T>
	bool TypedDataPoint<T>::ShouldGenerateEvent(const TypedDataPoint<T>& arRHS, double aDeadband, T aLastReportedVal) const
	{
		if (mQuality != arRHS.mQuality)	return true;

		return ExceedsDeadband<T>(arRHS.GetValue(), aLastReportedVal, aDeadband);
	}

	template <class T>
	std::string TypedDataPoint<T>::ToString() const
	{
		std::ostringstream oss;
		oss << "Value: " << GetValue() << " Quality: " << static_cast<int>(GetQuality());
		return oss.str();
	}

#ifdef SWIG
%template(DoublePoint) apl::TypedDataPoint<double>;
%template(UnsignedPoint) apl::TypedDataPoint<apl::uint_32_t>;
#endif

	///////////////////////////////////////////////////////////////////
	// Concrete Classes
	///////////////////////////////////////////////////////////////////

	/**
		The Binary data type is for describing on-off (boolean) type values. Good examples of
		binaries are alarms, mode settings, enabled/disabled flags etc. Think of it as a status
		LED on a piece of equipment.
	*/
	class Binary : public BoolDataPoint
	{
		public:
		Binary(bool aValue, byte_t aQuality = BQ_RESTART) : BoolDataPoint(BQ_RESTART, DT_BINARY, BQ_STATE)
		{
			SetValue(aValue);
			SetQuality(aQuality);
		}
		Binary() : BoolDataPoint(BQ_RESTART, DT_BINARY, BQ_STATE) {}

		typedef bool ValueType;
		typedef BinaryQuality QualityType;
		typedef BinaryQualConverter QualConverter;

		/// Describes the static data type of the measurement as an enum
		static const DataTypes MeasEnum = DT_BINARY;

		static const int ONLINE = BQ_ONLINE;

		operator ValueType() const { return this->GetValue(); }
		ValueType operator=(ValueType aValue) { this->SetValue(aValue); return GetValue(); }
	};

	/**
		ControlStatus is used for describing the current state of a control. It is very infrequently
		used and many masters don't provide any mechanisms for reading these values so their use is
		strongly discouraged, a Binary should be used instead.
	*/
	class ControlStatus : public BoolDataPoint
	{
		public:

		ControlStatus(bool aValue, byte_t aQuality = TQ_RESTART) : BoolDataPoint(TQ_RESTART, DT_CONTROL_STATUS, TQ_STATE)
		{
			SetValue(aValue);
			SetQuality(aQuality);
		}

		ControlStatus() : BoolDataPoint(TQ_RESTART, DT_CONTROL_STATUS, TQ_STATE) {}

		typedef bool ValueType;
		typedef ControlQuality QualityType;
		typedef ControlQualConverter QualConverter;

		static const DataTypes MeasEnum = DT_CONTROL_STATUS;

		static const int ONLINE = TQ_ONLINE;

		operator ValueType() const { return this->GetValue(); }
		ValueType operator=(ValueType aValue) { this->SetValue(aValue); return GetValue(); }
	};

	/**
		Analogs are used for variable data points that usuually reflect a real world value.
		Good examples are current, voltage, sensor readouts, etc. Think of a speedometer gauge.
	*/

	class Analog : public TypedDataPoint<double>
	{
		public:
		Analog() : TypedDataPoint<double>(AQ_RESTART, DT_ANALOG) {}

		Analog(double aVal, byte_t aQuality = AQ_RESTART) : TypedDataPoint<double>(AQ_RESTART, DT_ANALOG)
		{
			SetValue(aVal);
			SetQuality(aQuality);
		}


		typedef double ValueType;
		typedef AnalogQuality QualityType;
		typedef AnalogQualConverter QualConverter;

		static const DataTypes MeasEnum = DT_ANALOG;

		static const int ONLINE = AQ_ONLINE;

		operator ValueType() const { return this->GetValue(); }
		ValueType operator=(ValueType aValue) { this->SetValue(aValue); return GetValue(); }


	};

	/**
		Counters are used for describing generally increasing values (non-negative!). Good examples are
		total power consumed, max voltage. Think odometer on a car.
	*/
	class Counter : public TypedDataPoint<uint_32_t>
	{
		public:
		Counter() : TypedDataPoint<uint_32_t>(CQ_RESTART, DT_COUNTER) {}
		Counter(uint_32_t aVal, byte_t aQuality = CQ_RESTART) : TypedDataPoint<uint_32_t>(CQ_RESTART, DT_COUNTER)
		{
			SetValue(aVal);
			SetQuality(aQuality);
		}

		typedef uint_32_t ValueType;
		typedef CounterQuality QualityType;
		typedef CounterQualConverter QualConverter;

		static const int ONLINE = CQ_ONLINE;

		static const DataTypes MeasEnum = DT_COUNTER;

		operator ValueType() const { return this->GetValue(); }
		ValueType operator=(ValueType aValue) { this->SetValue(aValue); return GetValue(); }
	};

	/**
		Descibes the last set value of the setpoint. Like the ControlStatus data type it is not
		well supportted and its generally better practice to use an explict analog.
	*/
	class SetpointStatus : public TypedDataPoint<double>
	{
		public:
		SetpointStatus() : TypedDataPoint<double>(PQ_RESTART, DT_SETPOINT_STATUS) {}
		SetpointStatus(double aVal, byte_t aQuality = PQ_RESTART) : TypedDataPoint<double>(PQ_RESTART, DT_SETPOINT_STATUS)
		{
			SetValue(aVal);
			SetQuality(aQuality);
		}

		typedef double ValueType;
		typedef SetpointQuality QualityType;
		typedef SetpointQualConverter QualConverter;

		static const int ONLINE = PQ_ONLINE;

		static const DataTypes MeasEnum = DT_SETPOINT_STATUS;

		operator ValueType() const { return this->GetValue(); }
		ValueType operator=(ValueType aValue) { this->SetValue(aValue); return GetValue(); }
	};

}

#endif
