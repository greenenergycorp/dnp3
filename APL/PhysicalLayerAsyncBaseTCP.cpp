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
#include "ASIOIncludes.h"
#include "PhysicalLayerAsyncBaseTCP.h"

#include <boost/bind.hpp>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/detail/socket_option.hpp>

#include "Exception.h"
#include "IHandlerAsync.h"
#include "Logger.h"

// BSD uses IPPROTO_TCP instead of SOL_TCP.
#if !defined(BOOST_WINDOWS)
#  ifndef SOL_TCP
#    define SOL_TCP IPPROTO_TCP
#  endif
#endif

using namespace boost;
using namespace boost::asio;
using namespace boost::system;
using namespace std;

namespace apl {

PhysicalLayerAsyncBaseTCP::PhysicalLayerAsyncBaseTCP(Logger* apLogger, boost::asio::io_service* apIOService, TCPSettings aTcp) :
PhysicalLayerAsyncASIO(apLogger, apIOService),
mSocket(*apIOService),
mTcp(aTcp)
{
}

/* Implement the actions */

void PhysicalLayerAsyncBaseTCP::DoClose()
{
	error_code ec;
	mSocket.close(ec);
	if(ec) LOG_BLOCK(LEV_WARNING, ec.message());
}

void PhysicalLayerAsyncBaseTCP::DoOpenSuccess()
{
	LOG_BLOCK(LEV_INFO, "Successful connection");

	if(mTcp.mEnableKeepalive)
	{
		const boost::asio::socket_base::keep_alive keepaliveOption(true);
		mSocket.set_option(keepaliveOption);

#if defined(BOOST_WINDOWS)
		LOG_BLOCK(LEV_WARNING, "Per-socket keepalive settings are not yet implemented for Windows.");

		// TODO: It looks like this could be implemented via the SIO_KEEPALIVE_VALS WSAIoctl():
		//
		//           http://msdn.microsoft.com/en-us/library/dd877220%28v=VS.85%29.aspx
#else
		const boost::asio::detail::socket_option::integer<SOL_TCP, TCP_KEEPIDLE> keepaliveTimeOption(mTcp.mKeepaliveTime);
		const boost::asio::detail::socket_option::integer<SOL_TCP, TCP_KEEPINTVL> keepaliveIntervalOption(mTcp.mKeepaliveInterval);
		const boost::asio::detail::socket_option::integer<SOL_TCP, TCP_KEEPCNT> keepaliveProbesOption(mTcp.mKeepaliveProbes);
		mSocket.set_option(keepaliveTimeOption);
		mSocket.set_option(keepaliveIntervalOption);
		mSocket.set_option(keepaliveProbesOption);
#endif
	}
}

void PhysicalLayerAsyncBaseTCP::DoAsyncRead(byte_t* apBuffer, size_t aMaxBytes)
{
	mSocket.async_read_some(buffer(apBuffer, aMaxBytes),
		boost::bind(&PhysicalLayerAsyncBaseTCP::OnReadCallback, this, placeholders::error, apBuffer, placeholders::bytes_transferred));
}

void PhysicalLayerAsyncBaseTCP::DoAsyncWrite(const byte_t* apBuffer, size_t aNumBytes)
{
	async_write(mSocket, buffer(apBuffer, aNumBytes), 
		boost::bind(&PhysicalLayerAsyncBaseTCP::OnWriteCallback, this, placeholders::error, aNumBytes));
}

void PhysicalLayerAsyncBaseTCP::DoOpenFailure()
{
	LOG_BLOCK(LEV_INFO, "Failed socket open, re-closing");
	DoClose();
}

}

