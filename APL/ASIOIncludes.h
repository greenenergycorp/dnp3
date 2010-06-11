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
#ifndef __ASIO_INCLUDES_H_
#define __ASIO_INCLUDES_H_

#include "Configure.h"

#ifdef APL_CYGWIN
#define __USE_W32_SOCKETS
#define _WIN32_WINNT 0x0501
#endif

#ifdef APL_PLATFORM_WIN
#define _WIN32_WINNT 0x0501
#ifdef _DEBUG
#define __USE_W32_SOCKETS
#endif
#endif

#ifdef ARM
#define BOOST_ASIO_DISABLE_EPOLL
#endif

#define BOOST_ASIO_ENABLE_CANCELIO
#define BOOST_REGEX_NO_LIB
#include <boost/asio.hpp>

#endif
