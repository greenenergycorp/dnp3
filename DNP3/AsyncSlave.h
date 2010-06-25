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
#ifndef __ASYNC_SLAVE_H_
#define __ASYNC_SLAVE_H_

#include <APL/Loggable.h>
#include <APL/Logger.h>
#include <APL/TimeSource.h>
#include <APL/Lock.h>
#include <APL/PostingNotifierSource.h>
#include <APL/ChangeBuffer.h>
#include <APL/CommandResponseQueue.h>
#include <APL/CachedLogVariable.h>

#include "AsyncAppInterfaces.h"
#include "APDU.h"
#include "AsyncResponseContext.h"
#include "AsyncSlaveEventBuffer.h"
#include "SlaveConfig.h"
#include "SlaveResponseTypes.h"
#include "ObjectReadIterator.h"
#include "DNPCommandMaster.h"

namespace apl
{
	class ITimerSource;
}

namespace apl { namespace dnp {

class AS_Base;

/** @section desc DNP3 outstation.

	Manages a state machine that handles events from the user layer and the application layer to provide DNP outstation services.

	ResponseContext and AsyncSlaveEventBuffer objects manage data/event responses to master requests, and the IDNPCommandMaster
	implementation verifies control/setpoint behavior and passes valid commands to the user code.

	SlaveConfig structure represents the slave behavioral configuration, the AsyncDatabase is in charge of the data model itself.

	Global IIN state is maintained and combined with request-specific information to form response IINs.

	The AsyncSlave is responsible for building all aspects of APDU packet responses except for the application sequence number.

*/
class AsyncSlave : public Loggable, public IAsyncAppUser
{
	
	enum CommsStatus
	{
		COMMS_DOWN = 0,
		COMMS_UP = 2
	};

	friend class AS_Base; //make the state base class a friend
	friend class AS_OpenBase;
	friend class AS_Closed;
	friend class AS_Idle;
	friend class AS_WaitForRspSuccess;
	friend class AS_WaitForUnsolSuccess;
	friend class AS_WaitForSolUnsolSuccess;

	public:

	AsyncSlave(Logger*, IAsyncAppLayer*, ITimerSource*, ITimeManager* apTime, AsyncDatabase*, IDNPCommandMaster*, const SlaveConfig& arCfg);
	virtual ~AsyncSlave() {}


	///////////////////////////////////
	// External events
	///////////////////////////////////

	/* Implement IAsyncAppUser - callbacks from the app layer */
	void OnLowerLayerUp();
	void OnLowerLayerDown();

	void OnUnsolSendSuccess();
	void OnSolSendSuccess();

	void OnUnsolFailure();
	void OnSolFailure();

	// Only have to override OnRequest since we're a slave
	void OnRequest(const APDU&, SequenceInfo);
	void OnUnknownObject();

	///////////////////////////////////
	// End - External events
	///////////////////////////////////

	bool IsMaster() { return false; }

	IDataObserver* GetDataObserver() { return &mChangeBuffer; }

	private:

	ChangeBuffer<SigLock> mChangeBuffer;	/// how client code gives us updates
	PostingNotifierSource mNotifierSource;	/// way to get special notifiers for the change queue / vto
	IAsyncAppLayer* mpAppLayer;				/// lower application layer
	ITimerSource* mpTimerSrc;				/// used for post and timers
	AsyncDatabase* mpDatabase;				/// holds static data
	IDNPCommandMaster* mpCmdMaster;			/// how commands are selected/operated
	int mSequence;							/// control sequence
	CommandResponseQueue mRspQueue;			/// how command responses are received
	AS_Base* mpState;						/// current state for the state pattern
	SlaveConfig mConfig;					/// houses the configurable paramters of the outstation
	SlaveResponseTypes mRspTypes;			/// converts the group/var in the config to dnp singletons

	ITimer* mpUnsolTimer;					/// timer for sending unsol responsess

	IINField mIIN;							/// IIN bits that persist between requests (i.e. NeedsTime/Restart/Etc)
	IINField mRspIIN;						/// Transient IIN bits that get merged before a response is issued
	APDU mResponse;							/// APDU used to form responses
	APDU mRequest;							/// APDU used to save Deferred requests
	SequenceInfo mSeqInfo;
	APDU mUnsol;							/// APDY used to form unsol respones
	AsyncResponseContext mRspContext;		/// Used to track and construct response fragments

	bool mHaveLastRequest;
	APDU mLastRequest;						/// APDU used to form responses

	ITimeManager* mpTime;
	CachedLogVariable mCommsStatus;

	// Flags that tell us that some action has been Deferred
	// until the slave is in a state capable of handling it.

	bool mDeferredUpdate;					/// Indicates that a data update has been Deferred
	bool mDeferredRequest;					/// Indicates that a request has been Deferred
	bool mDeferredUnsol;						/// Indicates that the unsol timer expired, but the event was Deferred
	bool mDeferredUnknown;

	bool mStartupNullUnsol;					/// Tracks whether the device has completed the NULL unsol startup message

	void OnDataUpdate();					/// internal event dispatched when user code commits an update to mChangeBuffer
	void OnUnsolTimerExpiration();			/// internal event dispatched when the unsolicted pack/retry timer expires

	void ConfigureAndSendSimpleResponse();
	void Send(APDU&);
	void Send(APDU& arAPDU, const IINField& arIIN); /// overload with additional IIN data
	void SendUnsolicited(APDU& arAPDU);

	void HandleWrite(const APDU& arRequest);
	void HandleWriteIIN(HeaderReadIterator& arHdr);
	void HandleWriteTimeDate(HeaderReadIterator& arHWI);
	void HandleSelect(const APDU& arRequest, SequenceInfo aSeqInfo);
	void HandleOperate(const APDU& arRequest, SequenceInfo aSeqInfo);
	void HandleDirectOperate(const APDU& arRequest, SequenceInfo aSeqInfo);
	void HandleEnableUnsolicited(const APDU& arRequest, bool aIsEnable);
	void HandleUnknown();


	void ConfigureDelayMeasurement(const APDU& arRequest);
	void CreateResponseContext(const APDU& arRequest);

	// Helpers

	size_t FlushUpdates();
	void FlushDeferredEvents();
	void StartUnsolTimer(millis_t aTimeout);

	// Task handlers

	void ResetTimeIIN();
	ITimer* mpTimeTimer;

	/// C++ doesn't have templated typedefs, so this will suffice
	template <class T>
	struct CommandFunc
	{
		typedef boost::function<CommandStatus (T&, size_t)> Type;
	};

	/**

		@param apObj DNP3 object capable of reading and writing its own type from stream
		@param arIter Iterator to read objects
		@param aFunc Function for issuing/selecting
	*/
	template <class T>
	void RespondToCommands(const StreamObject<T>* apObj, ObjectReadIterator& arIter, typename CommandFunc<T>::Type aFunc);

	template <class T>
	CommandStatus Select(T& arCmd, size_t aIndex, const HeaderInfo& aHdr, SequenceInfo aSeqInfo, int aAPDUSequence);

	template <class T>
	CommandStatus Operate(T& arCmd, size_t aIndex, bool aDirect, const HeaderInfo& aHdr, SequenceInfo aSeqInfo, int aAPDUSequence);

};

template<class T>
void AsyncSlave::RespondToCommands(const StreamObject<T>* apObj, ObjectReadIterator& arIter, typename CommandFunc<T>::Type arFunc)
{
	IndexedWriteIterator i = mResponse.WriteIndexed(apObj, arIter.Count(), arIter.Header().GetQualifier());
	size_t count = 1;
	while(!arIter.IsEnd())
	{
		T val = apObj->Read(*arIter);
		size_t index = arIter->Index();
		if ( count > mConfig.mMaxControls )
			val.mStatus = CS_TOO_MANY_OPS;
		else
			val.mStatus = arFunc(val, index);
		i.SetIndex(index);
		apObj->Write(*i, val);
		++i;
		++arIter;
		++count;
	}
}

template <class T>
CommandStatus AsyncSlave::Select(T& arCmd, size_t aIndex, const HeaderInfo& aHdr, SequenceInfo aSeqInfo, int aAPDUSequence)
{
	CommandStatus res = mpCmdMaster->Select(CommandRequestInfo<T>(arCmd, aHdr.GetObjectType(), aHdr.GetVariation(), aHdr.GetQualifier(), aSeqInfo, aAPDUSequence), aIndex) ? CS_SUCCESS : CS_NOT_SUPPORTED;
	LOG_BLOCK(LEV_INFO, "Selecting " << arCmd.ToString() << " Index: " << aIndex << " Result: " << ToString(res));
	if ( res == CS_NOT_SUPPORTED )
		mRspIIN.SetParameterError(true);
	if ( res == CS_TOO_MANY_OPS )
		mpCmdMaster->DeselectAll(); // 4.4.3 rule 3
	return res;
}

template <class T>
CommandStatus AsyncSlave::Operate(T& arCmd, size_t aIndex, bool aDirect, const HeaderInfo& aHdr, SequenceInfo aSeqInfo, int aAPDUSequence)
{
	++mSequence;
	CommandStatus res;
	if ( aDirect )
		res = mpCmdMaster->DirectOperate(CommandRequestInfo<T>(arCmd, aHdr.GetObjectType(), aHdr.GetVariation(), aHdr.GetQualifier(), aSeqInfo, aAPDUSequence), aIndex, mSequence);
	else
		res = mpCmdMaster->Operate(CommandRequestInfo<T>(arCmd, aHdr.GetObjectType(), aHdr.GetVariation(), aHdr.GetQualifier(), aSeqInfo, aAPDUSequence), aIndex, mSequence);

	if(res != CS_SUCCESS)
	{
		if ( res == CS_NOT_SUPPORTED )
			mRspIIN.SetParameterError(true);
		return res;
	}
	else
	{
		CommandResponse cr(CS_HARDWARE_ERROR);
		mRspQueue.WaitForResponse(cr, mSequence); //wait forever on a response from user space
		LOG_BLOCK(LEV_INFO, arCmd.ToString() << " Index: " << aIndex << " Result: " << ToString(cr.mResult));
		return cr.mResult;
	}
}


}}

#endif
