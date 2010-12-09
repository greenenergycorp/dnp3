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

#ifndef __I_PHYSICAL_LAYER_SOURCE_H_
#define __I_PHYSICAL_LAYER_SOURCE_H_


#include <map>
#include <string>
#include <boost/function.hpp>

#include "Exception.h"
#include "Types.h"
#include "PhysicalLayerFactory.h"
#include "LogBase.h"

namespace boost { namespace asio { class io_service; }}

namespace apl
{
	class Logger;
	class IPhysicalLayerAsync;

	struct PhysLayerSettings;

	class IPhysicalLayerSource
	{
		public:
			virtual ~IPhysicalLayerSource(){}

			virtual PhysLayerSettings GetSettings(const std::string& arName) = 0;
			virtual IPhysicalLayerAsync* GetLayer(const std::string& arName, boost::asio::io_service*) = 0;
	};


}


#endif
