/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_ENGINES_REMOVEHDF5VARIABLE_H_
#define TEST_ENGINES_REMOVEHDF5VARIABLE_H_

#include <memory>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace ioda {
namespace test {

class RmH5VarTestParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(RmH5VarTestParameters, Parameters)
 public:
  oops::RequiredParameter<std::string> outFile{"output file", this};
};

CASE("ioda/RemoveHdf5Variable") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  RmH5VarTestParameters params;
  params.validateAndDeserialize(conf);

  // Create an HDF5 file backend and attach to an ObsGroup
  Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;
  Engines::BackendCreationParameters backendParams;
  backendParams.fileName = params.outFile;
  backendParams.action = Engines::BackendFileActions::Create;
  backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;

  ioda::Group g = constructBackend(backendName, backendParams);

  // Need two dimensions, nlocs and nchans, so we can test using 1D and 2D variables
  const int numLocs = 5;
  const int numChans = 3;
  ioda::NewDimensionScales_t newDims;
  newDims.push_back(ioda::NewDimensionScale<int>("nlocs", numLocs, numLocs, numLocs));
  newDims.push_back(ioda::NewDimensionScale<int>("nchans", numChans, numChans, numChans));

  ioda::ObsGroup og = ioda::ObsGroup::generate(g, newDims);

  // Create 2 1D vars and remove one of them and create 2 2D vars and remove one
  // of them. This is enough to check the output file as see if the DIMENSION_LIST
  // and REFERENCE_LIST attributes all got updated correctly with the removals.
  ioda::Variable nlocsVar = og.vars["nlocs"];
  ioda::Variable nchansVar = og.vars["nchans"];

  ioda::VariableCreationParameters float_params;
  float_params.chunk = true;
  float_params.compressWithGZIP();
  float_params.setFillValue<float>(-999);

  og.vars.createWithScales<float>("keep1d", {nlocsVar}, float_params);
  og.vars.createWithScales<float>("toss1d", {nlocsVar}, float_params);

  og.vars.createWithScales<float>("keep2d", {nlocsVar, nchansVar}, float_params);
  og.vars.createWithScales<float>("toss2d", {nlocsVar, nchansVar}, float_params);

  // remove the two variables
  og.vars.remove("toss1d");
  og.vars.remove("toss2d");
}

class RemoveHdf5Variable : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::RemoveHdf5Variable";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_ENGINES_REMOVEHDF5VARIABLE_H_
