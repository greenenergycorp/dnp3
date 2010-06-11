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
#ifndef __DNP_CONSTANTS_H_
#define __DNP_CONSTANTS_H_

#include <string>

//controls the maximum DNP fragment size

#define MACRO_DNP_RADIX(obj,var) (obj<<4) | var

namespace apl { namespace dnp {

	const size_t DEFAULT_FRAG_SIZE = 2048;

	enum DNPErrorCodes
	{
		// Slave error codes

		SERR_FUNC_NOT_SUPPORTED,
		SERR_CTRL_NOT_CONVERTIBLE,
		SERR_MULTIPLE_HDR_INVALID,
		SERR_MULTIPLE_OBJ_INVALID,
		SERR_OBJ_FUNC_MISMATCH,
		SERR_INVALID_IIN_WRITE,

		// Master error codes
		MERR_FUNC_NOT_SUPPORTED,
		MERR_UNEXPECTED_MULTI_FRAG_RSP,

		// APPLICATION Layer Context

		ALERR_TIMEOUT,
		ALERR_BAD_UNSOL_BIT,
		ALERR_UNEXPECTED_CONFIRM,
		ALERR_UNEXPECTED_RESPONSE,
		ALERR_MULTI_FRAGEMENT_REQUEST,
		ALERR_NOT_FIR_PACKET,
		ALERR_BAD_FIR_FIN,
		ALERR_BAD_SEQUENCE,
		ALERR_BAD_CONFIRM_MESSAGE,
		ALERR_FRAGMENT_TOO_LARGE,
		ALERR_UNSOL_FLOOD,
		ALERR_SOL_FLOOD,
		ALERR_INVALID_PACKET,
		ALERR_BAD_STATE,

		// Application Layer Parsing

		ALERR_INSUFFICIENT_DATA_FOR_FRAG,			// fragment is less than 2 bytes
		ALERR_INSUFFICIENT_DATA_FOR_RESPONSE,		// response indicated but < 4 bytes of data
		ALERR_INSUFFICIENT_DATA_FOR_HEADER,			// not enough to read object header
		ALERR_INSUFFICIENT_DATA_FOR_OBJECTS,		// not enough to read the indicated objects
		ALERR_UNKNOWN_GROUP_VAR,					// encounter an unknown group var while parsing
		ALERR_START_STOP_MISMATCH,					// Start > Stop in a ranged header
		ALERR_ILLEGAL_QUALIFIER_AND_OBJECT,			// The qualifier/object combination is invalid
		ALERR_UNKNOWN_QUALIFIER,
		ALERR_ILLEGAL_RANGE_AND_PREFIX,
		ALERR_ITERATOR_OUT_OF_BOUNDS,
		ALERR_ITERATOR_NO_DATA,


		/* TRANSPORT Layer */

		TLERR_NEW_FIR,
		TLERR_MESSAGE_WITHOUT_FIR, 
		TLERR_BAD_SEQUENCE, 
		TLERR_BAD_LENGTH, 
		TLERR_BUFFER_FULL,
		TLERR_TIMEOUT,

		//used in linkframeparser
		DLERR_CRC,				//	CRC failure in header or data payload
		DLERR_INVALID_LENGTH,	//	length parameter outside range [0,255]
		DLERR_UNEXPECTED_DATA,	//  length > 5 for non-user data func code	
		DLERR_NO_DATA,			//  length == 5 but func code indicates data
		DLERR_UNKNOWN_FUNC,		//  unknown function code
		DLERR_UNEXPECTED_FCV,	//	FCV bit set unexpectedly, ie set on wrong func code
		DLERR_UNEXPECTED_FCB,	//  FCB set unexpectedly

		//user in datalinklayer
		DLERR_UNKNOWN_DESTINATION,
		DLERR_UNKNOWN_SOURCE,
		DLERR_CONFIRM_NOT_RECEIVED,
		DLERR_NACK_RECEIVED,
		DLERR_SEND_FAILED,
		DLERR_REMOTE_LINKLAYER_NOT_SUPPORTED,
		DLERR_REMOTE_BUFFERS_FULL_TIMEOUT,
		DLERR_WRONG_FCB_ON_RECEIVE_DATA,
		DLERR_WRONG_FCB_ON_TEST,
		DLERR_UNEXPECTED_ACK,
		DLERR_UNEXPECTED_FRAME,
		DLERR_TIMEOUT_RETRY,
		DLERR_TIMEOUT_NO_RETRY,
		DLERR_MASTER_BIT_MATCH,
		DLERR_PRI_IS_FLOODING,

		//Used all over the place
		ERR_INDEX_OUT_OF_BOUNDS


	};
	
}}

#endif
