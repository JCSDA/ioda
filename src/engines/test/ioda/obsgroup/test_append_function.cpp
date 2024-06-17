/*
 * (C) Copyright 2024 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <array>     // Arrays are fixed-length vectors.
#include <iomanip>   // std::setw
#include <iostream>  // We want I/O.
#include <numeric>   // std::iota
#include <string>    // We want strings
#include <valarray>  // Like a vector, but can also do basic element-wise math.
#include <vector>    // We want vectors

#include "ioda/Engines/EngineUtils.h"  // Used to kickstart the Group engine.
#include "ioda/Exception.h"        // Exceptions and debugging
#include "ioda/Group.h"            // Groups have attributes.
#include "ioda/ObsGroup.h"

//-----------------------------------------------------------------------------------
void checkGroup(const ioda::Group & group,
                const std::vector<int64_t> & locVals,
                const std::vector<int> & chanVals,
                const std::vector<float> & chanFreqVals,
                const std::vector<float> & latVals,
                const std::vector<float> & lonVals,
                const std::vector<float> & tbVals,
                const std::vector<float> & tbErrVals) {
  std::vector<int64_t> testLocVals;
  std::vector<int> testChanVals;
  std::vector<float> testChanFreqVals;
  std::vector<float> testLatVals;
  std::vector<float> testLonVals;
  std::vector<float> testTbVals;
  std::vector<float> testTbErrVals;

  group.vars.open("Location").read<int64_t>(testLocVals);
  group.vars.open("Channel").read<int>(testChanVals);
  group.vars.open("MetaData/channelFrequency").read<float>(testChanFreqVals);
  group.vars.open("MetaData/latitude").read<float>(testLatVals);
  group.vars.open("MetaData/longitude").read<float>(testLonVals);
  group.vars.open("ObsValue/brightnessTemperature").read<float>(testTbVals);
  group.vars.open("ObsError/brightnessTemperature").read<float>(testTbErrVals);

  Expects(testLocVals == locVals);
  Expects(testChanVals == chanVals);
  Expects(testChanFreqVals == chanFreqVals);
  Expects(testLatVals == latVals);
  Expects(testLonVals == lonVals);
  Expects(testTbVals == tbVals);
  Expects(testTbErrVals == tbErrVals);
}

//-----------------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    // set up data for the test
    const std::vector<int64_t> origLocs{ 0, 1, 2, 3, 4 };
    const std::vector<int> origChans{ 1, 2, 3 };
    const std::vector<float> origChanFreqs{ 90.0, 100.0, 110.0 };
    const std::vector<float> origLats{ 10.0, 11.0, 12.0, 13.0, 14.0 };
    const std::vector<float> origLons{ -10.0, -11.0, -12.0, -13.0, -14.0 };
    const std::vector<float> origTb{
      280.0, 281.0, 282.0,
      283.0, 284.0, 285.0,
      286.0, 287.0, 288.0,
      289.0, 290.0, 291.0,
      292.0, 293.0, 294.0
    };
    const std::vector<float> origTbErr {
      1.0, 1.1, 1.2,
      1.3, 1.4, 1.5,
      1.6, 1.7, 1.8,
      1.9, 2.0, 2.1,
      2.2, 2.3, 2.4
    };

    const std::vector<int64_t> newLocs{ 5, 6, 7 };
    const std::vector<float> newLats{ 15.0, 16.0, 17.0 };
    const std::vector<float> newLons{ -15.0, -16.0, -17.0 };
    const std::vector<float> newTb{
      295.0, 296.0, 297.0,
      298.0, 299.0, 300.0,
      301.0, 302.0, 303.0
    };
    const std::vector<float> newTbErr {
      2.5, 2.6, 2.7,
      2.8, 2.9, 3.0,
      3.1, 3.2, 3.3
    };

    const std::vector<int64_t> totalLocs{ 0, 1, 2, 3, 4, 5, 6, 7 };
    const std::vector<float> totalLats{ 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0 };
    const std::vector<float> totalLons{ -10.0, -11.0, -12.0, -13.0, -14.0, -15.0, -16.0, -17.0 };
    const std::vector<float> totalTb{
      280.0, 281.0, 282.0,
      283.0, 284.0, 285.0,
      286.0, 287.0, 288.0,
      289.0, 290.0, 291.0,
      292.0, 293.0, 294.0,
      295.0, 296.0, 297.0,
      298.0, 299.0, 300.0,
      301.0, 302.0, 303.0
    };
    const std::vector<float> totalTbErr {
      1.0, 1.1, 1.2,
      1.3, 1.4, 1.5,
      1.6, 1.7, 1.8,
      1.9, 2.0, 2.1,
      2.2, 2.3, 2.4,
      2.5, 2.6, 2.7,
      2.8, 2.9, 3.0,
      3.1, 3.2, 3.3
    };

    // build an ObsGroup with 1D and 2D variables
    ioda::Engines::BackendNames backendName = ioda::Engines::BackendNames::ObsStore;
    ioda::Engines::BackendCreationParameters backendParams;
    ioda::Group backend = constructBackend(backendName, backendParams);

    const int numLocs = origLocs.size();
    const int numChans = origChans.size();
    ioda::ObsGroup origGroup = ioda::ObsGroup::generate(backend, {
      ioda::NewDimensionScale<int64_t>("Location", numLocs, ioda::Unlimited, numLocs),
      ioda::NewDimensionScale<int>("Channel", numChans, numChans, numChans)
      });

    ioda::Variable locVar = origGroup.vars.open("Location");
    ioda::Variable chanVar = origGroup.vars.open("Channel");
    locVar.write<int64_t>(origLocs);
    chanVar.write<int>(origChans);

    ioda::VariableCreationParameters floatParams = ioda::VariableCreationParameters::defaults<float>();
    floatParams.noCompress();

    ioda::Variable chanFreqVar =
      origGroup.vars.createWithScales<float>("MetaData/channelFrequency",
                                             { chanVar }, floatParams);
    chanFreqVar.atts.add<std::string>("units", std::string("GHz"));
    chanFreqVar.write<float>(origChanFreqs);

    ioda::Variable latVar =
      origGroup.vars.createWithScales<float>("MetaData/latitude", { locVar }, floatParams);
    latVar.atts.add<std::string>("units", std::string("degrees"));
    latVar.write<float>(origLats);

    ioda::Variable lonVar =
      origGroup.vars.createWithScales<float>("MetaData/longitude", { locVar }, floatParams);
    lonVar.atts.add<std::string>("units", std::string("degrees"));
    lonVar.write<float>(origLons);

    ioda::Variable tbVar =
      origGroup.vars.createWithScales<float>("ObsValue/brightnessTemperature",
                                      { locVar, chanVar }, floatParams);
    tbVar.atts.add<std::string>("units", std::string("K"));
    tbVar.write<float>(origTb);

    ioda::Variable tbErrVar =
      origGroup.vars.createWithScales<float>("ObsError/brightnessTemperature",
                                      { locVar, chanVar }, floatParams);
    tbErrVar.atts.add<std::string>("units", std::string("K"));
    tbErrVar.write<float>(origTbErr);

    // check the contents of the ObsGroup
    checkGroup(origGroup, origLocs, origChans, origChanFreqs, origLats,
               origLons, origTb, origTbErr);

    // build Group for appending (another ObsStore backend instance)
    ioda::Group appendBackend = constructBackend(backendName, backendParams);

    const int newNumLocs = newLocs.size();
    const int newNumChans = origChans.size();
    ioda::ObsGroup appendGroup = ioda::ObsGroup::generate(appendBackend, {
      ioda::NewDimensionScale<int64_t>("Location", newNumLocs, ioda::Unlimited, newNumLocs),
      ioda::NewDimensionScale<int>("Channel", newNumChans, newNumChans, newNumChans)
      });

    ioda::Variable newLocVar = appendGroup.vars.open("Location");
    ioda::Variable newChanVar = appendGroup.vars.open("Channel");
    newLocVar.write<int64_t>(newLocs);
    newChanVar.write<int>(origChans);

    ioda::Variable newChanFreqVar =
      appendGroup.vars.createWithScales<float>("MetaData/channelFrequency",
                                               { newChanVar }, floatParams);
    newChanFreqVar.atts.add<std::string>("units", std::string("GHz"));
    newChanFreqVar.write<float>(origChanFreqs);

    ioda::Variable newLatVar =
      appendGroup.vars.createWithScales<float>("MetaData/latitude", { newLocVar }, floatParams);
    newLatVar.atts.add<std::string>("units", std::string("degrees"));
    newLatVar.write<float>(newLats);

    ioda::Variable newLonVar =
      appendGroup.vars.createWithScales<float>("MetaData/longitude", { newLocVar }, floatParams);
    newLonVar.atts.add<std::string>("units", std::string("degrees"));
    newLonVar.write<float>(newLons);

    ioda::Variable newTbVar =
      appendGroup.vars.createWithScales<float>("ObsValue/brightnessTemperature",
                                      { newLocVar, newChanVar }, floatParams);
    newTbVar.atts.add<std::string>("units", std::string("K"));
    newTbVar.write<float>(newTb);

    ioda::Variable newTbErrVar =
      appendGroup.vars.createWithScales<float>("ObsError/brightnessTemperature",
                                      { newLocVar, newChanVar }, floatParams);
    newTbErrVar.atts.add<std::string>("units", std::string("K"));
    newTbErrVar.write<float>(newTbErr);

    // check the contents of the append group
    checkGroup(appendGroup, newLocs, origChans, origChanFreqs, newLats,
               newLons, newTb, newTbErr);

    // call the append command and check the results
    origGroup.append(appendGroup);
    checkGroup(origGroup, totalLocs, origChans, origChanFreqs, totalLats,
               totalLons, totalTb, totalTbErr);

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
