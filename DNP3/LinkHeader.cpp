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
#include "LinkHeader.h"

#include <APL/PackingUnpacking.h>
#include "DNPCrc.h"

#include <sstream>

using namespace std;

namespace apl { namespace dnp {

LinkHeader::LinkHeader(){}

LinkHeader::LinkHeader(byte_t aLen, uint_16_t aSrc, uint_16_t aDest, bool aMaster, bool aFcvDfc, bool aFcb, FuncCodes aCode) :
length(aLen),
src(aSrc),
dest(aDest),
ctrl(ControlByte(aMaster, aFcb, aFcvDfc, aCode))
{
	
}

LinkHeader::LinkHeader(const byte_t* apBuff)
{
	this->Read(apBuff);
}

void LinkHeader::Set(byte_t aLen, uint_16_t aSrc, uint_16_t aDest, bool aFromMaster, bool aFcvDfc, bool aFcb, FuncCodes aCode)
{
	length = aLen;
	src = aSrc;
	dest = aDest;
	ctrl = ControlByte(aFromMaster, aFcb, aFcvDfc, aCode);
}

void LinkHeader::ChangeFCB(bool aFCB)
{
	if(aFCB) ctrl |= MASK_FCB;
	else ctrl &= ~MASK_FCB;
}

byte_t LinkHeader::ControlByte(bool aIsMaster, bool aFcb, bool aFcvDfc, FuncCodes aFunc)
{
	byte_t ret = aFunc;

	if(aIsMaster) ret |= MASK_DIR;
	if(aFcb) ret |= MASK_FCB;
	if(aFcvDfc) ret |= MASK_FCV;

	return ret;
}

void LinkHeader::Read(const byte_t* apBuff)
{
	length = apBuff[LI_LENGTH];
	dest = UInt16LE::Read(apBuff + LI_DESTINATION);
	src = UInt16LE::Read(apBuff + LI_SOURCE);
	ctrl = apBuff[LI_CONTROL];
}

void LinkHeader::Write(byte_t* apBuff) const
{
	apBuff[LI_START_05] = 0x05; 
	apBuff[LI_START_64]= 0x64;

	apBuff[LI_LENGTH] = length;
	UInt16LE::Write(apBuff+LI_DESTINATION, dest);
	UInt16LE::Write(apBuff+LI_SOURCE, src);
	apBuff[LI_CONTROL] = ctrl;

	DNPCrc::AddCrc(apBuff, LI_CRC);
}

std::string LinkHeader::ToString() const
{
	ostringstream oss;
	oss << "DL " << this->GetSrc() << " to " << this->GetDest();
	oss << " : " << FuncCodeToString(this->GetFuncEnum());
	oss << " PayloadSize: " << (this->GetLength() - 5);
	oss << ((this->IsFromMaster())?" From Master":" From Outstation");
	if(this->IsPriToSec())
	{
		oss << " Pri->Sec";
		oss << " FCB=" << this->IsFcbSet() << " FCV=" << this->IsFcvDfcSet();
	}
	else
	{
		oss << " Sec->Pri";
		if(this->IsFcbSet()) oss << " ERROR: FCB not Blank!!";
		oss << " DFC=" << this->IsFcvDfcSet();
	}

	return oss.str();
}

}}

