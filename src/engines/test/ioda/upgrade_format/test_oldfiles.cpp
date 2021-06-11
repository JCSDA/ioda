/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <iostream>
#include <string>
#include <vector>

#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"
#include "ioda/defs.h"

int main(int, char**) {
  try {
    using namespace std;
    string srcfile(
      "C:/Users/ryan/Downloads/testinput_tier_1.tar/testinput_tier_1/atms_npp_obs_2018041500_m.nc4");

    using namespace ioda;
    Group f = Engines::HH::openFile(srcfile, Engines::BackendOpenModes::Read_Only);
    Variable datetime = f.vars["datetime@MetaData"];
    Variable variable_names = f.vars["variable_names@VarMetaData"];

    if (!datetime.isA<string>()) throw ioda::Exception("Unexpected type.", ioda_Here());

    // vector<char> vd = datetime.readAsVector<char>();

    // vector<string> vDatetimes = datetime.readAsVector<
    //	std::string,
    //	ioda::detail::Object_Accessor_Fixed_Array<std::string, char*>
    //>();
    vector<string> vDatetimes = datetime.readAsVector<std::string>();
    cout << "vDatetimes has " << vDatetimes.size() << " elements.\n";

    // Attribute satellite = f.atts["satellite"];

    return 0;
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception." << std::endl;
    return 2;
  }
}
