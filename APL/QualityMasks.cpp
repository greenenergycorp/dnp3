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
#include "QualityMasks.h"

#include <sstream>
#include <vector>


using namespace std;


#define CASE_RET(a, ret) case a: return ret
#define RET_CASE(ret, a) case a: return ret



namespace apl
{

	byte_t BinaryQualInfo::masks[] = {BQ_ONLINE, BQ_RESTART, BQ_COMM_LOST, BQ_REMOTE_FORCED_DATA, BQ_LOCAL_FORCED_DATA, BQ_CHATTER_FILTER};
	string BinaryQualInfo::names[] = {"Online", "Restart", "CommsLost", "RemoteForced", "LocalForced", "ChatterFilter"};
	char BinaryQualInfo::symbols[] = {'O', 'R', 'C', 'F', 'L', 'T'};

	byte_t AnalogQualInfo::masks[] = {AQ_ONLINE, AQ_RESTART, AQ_COMM_LOST, AQ_REMOTE_FORCED_DATA, AQ_LOCAL_FORCED_DATA, AQ_OVERRANGE, AQ_REFERENCE_CHECK};
	string AnalogQualInfo::names[] = {"Online", "Restart", "CommsLost", "RemoteForced", "LocalForced", "Overrange", "ReferenceCheck"};
	char AnalogQualInfo::symbols[] = {'O', 'R', 'C', 'F', 'L', 'V', 'K'};

	byte_t CounterQualInfo::masks[] = {CQ_ONLINE, CQ_RESTART, CQ_COMM_LOST, CQ_REMOTE_FORCED_DATA, CQ_LOCAL_FORCED_DATA, CQ_ROLLOVER, CQ_DISCONTINUITY};
	string CounterQualInfo::names[] = {"Online", "Restart", "CommsLost", "RemoteForced", "LocalForced", "Rollover", "Discontinuity"};
	char CounterQualInfo::symbols[] = {'O', 'R', 'C', 'F', 'L', 'V', 'D'};

	byte_t ControlQualInfo::masks[] = {TQ_ONLINE, TQ_RESTART, TQ_COMM_LOST, TQ_REMOTE_FORCED_DATA, TQ_LOCAL_FORCED_DATA};
	string ControlQualInfo::names[] = {"Online", "Restart", "CommsLost", "RemoteForced", "LocalForced"};
	char ControlQualInfo::symbols[] = {'O', 'R', 'C', 'F', 'L'};

	byte_t SetpointQualInfo::masks[] = {PQ_ONLINE, PQ_RESTART, PQ_COMM_LOST, PQ_REMOTE_FORCED_DATA};
	string SetpointQualInfo::names[] = {"Online", "Restart", "CommsLost", "RemoteForced"};
	char SetpointQualInfo::symbols[] = {'O', 'R', 'C', 'F'};

	string BinaryQualToString(byte_t aQual)
	{
		ostringstream oss;
		if(aQual & BQ_ONLINE) oss << " Online";
		if(aQual & BQ_RESTART) oss << " Restart";
		if(aQual & BQ_COMM_LOST) oss << " CommLost";
		if(aQual & BQ_REMOTE_FORCED_DATA) oss << " RemoteForced";
		if(aQual & BQ_LOCAL_FORCED_DATA) oss << " LocalForced";
		if(aQual & BQ_CHATTER_FILTER) oss << " ChatterFilter";
		return oss.str();
	}

	string AnalogQualToString(byte_t aQual)
	{
		ostringstream oss;
		if(aQual & AQ_ONLINE) oss << " Online";
		if(aQual & AQ_RESTART) oss << " Restart";
		if(aQual & AQ_COMM_LOST) oss << " CommLost";
		if(aQual & AQ_REMOTE_FORCED_DATA) oss << " RemoteForced";
		if(aQual & AQ_LOCAL_FORCED_DATA) oss << " LocalForced";
		if(aQual & AQ_OVERRANGE) oss << " OverRange";
		if(aQual & AQ_REFERENCE_CHECK) oss << " ReferenceCheck";
		return oss.str();
	}

	string CounterQualToString(byte_t aQual)
	{
		ostringstream oss;
		if(aQual & CQ_ONLINE) oss << " Online";
		if(aQual & CQ_RESTART) oss << " Restart";
		if(aQual & CQ_COMM_LOST) oss << " CommLost";
		if(aQual & CQ_REMOTE_FORCED_DATA) oss << " RemoteForced";
		if(aQual & CQ_LOCAL_FORCED_DATA) oss << " LocalForced";
		if(aQual & CQ_ROLLOVER) oss << " Rollover";
		if(aQual & CQ_DISCONTINUITY) oss << " Discontinuity";
		return oss.str();
	}

	string ControlStatusQualToString(byte_t aQual)
	{
		ostringstream oss;
		if(aQual & TQ_ONLINE) oss << " Online";
		if(aQual & TQ_RESTART) oss << " Restart";
		if(aQual & TQ_COMM_LOST) oss << " CommLost";
		if(aQual & TQ_REMOTE_FORCED_DATA) oss << " RemoteForced";
		if(aQual & TQ_LOCAL_FORCED_DATA) oss << " LocalForced";
		if(aQual & TQ_RESERVED_1) oss << " Reserved1";
		if(aQual & TQ_RESERVED_2) oss << " Reserved2";
		return oss.str();
	}

	string SetpointStatusQualToString(byte_t aQual)
	{
		ostringstream oss;
		if(aQual & PQ_ONLINE) oss << " Online";
		if(aQual & PQ_RESTART) oss << " Restart";
		if(aQual & PQ_COMM_LOST) oss << " CommLost";
		if(aQual & PQ_REMOTE_FORCED_DATA) oss << " RemoteForced";
		if(aQual & PQ_RESERVED_1) oss << " Reserved1";
		if(aQual & PQ_RESERVED_2) oss << " Reserved2";
		if(aQual & PQ_RESERVED_3) oss << " Reserved3";
		if(aQual & PQ_RESERVED_4) oss << " Reserved4";
		return oss.str();
	}

}
