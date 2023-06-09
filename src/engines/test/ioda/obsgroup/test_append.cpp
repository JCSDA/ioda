/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <math.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <typeinfo>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "Eigen/Dense"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/Layout.h"
#include "ioda/ObsGroup.h"

#include "ioda/testconfig.h"

#include "oops/util/Logger.h"
#include "oops/runs/Run.h"
#include "oops/runs/Test.h"

void test_obsgroup_helper_funcs(std::string backendType, std::string fileName,
                                const std::string mappingFile = "") {
  using namespace ioda;

  // Create test data
  const int locations = 40;
  const int channels = 30;
  const int locationsX2 = 2 * locations;

  // Build data that holds 2 chunks (each chunk is locations in size)
  // to see if can write the first chunk, resize the variable and write
  // the second chunk.

  // Set nlocs (size: 2*locations) and Channel (size: channels) coordinate values
  // nlocs set to 0..nlocs-1, and Channel set to 1..nchans
  std::vector<int> nLocs(locationsX2);
  std::iota(nLocs.begin(), nLocs.end(), 0);

  std::vector<int> Channel(channels);
  std::iota(Channel.begin(), Channel.end(), 1);

  Eigen::ArrayXXf myDataExpected(locationsX2, channels);
  std::vector<float> myLonExpected(locationsX2);
  std::vector<float> myLatExpected(locationsX2);
  auto mid_loc = static_cast<float>(locations);
  auto mid_chan = static_cast<float>(channels) / 2.0f;
  for (std::size_t i = 0; i < locationsX2; ++i) {
    myLonExpected[i] = static_cast<float>(i % 8) * 3.0f;
    myLatExpected[i] = static_cast<float>(i / 8) * 3.0f;  // NOLINT(bugprone-integer-division)
    for (std::size_t j = 0; j < channels; ++j) {
      float del_i = static_cast<float>(i) - mid_loc;
      float del_j = static_cast<float>(j) - mid_chan;
      myDataExpected(i, j) = sqrt(del_i * del_i + del_j * del_j);
    }
  }

  // Split the data into two chunks
  Eigen::ArrayXXf myDataExpected1(locations, channels);
  Eigen::ArrayXXf myDataExpected2(locations, channels);
  myDataExpected1 = myDataExpected.block<locations, channels>(0, 0);
  myDataExpected2 = myDataExpected.block<locations, channels>(locations, 0);

  std::vector<float> myLatExpected1(myLatExpected.begin(), myLatExpected.begin() + locations);
  std::vector<float> myLatExpected2(myLatExpected.begin() + locations, myLatExpected.end());
  std::vector<float> myLonExpected1(myLonExpected.begin(), myLonExpected.begin() + locations);
  std::vector<float> myLonExpected2(myLonExpected.begin() + locations, myLonExpected.end());
  std::vector<int> nLocs1(nLocs.begin(), nLocs.begin() + locations);
  std::vector<int> nLocs2(nLocs.begin() + locations, nLocs.end());

  // Create a backend
  Engines::BackendNames backendName;
  Engines::BackendCreationParameters backendParams;
  if (backendType == "file" || backendType == "fileRemapped") {
    backendName = Engines::BackendNames::Hdf5File;

    backendParams.fileName = fileName;
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
  } else if (backendType == "memory") {
    backendName = Engines::BackendNames::ObsStore;
  } else {
    throw Exception("Unrecognized backend type", ioda_Here())
            .add("backendType", backendType);
  }
  Group backend = constructBackend(backendName, backendParams);

  // Create an ObsGroup object and attach the backend
  ObsGroup og;
  if (backendType != "fileRemapped") {
    og = ObsGroup::generate(
          backend, {
            NewDimensionScale<int>("Location", locations, ioda::Unlimited, locations),
            NewDimensionScale<int>("Channel", channels, channels, channels)
          });
  } else {
    og = ObsGroup::generate(
          backend,
          {
            NewDimensionScale<int>(
            "Location", locations, ioda::Unlimited, locations),
            NewDimensionScale<int>(
            "Channel", channels, channels, channels) },
          detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroupODB,
                                             mappingFile, {"Location", "Channel"}));
  }
  Variable Location_var = og.vars.open("Location");
  Location_var.write(nLocs1);

  Variable Channel_var = og.vars["Channel"];
  Channel_var.write(Channel);

  // Set up creation parameters for variables
  ioda::VariableCreationParameters float_params;
  float_params.chunk = true;
  float_params.compressWithGZIP();
  float_params.setFillValue<float>(-999);

  Variable obs_var;
  Variable lat_var;
  Variable lon_var;
  if (backendType == "fileRemapped") {
    obs_var = og.vars.createWithScales<float>("ObsValue_renamed/myObs_renamed", {Location_var, Channel_var}, float_params);

    og.vars.createWithScales<float>("MetaData_renamed/latitude_renamed", {Location_var}, float_params);
    lat_var = og.vars.open("MetaData/latitude");

    og.vars.createWithScales<float>("MetaData_renamed/longitude_renamed", {Location_var}, float_params);
    lon_var = og.vars["MetaData/longitude"];

    // Now testing that creating a variable not specified in mapping throws an exception
    bool unspecifiedVariableThrows = false;
    try {
      og.vars.createWithScales<float>("Foo/bar", {Location_var}, float_params);
    } catch (const Exception&) {
      unspecifiedVariableThrows = true;
    }
    if (!unspecifiedVariableThrows) {
      throw Exception("Foo/bar did not throw an exception");
    }
  } else {
    obs_var = og.vars.createWithScales<float>("ObsValue/myObs", {Location_var, Channel_var}, float_params);

    og.vars.createWithScales<float>("MetaData/latitude", {Location_var}, float_params);
    lat_var = og.vars.open("MetaData/latitude");

    og.vars.createWithScales<float>("MetaData/longitude", {Location_var}, float_params);
    lon_var = og.vars["MetaData/longitude"];
  }

  // Add attributes to variables
  obs_var.atts.add<std::string>("coordinates", {"longitude latitude Channel"}, {1})
    .add<std::string>("long_name", {"obs I made up"}, {1})
    .add<std::string>("units", {"K"}, {1})
    .add<float>("valid_range", {0.0, 50.0}, {2});
  lat_var.atts.add<std::string>("long_name", {"latitude"}, {1})
    .add<std::string>("units", {"degrees_north"}, {1})
    .add<float>("valid_range", {-90.0, 90.0}, {2});
  lon_var.atts.add<std::string>("long_name", {"longitude"}, {1})
    .add<std::string>("units", {"degrees_east"}, {1})
    .add<float>("valid_range", {-360.0, 360.0}, {2});

  // Write data into group variable structure
  obs_var.writeWithEigenRegular(myDataExpected1);
  lat_var.write(myLatExpected1);
  lon_var.write(myLonExpected1);

  // Append the second data chunk
  // resize the Location variable - do this before writing
  og.resize({std::pair<ioda::Variable, ioda::Dimensions_t>(Location_var, locationsX2)});

  // 1D vector selection objects
  std::vector<ioda::Dimensions_t> memStarts(1, 0);
  std::vector<ioda::Dimensions_t> memCounts(1, locations);
  std::vector<ioda::Dimensions_t> fileStarts(1, locations);
  std::vector<ioda::Dimensions_t> fileCounts(1, locations);

  ioda::Selection memSelect1D;
  ioda::Selection fileSelect1D;
  memSelect1D.extent({locations}).select({ioda::SelectionOperator::SET, memStarts, memCounts});
  fileSelect1D.select({ioda::SelectionOperator::SET, fileStarts, fileCounts});

  // 2D selection objects
  memStarts.push_back(0);  // borrow the arrays from above
  memCounts.push_back(channels);
  fileStarts.push_back(0);
  fileCounts.push_back(channels);

  ioda::Selection memSelect2D;
  ioda::Selection fileSelect2D;
  memSelect2D.extent({locations, channels}).select({ioda::SelectionOperator::SET, memStarts, memCounts});
  fileSelect2D.select({ioda::SelectionOperator::SET, fileStarts, fileCounts});

  // Write the sencond data chunk
  Location_var.write(nLocs2, memSelect1D, fileSelect1D);
  obs_var.writeWithEigenRegular(myDataExpected2, memSelect2D, fileSelect2D);
  lat_var.write(myLatExpected2, memSelect1D, fileSelect1D);
  lon_var.write(myLonExpected2, memSelect1D, fileSelect1D);

  // Read data back and check values
  Eigen::ArrayXXf myData(locationsX2, channels);
  obs_var.readWithEigenRegular(myData);
  if (!myData.isApprox(myDataExpected)) {
    throw Exception("Test obs data mismatch", ioda_Here());
  }

  std::vector<float> myLats(locationsX2, 0.0);
  lat_var.read(myLats);
  for (std::size_t i = 0; i < locationsX2; ++i) {
    // Avoid divide by zero
    double check;
    if (myLatExpected[i] == 0.0) {
      check = fabs(myLats[i]);
    } else {
      check = fabs((myLats[i] / myLatExpected[i]) - 1.0);
    }
    if (check > 1.0e-3) {
      throw Exception("Test lats mismatch outside tolerence (1e-3)", ioda_Here())
              .add("  i", i)
              .add("  myLatExpected[i]", myLatExpected[i])
              .add("  myLats[i]", myLats[i]);
    }
  }

  std::vector<float> myLons(locationsX2, 0.0);
  lon_var.read(myLons);
  for (std::size_t i = 0; i < locationsX2; ++i) {
    // Avoid divide by zero
    double check;
    if (myLonExpected[i] == 0.0) {
      check = fabs(myLons[i]);
    } else {
      check = fabs((myLons[i] / myLonExpected[i]) - 1.0);
    }
    if (check > 1.0e-3) {
      throw Exception("Test lons mismatch outside tolerence (1e-3)", ioda_Here())
              .add("  i", i)
              .add("  myLonExpected[i]", myLonExpected[i])
              .add("  myLons[i]", myLons[i]);
    }
  }

  // Some more checks
  Expects(og.open("ObsValue").vars["myObs"].isDimensionScaleAttached(1, og.vars["Channel"]));
}

int runTest(const std::string & backendType, const std::string & defaultMappingFile,
            const std::string & incompleteMappingFile) {
  try {
    if (backendType == "file") {
      oops::Log::info() << "Testing file backend, "
                        << "using the default Data Layout Policy" << std::endl;
      test_obsgroup_helper_funcs(backendType, {"ioda-engines_obsgroup_append-file.hdf5"});
    } else if (backendType == "memory") {
      oops::Log::info() << "Testing memory backend, "
                        << "using the default Data Layout Policy" << std::endl;
      test_obsgroup_helper_funcs(backendType, {""});
    } else if (backendType == "fileRemapped") {
      oops::Log::info() << "Testing file backend, remapped, "
                        << "using the ODB Data Layout Policy with a complete "
                        << "mapping file" << std::endl;
      std::string mappingFile = std::string(IODA_ENGINES_TEST_SOURCE_DIR)
        + "/obsgroup/" + defaultMappingFile;
      test_obsgroup_helper_funcs(backendType, {"append-remapped.hdf5"}, mappingFile);
      oops::Log::info() << "Testing file backend, remapped, "
                        << "using the ODB Data Layout Policy with an incomplete "
                        << "mapping file" << std::endl;
      mappingFile = std::string(IODA_ENGINES_TEST_SOURCE_DIR)
        + "/obsgroup/" + incompleteMappingFile;
      bool failedWhenNotAllVarsRemapped = false;
      try {
        test_obsgroup_helper_funcs(backendType, {"append-remapped.hdf5"}, mappingFile);
      } catch (const std::exception &e) {
        failedWhenNotAllVarsRemapped = true;
      }
      bool odbGroupFailedWithoutMapping = false;
      // Layout_ObsGroup_ODB throws an exception if mapping yaml file not provided
      try {
        test_obsgroup_helper_funcs(backendType, {"ioda-engines_obsgroup_append-remapped-file.hdf5"});
      } catch (const std::exception& e) {
        odbGroupFailedWithoutMapping = true;
      }
      assert(odbGroupFailedWithoutMapping && failedWhenNotAllVarsRemapped);
    } else {
      throw ioda::Exception("Unrecognized backend type:", ioda_Here())
              .add("Backend type", backendType);
    }
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}

// -----------------------------------------------------------------------------

CASE("AppendTests") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  std::vector<eckit::LocalConfiguration> confs;
  std::string defaultMappingFile = conf.getString("default mapping file");
  std::string incompleteMappingFile = conf.getString("incomplete mapping file");
  conf.get("test cases", confs);
  for (size_t jconf = 0; jconf < confs.size(); ++jconf) {
    eckit::LocalConfiguration config = confs[jconf];
    std::string testName = config.getString("name");
    std::string testBackend = config.getString("backend");
    runTest(testBackend, defaultMappingFile, incompleteMappingFile);
  }
}

// -----------------------------------------------------------------------------

class ObsGroupAppend : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::obsgroup-append";}

  void register_tests() const override {}

  void clear() const override {}
};

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
  oops::Run run(argc, argv);
  ObsGroupAppend tests;
  return run.execute(tests);
}
