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

#include "StackHelpers.h"

#include <DNP3XML/XML_DNP3.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <memory>
#include <sys/stat.h>

using namespace std;
using namespace apl;
using namespace apl::dnp;
using namespace APLXML_Base;
using namespace apl::xml;

namespace po = boost::program_options;

void WaitOnEnterKey()
{
	cout << "Press enter to quit " << endl;
	string s;
	cin >> s;	
}

bool GetFilter(char c, FilterLevel& aLevel)
{
	bool success = true;
	switch(c) {
			case('d'): aLevel = LEV_DEBUG; break;
			case('c'): aLevel = LEV_COMM; break;
			case('p'): aLevel = LEV_INTERPRET; break;
			case('i'): aLevel = LEV_INFO; break;
			case('w'): aLevel = LEV_WARNING; break;
			case('e'): aLevel = LEV_ERROR; break;
			default:
				success = false;
				break;
	}
	return success;
}

bool GetFilter(const std::string& arg, FilterLevel& aLevel)
{
	return (arg.size() == 1) ? GetFilter(arg[0], aLevel) : false;	
}

bool FileExists(const std::string& arFileName)
{
	struct stat stFileInfo;
	int statval = stat(arFileName.c_str(), &stFileInfo);
	return statval == 0;
}

bool LoadXmlFile(const std::string& arFileName, IXMLDataBound* apCfg)
{
	try {
		loadXmlInto(arFileName, apCfg);
		return true;
	}
	catch(Exception ex) {
		cout << "Error loading config file: " << arFileName << endl;
		cout << ex.GetErrorString() << endl;
		return false;
	}
}

bool GenerateConfig(bool aIsMaster, const std::string& arPath)
{	
	try {
		if(aIsMaster) {
			APLXML_MTS::MasterTestSet_t cfg;
			XML_TestSet::Configure(cfg, false);
			WriteXmlToFile(&cfg, arPath);		
		}
		else {
			APLXML_STS::SlaveTestSet_t cfg;
			XML_TestSet::Configure(cfg);
			XML_DNP3::Configure(cfg.DeviceTemplate,10,10,10,2,2);
			WriteXmlToFile(&cfg, arPath);					
		}
		std::cout << "generated " << arPath << std::endl;
		return true;
	}
	catch(const apl::Exception& ex) {
		std::cout << "failure generating " << arPath << ex.GetErrorString() << std::endl;
		return false;
	}	
}

int main(int argc, char* argv[])
{
	// uses the simple argument helper to set the config flags approriately
	std::string xmlFilename;
	std::string logLevel;
				
	po::options_description desc("Allowed options");
	desc.add_options()
		("configfile,F", po::value<std::string>(&xmlFilename), "The xml configuration file to use")
		("help,H", "display program options")		
		("generate,G", "Generate a new default config file and exit")
		("gen_on_no_exist,E", "Generate the specified config file automatically if it doesn't exist")
		("log_level,L", po::value<std::string>(&logLevel), "Override the configuration's log level")
		("slave,S", "Use slave test set");

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	}
	catch ( boost::program_options::error& ex ) {
		cout << ex.what() << endl;
		cout << desc << endl;
		return -1;
	}

	if(vm.count("configfile") == 0) {
		xmlFilename = vm.count("slave") ? "dnp3_slave_default.xml" : "dnp3_master_default.xml";
	}

	if(vm.count("help")) {
		cout << desc << endl;		
		return 0;
	}

	if(vm.count("generate")) {				
		return GenerateConfig(vm.count("slave") == 0, xmlFilename) ? 0 : -1;		
	}

	if(!FileExists(xmlFilename) && vm.count("gen_on_no_exist")) {
		if(!GenerateConfig(vm.count("slave") == 0, xmlFilename)) return -1;
	}

	try {
		if ( vm.count("slave") ) {
			auto_ptr<APLXML_STS::SlaveTestSet_t> pCfg(new APLXML_STS::SlaveTestSet_t());
			if(!LoadXmlFile(xmlFilename, pCfg.get())) return -1;
			FilterLevel lev = Convert(pCfg.get()->Log.Filter);
			GetFilter(logLevel, lev);
			SlaveXMLStack slave(pCfg.get(), lev);
			slave.Run();
		}
		else {
			auto_ptr<APLXML_MTS::MasterTestSet_t> pCfg(new APLXML_MTS::MasterTestSet_t());
			if(!LoadXmlFile(xmlFilename, pCfg.get())) return -1;
			FilterLevel lev = Convert(pCfg.get()->Log.Filter);			
			GetFilter(logLevel, lev);
			MasterXMLStack master(pCfg.get(), lev);
			master.Run();
		}

	}
	catch(const Exception& ex) {
		cout << ex.Message() << endl;
		WaitOnEnterKey();
		return -1;
	}
}

