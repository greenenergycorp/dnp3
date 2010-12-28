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

#include "AddressScanner.h"

#include <APL/Logger.h>
#include <APLXML/XMLConversion.h>
#include <XMLBindings/APLXML_MTS.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

namespace apl { namespace dnp {

AddressScanner::AddressScanner(Logger * apLogger, const APLXML_MTS::MasterTestSet_t& cfg, uint_16_t start, uint_16_t stop) : 
Loggable(apLogger),
manager(apLogger, &cfg.PhysicalLayerList, xml::Convert(cfg.Log.Filter)),
mpService(new boost::asio::io_service()),
mTimerSrc(mpService.get()),
mThread(mpService.get()),
mRouter(apLogger, manager.GetLayer(cfg.PhysicalLayer, mpService.get()), &mTimerSrc, 1000),
mpTimer(NULL),
mMasterAddr(cfg.Master.Stack.LinkLayer.LocalAddress),
mScanTimeout(cfg.Master.Stack.LinkLayer.AckTimeoutMS),
mCurrent(start),
mStop(stop)
{
	manager.SetLayerOwnership(false);	
	mRouter.AddContext(this, mMasterAddr);		
}

void AddressScanner::OnLowerLayerUp()
{
	this->NextFrame();
}

void AddressScanner::OnLowerLayerDown()
{

}

void AddressScanner::OnTimeout()
{
	LOG_BLOCK(LEV_INFO, "Scan timed out for address: " << mFrame.GetDest());
	mpTimer = NULL;
	this->NextFrame();
}

void AddressScanner::NextFrame()
{
	if(mCurrent >= mStop) mRouter.Stop();
	else {
		mpTimer = mTimerSrc.Start(mScanTimeout, boost::bind(&AddressScanner::OnTimeout, this));
		mFrame.FormatResetLinkStates(true, mCurrent, mMasterAddr);
		++mCurrent;
		mRouter.Transmit(mFrame);
	}
}

void AddressScanner::Ack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc)
{
	LOG_BLOCK(LEV_EVENT, "Received acknowledgement from address: " << aSrc);
	mRouter.Stop();
	if(mpTimer != NULL) mpTimer->Cancel();
}

void AddressScanner::Nack(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc) {}
void AddressScanner::LinkStatus(bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc) {}
void AddressScanner::NotSupported (bool aIsMaster, bool aIsRcvBuffFull, uint_16_t aDest, uint_16_t aSrc) {}

void AddressScanner::TestLinkStatus(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc) {}
void AddressScanner::ResetLinkStates(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc) {}
void AddressScanner::RequestLinkStatus(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc) {}
void AddressScanner::ConfirmedUserData(bool aIsMaster, bool aFcb, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength) {}
void AddressScanner::UnconfirmedUserData(bool aIsMaster, uint_16_t aDest, uint_16_t aSrc, const apl::byte_t* apData, size_t aDataLength) {}

void AddressScanner::Run()
{	
	mRouter.Start();

	LOG_BLOCK(LEV_INFO, "Scanning from " << mCurrent << " to " << mStop);
	mThread.Run();
	LOG_BLOCK(LEV_INFO, "Scan complete...");
}

}}

