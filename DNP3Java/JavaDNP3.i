%module(directors="1") javadnp3
%{
/* Includes the header in the wrapper code */

#include <DNP3/StackManager.h>

using namespace apl;
using namespace apl::dnp;

%}

%pragma(java) jniclasscode=%{
  static {
    	    	
    try{
      System.out.println("trying to load library");
      System.out.println("java.library.path="+System.getProperty("java.library.path"));
      System.loadLibrary("dnp3java");
      System.out.println("Done loading library");
    }catch(UnsatisfiedLinkError e){
      System.out.println(e);
      System.out.println("java.library.path="+System.getProperty("java.library.path"));
      System.exit(-1);
    }
  }
%}

%include <std_string.i>
%include <std_vector.i>

%template(VectorOfString) std::vector<std::string>;

%feature("director"); //generate directors for all classes that have virtual methods

%include "APL/Types.h"
%include "APL/INotifier.h"
%include "APL/LogTypes.h"
%include "APL/LogBase.h"
%include "APL/PhysLayerSettings.h"
%include "APL/SerialTypes.h"
%include "APL/QualityMasks.h"
%include "APL/CommandTypes.h"
%include "APL/DataTypes.h"
%include "APL/DataInterfaces.h"
%include "APL/CommandInterfaces.h"

%include "DNP3/PointClass.h"
%include "DNP3/LinkConfig.h"
%include "DNP3/AppConfig.h"
%include "DNP3/MasterConfigTypes.h"

%template(VectorOfExceptionScan) std::vector<apl::dnp::ExceptionScan>;
%include "DNP3/MasterConfig.h"
%include "DNP3/SlaveConfig.h"

%include "DNP3/DeviceTemplateTypes.h"
%template(VectorOfEventPointRecord) std::vector<apl::dnp::EventPointRecord>;
%template(VectorOfDeadbandPointRecord) std::vector<apl::dnp::DeadbandPointRecord>;
%template(VectorOfControlRecord) std::vector<apl::dnp::ControlRecord>;
%include "DNP3/DeviceTemplate.h"

%include "DNP3/MasterStackConfig.h"
%include "DNP3/SlaveStackConfig.h"


%include <exception.i>
// provide an exception handler for the stack manager
%exception {
	try {
		$action
	}
	catch (apl::Exception ex) {
		SWIG_exception(SWIG_ValueError, ex.what());
	}
	catch (std::exception ex) {
		SWIG_exception(SWIG_ValueError, ex.what());
	}
}


%include "DNP3/StackManager.h"

