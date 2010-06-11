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
#include "PhysicalLayerAsyncTCPClient.h"


#include <boost/bind.hpp>
#include <string>

#include "Exception.h"
#include "IHandlerAsync.h"
#include "Logger.h"

using namespace boost;
using namespace boost::asio;
using namespace std;

namespace apl {

PhysicalLayerAsyncTCPClient::PhysicalLayerAsyncTCPClient(
	Logger* apLogger,
	boost::asio::io_service* apIOService,
	const std::string& arAddress,
	uint_16_t aPort) :
	
	PhysicalLayerAsyncBaseTCP(apLogger, apIOService),
	mAddr(arAddress),
	mPort(aPort)
{

}

/* Implement the actions */
void PhysicalLayerAsyncTCPClient::DoOpen()
{
	ip::address_v4 address;
	boost::system::error_code ec;
	string ipstring(mAddr);
	address = address.from_string(ipstring, ec);
	if(ec) throw ArgumentException(LOCATION, "string Address: " + ipstring + " cannot be resolved");
	
	ip::tcp::endpoint serverEndpoint(address, mPort);

	mSocket.async_connect(serverEndpoint, boost::bind(&PhysicalLayerAsyncTCPClient::OnOpenCallback, this, placeholders::error));
}

}

