#include "ControlTerminalExtension.h"

#include <APL/FlexibleDataObserver.h>
#include <APL/CommandInterfaces.h>
#include <APL/CommandTypes.h>
#include <APL/CommandResponseQueue.h>
#include <APL/Util.h>
#include <APL/Parsing.h>

#include <boost/bind.hpp>
#include <sstream>

using namespace std;
using namespace boost;

namespace apl
{
	typedef boost::function<std::string(byte_t)> QualityFunctor;

	void ControlTerminalExtension::_BindToTerminal(ITerminal* apTerminal)
	{
		CommandNode cmd;
		
		cmd.mName = "issue bo";
		cmd.mUsage = "issue bo <index> <code> <on time> <off time> <count> \r\n";
		cmd.mUsage += "code = <pon | lon | loff | pclose | ptrip>";
		cmd.mDesc = "Issues a binary output command. ";
		cmd.mHandler = boost::bind(&ControlTerminalExtension::HandleIssueBO, this, _1);
		apTerminal->BindCommand(cmd, "issue bo");

		cmd.mName = "issue st";
		cmd.mUsage = "issue st <index> <value>";
		cmd.mDesc = "Issues a setpoint request. If value has a \'.\' it is treated as a double.";
		cmd.mHandler = boost::bind(&ControlTerminalExtension::HandleIssueST, this, _1);
		apTerminal->BindCommand(cmd, "issue st");
		
	}

	void ControlTerminalExtension::WaitForResponse(){
		this->Send("Waiting for response... ");
		CommandResponse rsp;
		bool success = mRspQueue.WaitForResponse(rsp, mSequence, 5000);
		ostringstream oss;
		if(success)
		{
			oss << "Result: " << ToString(rsp.mResult) << ITerminal::EOL;
			this->Send(oss.str());
		}
		else 
		{
			oss << "Timeout" << ITerminal::EOL;
			this->Send(oss.str());
		}
	}


	retcode ControlTerminalExtension::HandleIssueST(std::vector<std::string>& arArgs)
	{
		if(arArgs.size() < 2) return BAD_ARGUMENTS;

		Setpoint st;

		int index;

		if(!Parsing::GetPositive(arArgs[0], index)) return BAD_ARGUMENTS;
		if(arArgs[1].find('.') == std::string::npos){
			int iValue;
			if(!Parsing::Get(arArgs[1], iValue)) return BAD_ARGUMENTS;
			st.SetValue(static_cast<int_32_t>(iValue));
		}else{
			double dValue;
			if(!Parsing::Get(arArgs[1], dValue)) return BAD_ARGUMENTS;
			st.SetValue(dValue);
		}

		mpCmdAcceptor->AcceptCommand(st, static_cast<size_t>(index), ++mSequence, &mRspQueue);

		WaitForResponse();

		return SUCCESS;
	}

	uint_32_t ParseControlCode( const std::string& arString )
	{
		std::string lower(arString);
		toLowerCase(lower);

		if ( lower.compare("pon") == 0 )
		{
			return CC_PULSE;
		}
		else if ( lower.compare("lon") == 0 )
		{
			return CC_LATCH_ON;
		}
		else if ( lower.compare("loff") == 0 )
		{
			return CC_LATCH_OFF;
		}
		else if ( lower.compare("pclose") == 0 )
		{
			return CC_PULSE_CLOSE;
		}
		else if ( lower.compare("ptrip") == 0 )
		{
			return CC_PULSE_TRIP;
		}
		else
		{
			stringstream ss;
			ss << arString;
			int val = 0;
			ss >> val;
			return val;
		}
	}

	retcode ControlTerminalExtension::HandleIssueBO(std::vector<std::string>& arArgs)
	{
		if(arArgs.size() < 2) return BAD_ARGUMENTS;

		BinaryOutput b;
		b.mCount = 1;
		b.mOffTimeMS = 100;
		b.mOnTimeMS = 1000;
		b.mStatus = CS_SUCCESS;

		uint_32_t index = 0;
		uint_32_t code = 0;
		uint_32_t ontime = 0;
		uint_32_t offtime = 0;
		uint_32_t count = 0;
		
		stringstream ss;
		ss << arArgs[0];
		ss >> index;
		ss.clear();
		code = ParseControlCode(arArgs[1]);

		if ( code == CC_LATCH_ON || code == CC_LATCH_OFF )
		{
			ontime = 0;
			offtime = 0;
			count = 1;
		}
		else
		{
			if(arArgs.size() < 5) return BAD_ARGUMENTS;

			ss << arArgs[2]; 
			ss >> ontime;
			ss.clear();
			ss << arArgs[3];
			ss >> offtime;
			ss.clear();
			ss << arArgs[4];
			ss >> count;
		}

		b.mOffTimeMS = offtime;
		b.mOnTimeMS = ontime;
		b.mCount = static_cast<ControlCode>(count);
		b.mRawCode = static_cast<ControlCode>(code);

		mpCmdAcceptor->AcceptCommand(b, index, ++mSequence, &mRspQueue);

		WaitForResponse();

		return SUCCESS;
	}

	
}

