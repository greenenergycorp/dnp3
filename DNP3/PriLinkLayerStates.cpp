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
#include "PriLinkLayerStates.h"

#include <APL/Exception.h>
#include <APL/Loggable.h>
#include <APL/Logger.h>

#include "DNPConstants.h"
#include "AsyncLinkLayer.h"

namespace apl { namespace dnp {

///////////////////////////////////////////////////////////
// PriStateBase
///////////////////////////////////////////////////////////

void PriStateBase::Ack(AsyncLinkLayer* apLL, bool aIsRcvBuffFull)
{ ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Frame context not understood", DLERR_UNEXPECTED_FRAME); }
void PriStateBase::Nack(AsyncLinkLayer* apLL, bool aIsRcvBuffFull)
{ ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Frame context not understood", DLERR_UNEXPECTED_FRAME); }
void PriStateBase::LinkStatus(AsyncLinkLayer* apLL, bool aIsRcvBuffFull)
{ ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Frame context not understood", DLERR_UNEXPECTED_FRAME); }
void PriStateBase::NotSupported (AsyncLinkLayer* apLL, bool aIsRcvBuffFull)
{ ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Frame context not understood", DLERR_UNEXPECTED_FRAME); }

void PriStateBase::OnTimeout(AsyncLinkLayer* apLL)
{ throw InvalidStateException(LOCATION, this->Name()); }

void PriStateBase::SendConfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t)
{ throw InvalidStateException(LOCATION, this->Name()); }

void PriStateBase::SendUnconfirmed(AsyncLinkLayer*, const apl::byte_t*, size_t)
{ throw InvalidStateException(LOCATION, this->Name()); }

////////////////////////////////////////////////////////////////////////////////////
//	Class PLLS_SecNotResetIdle
////////////////////////////////////////////////////////////////////////////////////

PLLS_SecNotReset PLLS_SecNotReset::mInstance;

void PLLS_SecNotReset::SendUnconfirmed(AsyncLinkLayer* apLL, const apl::byte_t* apData, size_t aLength)
{	
	apLL->SendUnconfirmedUserData(apData, aLength);

}

void PLLS_SecNotReset::SendConfirmed(AsyncLinkLayer* apLL, const apl::byte_t* apData, size_t aLength)
{
	apLL->ResetRetry();
	apLL->StartTimer();
	apLL->ChangeState(PLLS_ResetLinkWait::Inst());
		
	// what we'll send if we successfully reset link state
	apLL->mDelayedPriFrame.FormatConfirmedUserData(apLL->mCONFIG.IsMaster, true, apLL->mCONFIG.RemoteAddr, apLL->mCONFIG.LocalAddr, apData, aLength);
	apLL->SendResetLinks();
}

////////////////////////////////////////////////////////////////////////////////////
//	Class PLLS_SecReset
////////////////////////////////////////////////////////////////////////////////////

PLLS_SecReset PLLS_SecReset::mInstance;

void PLLS_SecReset::SendUnconfirmed(AsyncLinkLayer* apLL, const apl::byte_t* apData, size_t aLength)
{	
	apLL->SendUnconfirmedUserData(apData, aLength);
}

void PLLS_SecReset::SendConfirmed(AsyncLinkLayer* apLL, const apl::byte_t* apData, size_t aLength)
{
	apLL->ResetRetry();
	apLL->StartTimer();
	apLL->ChangeState(PLLS_ConfDataWait::Inst());
	
	apLL->mDelayedPriFrame.FormatConfirmedUserData(apLL->mCONFIG.IsMaster, apLL->NextWriteFCB(), apLL->mCONFIG.RemoteAddr, apLL->mCONFIG.LocalAddr, apData, aLength);
	apLL->SendDelayedUserData(apLL->NextWriteFCB());
}

////////////////////////////////////////////////////////////////////////////////////
//	Class PLLS_ResetLinkWait
////////////////////////////////////////////////////////////////////////////////////

PLLS_ResetLinkWait PLLS_ResetLinkWait::mInstance;

void PLLS_ResetLinkWait::Ack(AsyncLinkLayer* apLL, bool aIsRcvBuffFull)
{
	apLL->ResetWriteFCB();
	apLL->CancelTimer();
	apLL->StartTimer();
	apLL->ChangeState(PLLS_ConfDataWait::Inst());
	apLL->SendDelayedUserData(apLL->NextWriteFCB());
}

void PLLS_ResetLinkWait::OnTimeout(AsyncLinkLayer* apLL)
{
	if(apLL->Retry()) {
		ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Confirmed data timeout, retrying, " << apLL->RetryRemaining() << " remaining", DLERR_TIMEOUT_RETRY);		
		apLL->StartTimer();
		apLL->SendResetLinks();
	}
	else {
		ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Confirmed data final timeout", DLERR_TIMEOUT_NO_RETRY);
		apLL->ChangeState(PLLS_SecNotReset::Inst());
		apLL->DoSendFailure();
	}
}

void PLLS_ResetLinkWait::Failure(AsyncLinkLayer* apLL)
{
	apLL->CancelTimer();
	apLL->ChangeState(PLLS_SecNotReset::Inst());
	apLL->DoSendFailure();
}

////////////////////////////////////////////////////////////////////////////////////
//	Class PLLS_ConfDataWait
////////////////////////////////////////////////////////////////////////////////////

PLLS_ConfDataWait PLLS_ConfDataWait::mInstance;

void PLLS_ConfDataWait::Ack(AsyncLinkLayer* apLL, bool aIsRcvBuffFull)
{
	apLL->ToggleWriteFCB();
	apLL->CancelTimer();
	apLL->ChangeState(PLLS_SecReset::Inst());
	apLL->DoSendSuccess();
}

void PLLS_ConfDataWait::Failure(AsyncLinkLayer* apLL)
{
	apLL->CancelTimer();
	apLL->ChangeState(PLLS_SecNotReset::Inst());
	apLL->DoSendFailure();
}

void PLLS_ConfDataWait::OnTimeout(AsyncLinkLayer* apLL)
{
	if(apLL->Retry()) {
		ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Retry confirmed data", DLERR_TIMEOUT_RETRY);
		apLL->StartTimer();
		apLL->ChangeState(PLLS_ConfDataWait::Inst());
		apLL->SendDelayedUserData(apLL->NextWriteFCB());
	}
	else {
		ERROR_LOGGER_BLOCK(apLL->GetLogger(), LEV_WARNING, "Confirmed data timeout", DLERR_TIMEOUT_NO_RETRY);
		apLL->ChangeState(PLLS_SecNotReset::Inst());
		apLL->DoSendFailure();
	}
}

}} //end namepsace
