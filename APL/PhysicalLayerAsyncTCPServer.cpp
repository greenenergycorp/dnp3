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
#include "ASIOIncludes.h"
#include "PhysicalLayerAsyncTCPServer.h"

#include <boost/bind.hpp>
#include <string>

#include "Exception.h"
#include "IHandlerAsync.h"
#include "Logger.h"

using namespace boost;
using namespace boost::system;
using namespace boost::asio;
using namespace std;

namespace apl {

PhysicalLayerAsyncTCPServer::PhysicalLayerAsyncTCPServer(Logger* apLogger, io_service* apIOService, uint_16_t aPort) :
PhysicalLayerAsyncBaseTCP(apLogger, apIOService),
mEndpoint(ip::tcp::v4(), aPort),
mAcceptor(*apIOService)
{
	
}

/* Implement the actions */
void PhysicalLayerAsyncTCPServer::DoOpen()
{
	if(!mAcceptor.is_open())
	{
		error_code ec;
		mAcceptor.open(mEndpoint.protocol(), ec);
		if(ec) throw Exception(LOCATION, ec.message());

		mAcceptor.set_option(ip::tcp::acceptor::reuse_address(true));
		mAcceptor.bind(mEndpoint, ec);
		if(ec) throw Exception(LOCATION, ec.message());

		mAcceptor.listen(socket_base::max_connections, ec);
		if(ec) throw Exception(LOCATION, ec.message());
	}
	
	mAcceptor.async_accept(mSocket, mEndpoint, boost::bind(&PhysicalLayerAsyncTCPServer::OnOpenCallback, this, placeholders::error));
}

void PhysicalLayerAsyncTCPServer::DoOpeningClose()
{
	mAcceptor.cancel();
}

}

