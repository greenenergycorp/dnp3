#define BOOST_TEST_MODULE terminal
#include <boost/test/unit_test.hpp>

#include <APL/Log.h>
#include <APL/Thread.h>
#include <APL/TimingTools.h>

#include <APLTestTools/TestHelpers.h>
#include <APLTestTools/MockTimerSource.h>
#include <APLTestTools/MockPhysicalLayerAsyncTS.h>

#include <boost/shared_ptr.hpp>
#include <iostream>

#include <Terminal/Terminal.h>
#include <Terminal/LogTerminalExtension.h>

using namespace boost;
using namespace std;
using namespace apl;



/*
tests that does a pretty stupid run through the Terminal UI commands to make sure
that everything is still functioning. This is not really intended to be a full validation
just a sanity check to make sure all the functionality has been run and an easy fast way 
to develop new functionality.
*/
BOOST_AUTO_TEST_SUITE(UISuite)

bool SendAndTest(MockPhysicalLayerAsyncTS* apLayer, std::string aCmd, std::string aSearchString)
{	
	aCmd.append("\r\n");
	apLayer->WriteToLayer(reinterpret_cast<const byte_t*>(aCmd.c_str()), aCmd.length());
	apLayer->Advance();
	return apLayer->BufferContains(aSearchString);	
}

void TestCommandParsing(MockPhysicalLayerAsyncTS* apLayer){
	//check that a bad command string is parsed and ignored correclty
	BOOST_REQUIRE(SendAndTest(apLayer, "badCommand", "Unrecognized"));

	//check that a valid command with a space before or after the string is ignored
	BOOST_REQUIRE(SendAndTest(apLayer, "help print ", "log"));
	BOOST_REQUIRE(SendAndTest(apLayer, " help print", "log"));

	//test that 2 lines recieved at once are both processed
	BOOST_REQUIRE(SendAndTest(apLayer, "help\r\nhelp print", "log"));

	//generate a big long string that should flood the Readline() buffer.
	byte_t tooBigBuff[1100];
	for(int i=0; i < 1100; ++i) tooBigBuff[i] = '0' + (i%10);
	string tooBigString((char*)tooBigBuff, 1100);
	//add onto the end of the string a valid command we can check the output of
	tooBigString.append("\r\nprint log");						
}

void TestHelpCommands(MockPhysicalLayerAsyncTS* apLayer){
	//test that help generates a big usage text screen.
	BOOST_REQUIRE(SendAndTest(apLayer, "help", "usage"));
	//test that we have atleast some subtopics for print
	BOOST_REQUIRE(SendAndTest(apLayer, "help print", "log"));
	//make sure we get a usefull error message for a missing subtopic
	BOOST_REQUIRE(SendAndTest(apLayer, "help print faketopic", "No topic found"));
	//see that we appear to get the list of registered devices
	BOOST_REQUIRE(SendAndTest(apLayer, "help print loggers", "registered logger"));

	//check that we get a list of sub commands (log should be one)
	BOOST_REQUIRE(SendAndTest(apLayer, "help run", "log"));
	
	//shows that atleast level is a subcommand of set
	BOOST_REQUIRE(SendAndTest(apLayer, "help set", "level"));
	//check that the help string is approriate for set level
	BOOST_REQUIRE(SendAndTest(apLayer, "help set level", "filter levels"));
	//check that the help string is approriate for set col
	BOOST_REQUIRE(SendAndTest(apLayer, "help set logcol", "column order"));

	//check that a bad subcommand generates the usage message.
	BOOST_REQUIRE(SendAndTest(apLayer, "print loggers NotARealDevice", "usage: "));

	//test that a "clear screen" command ilicits a "form feed" that clears the screen.
	BOOST_REQUIRE(SendAndTest(apLayer, "clear screen", "\f"));
}

void TestPrintCommands(MockPhysicalLayerAsyncTS* apLayer, Logger* apLogger){
	//check that our new device is in the list
	BOOST_REQUIRE(SendAndTest(apLayer, "print loggers", "TestDevice"));

	//Log a string and see that it appears in the print log command
	apLogger->Log(LEV_ERROR,LOCATION,"TestLogOutput");
	BOOST_REQUIRE(SendAndTest(apLayer, "print log", "TestLogOutput"));

	//log another string and make sure that it is displayed if we ask for jsut the TestDevice
	apLogger->Log(LEV_ERROR,LOCATION,"TestOutputOnly");
	BOOST_REQUIRE(SendAndTest(apLayer, "print log TestDevice", "TestOutputOnly"));
}
		
void TestSetCommands(MockPhysicalLayerAsyncTS* apLayer){
	//test that the help string is correct
	BOOST_REQUIRE(SendAndTest(apLayer, "set logcol", "usage: "));
	//test that we stop the user from having the same column more that once
	BOOST_REQUIRE(SendAndTest(apLayer, "set logcol tt", "more than once"));
	//check that bad column names are thrown out.
	BOOST_REQUIRE(SendAndTest(apLayer, "set logcol x", "Unrecognized log column"));
	//make sure a valid string is allowed
	BOOST_REQUIRE(SendAndTest(apLayer, "set logcol fldm", ">"));

	//test the help string
	BOOST_REQUIRE(SendAndTest(apLayer, "set level", "usage: "));

	//test that we cant have both all and none
	BOOST_REQUIRE(SendAndTest(apLayer, "set level an", "Couldn't parse"));
	//test that we cant have both all and anything else
	BOOST_REQUIRE(SendAndTest(apLayer, "set level ad", "Couldn't parse"));
	//check that a bad level is ignored
	BOOST_REQUIRE(SendAndTest(apLayer, "set level x", "Couldn't parse"));
	//test that all works
	BOOST_REQUIRE(SendAndTest(apLayer, "set level a", ">"));
	//test that none works
	BOOST_REQUIRE(SendAndTest(apLayer, "set level n", ">"));
	//test that setting all of the levels manually works
	BOOST_REQUIRE(SendAndTest(apLayer, "set level dciwev", ">"));

	//leave it set to all to error only
	BOOST_REQUIRE(SendAndTest(apLayer, "set level e", ">"));

	//make sure it ignores attempts to set levels on bad devieces
	BOOST_REQUIRE(SendAndTest(apLayer, "set level dciwev BadDevice", "Unrecognized device"));
	//check that it works for a good device
	BOOST_REQUIRE(SendAndTest(apLayer, "set level dciwev TestDevice", ">"));
}

void TestRunCommands(MockPhysicalLayerAsyncTS* apLayer, Logger* apLogger){
	//put an entry into the log that we will pickup on RunLog so we see something
	apLogger->Log(LEV_ERROR,LOCATION,"TestOutputOnRun");

	//do the run command, notice I adedd a \r\n which forces a second line of input to be ready 
	//immediateley to break the "run" cycle after a single iteration (this is necessary since the
	//parser is expecting ">" which we don't get during a "run" command.
	BOOST_REQUIRE(SendAndTest(apLayer, "run log\r\n", "TestOutputOnRun"));			
}
		
BOOST_AUTO_TEST_CASE(TerminalInteractions)
{
	//prepare the terminal on a "non-standard" port
	EventLog log;
	Logger* pLoggerA = log.GetLogger(LEV_INTERPRET, "Terminal");
	
	MockTimerSource mts;
	MockPhysicalLayerAsyncTS phys(log.GetLogger(LEV_INTERPRET, "Phys"), &mts);
		
	LogTerminalExtension lte(&log);
	Terminal trm(pLoggerA, &phys, &mts, "Test Terminal", false, false);
	trm.AddExtension(&lte);
	trm.Init();

	BOOST_REQUIRE(mts.DispatchOne());

	//add a "device" to the logger so we can test the devices behavior
	Logger* logger = log.GetLogger(LEV_ERROR,"TestDevice");

	TestCommandParsing(&phys);
	TestHelpCommands(&phys);
	TestPrintCommands(&phys, logger);			
	
	TestSetCommands(&phys);
	TestRunCommands(&phys, logger);

	trm.Shutdown();
}
		
BOOST_AUTO_TEST_SUITE_END()

