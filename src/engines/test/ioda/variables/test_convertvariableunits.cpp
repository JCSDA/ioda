/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Variables/Has_Variables.h"

#include "ioda/Engines/Factory.h"
#include "ioda/Layout.h"
#include "ioda/ObsGroup.h"

#include "eckit/testing/Test.h"

#include "oops/util/FloatCompare.h"

using namespace eckit::testing;
using namespace ioda;

namespace ioda {
namespace test {

const int locations = 40;
const int channels = 30;
CASE("Convert variables") {
  typedef std::string str;
  str mappingFile = str(TEST_SOURCE_DIR) + "/hasvariables_unitconversion_map.yaml";
  Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;
  Engines::BackendCreationParameters backendParams;
  backendParams.fileName = "ioda-engines_hasvariables_unitconv-file.hdf5";
  backendParams.action = Engines::BackendFileActions::Create;
  backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
  Group backend = ioda::Engines::constructBackend(backendName, backendParams);

  ioda::Variable temp = backend.vars.create<double>("temp", {3});
  temp.write<double>({0.0, 50.0, 100.0});
  ioda::Variable windspeed = backend.vars.create<double>("windspeed", {3});
  windspeed.write<double>({0.0, 50.0, 100.0});
  ioda::Variable relativeHumidity = backend.vars.create<double>("rh", {3});
  relativeHumidity.write<double>({0.0, 50.0, 100.0});
  ioda::Variable pressure = backend.vars.create<double>("press", {3});
  pressure.write<double>({0.0, 50.0, 100.0});
  ioda::Variable angle = backend.vars.create<double>("angle", {3});
  angle.write<double>({0.0, 50.0, 100.0});
  ioda::Variable cloudCoverage = backend.vars.create<double>("cloudCov", {3});
  cloudCoverage.write<double>({0.0, 50.0, 100.0});
  ioda::Variable undefinedUnit = backend.vars.create<double>("bar", {3});
  undefinedUnit.write<double>({0.0, 50.0, 100.0});

  ObsGroup og = ObsGroup::generate(
          backend,
          {
            std::make_shared<NewDimensionScale<int>>(
            "nlocs", locations, ioda::Unlimited, locations),
            std::make_shared<NewDimensionScale<int>>(
            "nchans", channels, channels, channels), },
          detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroupODB,
                                             mappingFile));

  std::vector<double> expectedValue = {0.0, 50.0, 100.0};
  std::vector<double> retrievedValue;
  og.vars.open("temp").read<double>(retrievedValue);
  EXPECT(retrievedValue == expectedValue);
  og.vars.convertVariableUnits();
  temp = og.vars.open("temp");
  EXPECT(temp.atts.exists("units"));
  std::string tempUnit;
  temp.atts.open("units").read<std::string>(tempUnit);
  EXPECT(tempUnit == "kelvin");
  og.vars.open("temp").read<double>(retrievedValue);
  expectedValue = {273.15, 323.15, 373.15};
  EXPECT(oops::are_all_close_relative(retrievedValue, expectedValue, .05));
  og.vars.open("windspeed").read<double>(retrievedValue);
  expectedValue = {0.0, 25.7222, 51.4444};
  EXPECT(oops::are_all_close_relative(retrievedValue, expectedValue, .05));
  og.vars.open("rh").read(retrievedValue);
  expectedValue = {0.0, 0.5, 1.0};
  EXPECT(oops::are_all_close_relative(retrievedValue, expectedValue, .05));
  og.vars.open("press").read<double>(retrievedValue);
  expectedValue = {0.0, 5000.0, 10000.0};
  EXPECT(oops::are_all_close_relative(retrievedValue, expectedValue, .05));
  og.vars.open("angle").read<double>(retrievedValue);
  expectedValue = {0.0, 0.872665, 1.74533};
  EXPECT(oops::are_all_close_relative(retrievedValue, expectedValue, .05));
  og.vars.open("cloudCov").read<double>(retrievedValue);
  expectedValue = {0.0, 6.25, 12.5};
  EXPECT(oops::are_all_close_relative(retrievedValue, expectedValue, .05));
  Variable bar = og.vars.open("bar");
  bar.read<double>(retrievedValue);
  std::string barUnit;
  bar.atts.open("units").read<std::string>(barUnit);
  EXPECT(barUnit == "baz");
  expectedValue = {0.0, 50.0, 100.0};
  EXPECT(oops::are_all_close_relative(retrievedValue, expectedValue, .05));
}
}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
