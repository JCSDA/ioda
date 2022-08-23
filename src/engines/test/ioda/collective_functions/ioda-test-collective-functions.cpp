/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <iostream>
#include <sstream>
#include <vector>

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/ObsGroup.h"

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "collective.hdf5");

    // Create a new ObsGroup with a few scales.
    NewDimensionScales_t newdims{NewDimensionScale<int32_t>("Location", 1000),
                                 NewDimensionScale<int16_t>("Channel", 64),
                                 NewDimensionScale<uint16_t>("Level", 100)};
    ObsGroup og        = ObsGroup::generate(f, newdims);
    Variable sLocation = og.vars["Location"];
    Variable sChannel  = og.vars["Channel"];
    Variable sLevel    = og.vars["Level"];


    // Create many variables, but use the collective operation instead
    // of the usual createWithScales.
    VariableCreationParameters vcpf = VariableCreationParameters::defaulted<float>();
    vcpf.compressWithGZIP();
    VariableCreationParameters vcpd = VariableCreationParameters::defaulted<double>();
    vcpd.compressWithGZIP();

    NewVariables_t newvars{
      NewVariable<float>("Metadata/Latitude", {sLocation}, vcpf),
      NewVariable<float>("Metadata/Longitude", {sLocation}, vcpf),
      NewVariable<float>("Metadata/Pressure_Level", {sLevel}, vcpf),
      NewVariable<double>("ObsValue/Brightness_Temperature", {sLocation, sChannel}, vcpd),
      NewVariable<float>("Metadata/Altitude", {sLocation, sLevel}, vcpf)
    };
   
    newvars.reserve(1005);
    
    for (size_t i = 0; i < 1000; ++i) {
      std::ostringstream varname;
      varname << "ObsValue/var-" << i;
      newvars.push_back(NewVariable<float>(varname.str(), {sLocation, sChannel}, vcpf));
    }
    

    og.vars.createWithScales(newvars);

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
}
