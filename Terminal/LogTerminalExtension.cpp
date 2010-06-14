#include "LogTerminalExtension.h"

#include <APL/Util.h>
#include <APL/TimerInterfaces.h>

#include <boost/bind.hpp>
#include <sstream>
#include <set>


using namespace std;
using namespace boost;

#ifdef GetMessage
	#undef GetMessage
#endif

namespace apl
{

	LogTerminalExtension::LogTerminalExtension(EventLog* apLog) : 
	mpLog(apLog),	
	mpFileLogger(NULL)
	{
		this->ResetActiveColumns();
		mpLog->AddLogSubscriber(&mBuffer);
		mpLog->AddLogSubscriber(&mMetrics);
		mBuffer.AddIgnoreCode(ITerminal::TERMINAL_CODE);
	}

	LogTerminalExtension::~LogTerminalExtension() {
		mpLog->RemoveLogSubscriber(&mMetrics);
		mpLog->RemoveLogSubscriber(&mBuffer);		
		StopLoggingToFile();
	}

	void LogTerminalExtension::Notify()
	{
		this->Post(boost::bind(&LogTerminalExtension::HandleLogTimer, this));
	}

	void LogTerminalExtension::RedirectedLine(const std::string&)
	{
		mBuffer.RemoveObserver(this);
		this->ClearRedirect();
	}

	void LogTerminalExtension::HandleLogTimer()
	{
		this->PrintLog();
	}

	void LogTerminalExtension::_BindToTerminal(ITerminal* apTerminal)
	{
		////////////////////////////////////
		// Print routines
		////////////////////////////////////

		CommandNode cmd;

		// PRINT LOG
		cmd.mName = "log";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandlePrintLog, this, _1);
		cmd.mUsage = "print log [device name]";
		cmd.mDesc = "Prints the current contents of the log";
		apTerminal->BindCommand(cmd, "print log");

		// PRINT LOGGERS
		cmd.mName = "loggers";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandlePrintLoggers, this, _1);
		cmd.mUsage = "print loggers";
		cmd.mDesc = "Prints out all registered logger names";
		apTerminal->BindCommand(cmd, "print loggers");

		////////////////////////////////////
		// run
		////////////////////////////////////

		cmd.mName = "log";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandleRunLog, this, _1);
		cmd.mUsage = "run log";
		cmd.mDesc = "Continuously outputs log entries as they occur.";
		apTerminal->BindCommand(cmd, "run log");

		////////////////////////////////////
		// set
		////////////////////////////////////

		cmd.mName = "filter";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandleSetFilterOrLevel, this, _1, false);
		cmd.mUsage = "set filter [a|d|i|c|p|w|e|v|n] <devicename1> <devicename2> ...\r\n";
		cmd.mUsage += "a=ALL, d=DEBUG, i=INFO, c=COMM, p=Interpret, w=WARNING, e=ERROR, v=EVENT";
		cmd.mDesc = "Set or clear log filters, more than one filter can be set at a time (Ex: wev). If no devices are specified, all devices are affected.";
		apTerminal->BindCommand(cmd, "set filter");

		cmd.mName = "level";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandleSetFilterOrLevel, this, _1, true);
		cmd.mUsage = "set filter [a|d|i|c|p|w|e|v|n] <devicename1> <devicename2> ...\r\n";
		cmd.mUsage += "a=ALL, d=DEBUG, i=INFO, c=COMM, p=Interpret, w=WARNING, e=ERROR, v=EVENT";
		cmd.mDesc = "Set or clear log filter level, all \"higher\" filters are also set (Ex: \"set level w\" is equivilant to \"set filter wev\"). If no devices are specified, all devices are affected.";
		apTerminal->BindCommand(cmd, "set level");

		cmd.mName = "logcol";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandleSetLogCol, this, _1);
		cmd.mUsage = "set logcol [t|f|d|l|m] \r\n";
		cmd.mUsage += "t=Time, f=Filter, d=Device, l=Location, m=Message";
		cmd.mDesc = "Sets the column order for displaying log entries";
		apTerminal->BindCommand(cmd, "set logcol");

		////////////////////////////////////
		// clear
		////////////////////////////////////

		cmd.mName = "screen";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandleClearScreen, this, _1);
		cmd.mUsage = "clear screen";
		cmd.mDesc = "Clears the screen.";
		apTerminal->BindCommand(cmd, "clear screen");

		cmd.mName = "start";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandleStartToFileLogging, this, _1);
		cmd.mUsage = "dump start [filename]";
		cmd.mDesc = "Starts a log file with the given file name";
		apTerminal->BindCommand(cmd, "dump start");

		cmd.mName = "stop";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandleStopToFileLogging, this, _1);
		cmd.mUsage = "dump stop";
		cmd.mDesc = "Stops logging to file (assuming it is currently logging to a file)";
		apTerminal->BindCommand(cmd, "dump stop");

		cmd.mName = "vars";
		cmd.mHandler = boost::bind(&LogTerminalExtension::HandlePrintVars, this, _1);
		cmd.mUsage = "print vars";
		cmd.mDesc = "Prints non-operational information to the console";
		apTerminal->BindCommand(cmd, "print vars");
	}

	void LogTerminalExtension::ResetActiveColumns()
	{
		this->mActiveColumns.clear();

		this->mActiveColumns.push_back(COL_TIME);
		this->mActiveColumns.push_back(COL_FILTER);
		this->mActiveColumns.push_back(COL_DEVICE);		
		this->mActiveColumns.push_back(COL_MESSAGE);
	}

	retcode LogTerminalExtension::HandlePrintLoggers(std::vector<std::string>& arTokens)
	{
		if(arTokens.size() > 0) return BAD_ARGUMENTS;

		vector<Logger*> loggers;
		mpLog->GetAllLoggers(loggers);

		ostringstream oss;
		for(size_t i=0; i<loggers.size(); i++)
		{
			oss << loggers[i]->GetName() << "\r\n";
		}

		this->Send(oss.str());

		return SUCCESS;
	}

	retcode LogTerminalExtension::HandlePrintVars(std::vector<std::string>& arTokens)
	{
		if(arTokens.size() > 0) return BAD_ARGUMENTS;

		std::vector<MetricBuffer::Var> v;
		mMetrics.Read(v);
		ostringstream oss;
		for(size_t i=0; i<v.size(); i++) {
			oss << "Source: " << v[i].source << " Name: " << v[i].name << " Value: " << v[i].value << "\r\n";
		}

		this->Send(oss.str());

		return SUCCESS;
	}

	retcode LogTerminalExtension::HandleRunLog(vector<string>& arTokens)
	{
		mBuffer.AddObserver(this);
		this->SetRedirect(boost::bind(&LogTerminalExtension::RedirectedLine, this, _1));
		this->PrintLog();		
		return SUCCESS;
	}

	retcode LogTerminalExtension::HandlePrintLog(vector<string>&)
	{
		this->PrintLog();
		return SUCCESS;
	}

	void LogTerminalExtension::PrintLog()
	{
		LogEntry entry;
		while(mBuffer.ReadLog(entry))
		{
			string logstr = this->GetLogString(entry).append("\r\n");
			this->Send(logstr, false);
		}
	}

	retcode LogTerminalExtension::HandleSetLogCol(std::vector<std::string>& arTokens)
	{
		if(arTokens.size() != 1) return BAD_ARGUMENTS;

		std::set<LogColumns> columns;
		std::vector<LogColumns> add_columns;

		string code = arTokens[0];
		toLowerCase(code);
		
		for(size_t i=0; i<code.size(); i++)
		{
			LogColumns col;

			switch(code[i])
			{
				case('t'):
					col = COL_TIME;
					break;
				case('f'):
					col = COL_FILTER;
					break;
				case('d'):
					col = COL_DEVICE;
					break;
				case('l'):
					col = COL_LOCATION;
					break;
				case('m'):
					col = COL_MESSAGE;
					break;
				default: //unrecognized character
				{
					ostringstream oss;
					oss << "Unrecognized log column character: " << code[i] << "\r\n";
					this->Send(oss.str());
					return BAD_ARGUMENTS;
				}
			}

			if(columns.find(col) != columns.end())
			{
				ostringstream oss;
				oss << "Log column character specified more than once: " << code[i] << "\r\n";
				this->Send(oss.str());
				return BAD_ARGUMENTS;
			}
			else
			{
				add_columns.push_back(col);
				columns.insert(col);
			}
		}

		//if we made it this far, clear the active map and set the columns
		this->mActiveColumns.clear();
		for(size_t i=0; i<add_columns.size(); i++) mActiveColumns.push_back(add_columns[i]);
		
		
		return SUCCESS;
	}

	std::string LogTerminalExtension::GetLogString(LogEntry& arEntry)
	{
		ostringstream oss;
		for(size_t i=0; i<mActiveColumns.size(); i++)
		{
			switch(mActiveColumns[i])
			{
				case(COL_TIME):
					oss << arEntry.GetTimeString();
					break;
				case(COL_DEVICE):
					oss << arEntry.GetDeviceName();
					break;
				case(COL_FILTER):
					oss << GetFilterString(arEntry.GetFilterLevel());
					break;
				case(COL_LOCATION):
					oss << arEntry.GetLocation();
					break;
				case(COL_MESSAGE):
					oss << arEntry.GetMessage();
					break;
			}

			if(i != (mActiveColumns.size()-1)) oss << " - ";
		}
		return oss.str();
	}

	retcode LogTerminalExtension::HandleSetFilterOrLevel(std::vector<std::string>& arTokens, bool aSetLevel)
	{
		//need at least one token
		if(arTokens.size() == 0) return BAD_ARGUMENTS;

		//first token is the level flags
		int level = 0;
		bool success = ParseLevelCode(arTokens[0], level);

		if(!success)
		{
			this->Send("Couldn't parse level argument: " + arTokens[0] + "\r\n");
			return BAD_ARGUMENTS;
		}

		if(aSetLevel && level > 0)
		{
			if (arTokens[0].size() != 1){
				this->Send("The \'set level\' command needs a single filter level, use the lowest filter only or use \'set filter\' to specify more than one level.\r\n");
				return BAD_ARGUMENTS;
			}
			// bit trick to set all bits below the highest one.
			level = (level - 1) | level;
		}

		arTokens.erase(arTokens.begin());

		std::vector<Logger*> loggers;

		if(arTokens.size() == 0) //if no device specified, set all of them
		{
			mpLog->GetAllLoggers(loggers);
		}
		else //otherwise loop over the remaining arguments
		{
			for(size_t i=0; i<arTokens.size(); i++)
			{
				Logger* pLogger = mpLog->GetExistingLogger(arTokens[i]);

				if(pLogger == NULL) //device name not found
				{
					this->Send("Unrecognized device name: " + arTokens[i] + "\r\n");
					return BAD_ARGUMENTS;
				}
				else
				{
					loggers.push_back(pLogger);
				}
			}
		}
		
		//set the log levels

		for(size_t i=0; i<loggers.size(); i++)
		{
			if(arTokens.size() != 0 || loggers[i]->GetName() != "Terminal")
				loggers[i]->SetFilters(level);
		}

		
		return SUCCESS;
	}

	retcode LogTerminalExtension::HandleClearScreen(std::vector<std::string>& /*arTokens*/)
	{
		this->Send(ITerminal::CLEAR_SRC);
		return SUCCESS;
	}


	void LogTerminalExtension::StopLoggingToFile()
	{		
		delete mpFileLogger;
		mpFileLogger = NULL;		
	}

	retcode LogTerminalExtension::HandleStartToFileLogging(std::vector<std::string>& arTokens)
	{
		if(arTokens.size() != 1) return BAD_ARGUMENTS;
		StopLoggingToFile();

		std::string fileName;
		mpFileLogger = new LogToFile(mpLog, arTokens[0]);
		return SUCCESS;
	}

	retcode LogTerminalExtension::HandleStopToFileLogging(std::vector<std::string>& arTokens)
	{
		if(arTokens.size() != 0) return BAD_ARGUMENTS;
		StopLoggingToFile();
		return SUCCESS;
	}
}
