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
#ifndef __TCP_TYPES_H_
#define __TCP_TYPES_H_

#include "Types.h"

#include <string>

namespace apl {

	struct TCPSettings
	{
		TCPSettings(const std::string aAddress,
			const uint_16_t aPort,
			const bool aEnableKeepalive = false,
			const int aKeepaliveTime = 0,
			const int aKeepaliveInterval = 0,
			const int aKeepaliveProbes = 0);

		std::string mAddress;
		uint_16_t mPort;
		bool mEnableKeepalive;
		int mKeepaliveTime;
		int mKeepaliveInterval;
		int mKeepaliveProbes;
	};

}

#endif

