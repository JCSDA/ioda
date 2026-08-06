/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "ioda/writer/save/saveObs.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testFrames() {
  const eckit::mpi::Comm & commAll = oops::mpi::world();
  // Configuration contains a list of obs types, each with a list
  // of columns. Each column has a name, type, and values. These are
  // organized by MPI size and rank in order to allow for testing different
  // MPI writing scenarios.
  const std::vector<eckit::LocalConfiguration> testConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("obs types");
  for (const auto & testConfig : testConfigs) {
    for (const auto & frameType : std::vector<std::string>{"FrameCols", "FrameRows"}) {
      const std::string testName = testConfig.getString("name");
      oops::Log::info() << "Testing: " << testName << " (" << frameType << ")" << std::endl;

      // Create the parameters for the obsdataout and io pool configurations.
      //
      // For the sake of the testing, append the frame type and the communicator
      // size at the end of the file name to create a unique base file name before
      // appending the rank numbers. That way the test output files can
      // co-exist.
      eckit::LocalConfiguration obsDataOutConfig =
        testConfig.getSubConfiguration("obsdataout");
      std::string abbrFtype;
      if (frameType == "FrameRows") {
        abbrFtype = std::string("_frows");
      } else {
        abbrFtype = std::string("_fcols");
      }
      const std::string testFileTag = abbrFtype + std::string("_mpi") +
                                      std::to_string(commAll.size());
      std::string outputFileName = obsDataOutConfig.getString("engine.obsfile");
      outputFileName.insert(outputFileName.find_last_of('.'), testFileTag);
      obsDataOutConfig.set("engine.obsfile", outputFileName);
      ioda::ObsDataOutParameters dataOutParams;
      dataOutParams.deserialize(obsDataOutConfig);

      // Create the source OSDF container
      std::unique_ptr<osdf::IFrame> testOsdf = osdf::createIFrame(frameType);

      const eckit::LocalConfiguration testDataConfig =
        testConfig.getSubConfiguration("test data");
      const std::string mpiSizeRankKey = "mpi size" + std::to_string(commAll.size()) +
                                       ".rank" + std::to_string(commAll.rank());
      const std::vector<eckit::LocalConfiguration> columnsConfig =
        testDataConfig.getSubConfiguration(mpiSizeRankKey).getSubConfigurations("columns");
      std::vector<std::string> testColumnNames;
      std::vector<std::string> testColumnTypes;
      populateFrame(columnsConfig, testOsdf, testColumnNames, testColumnTypes);

      // Create the frame metadata for the source OSDF container
      osdf::FrameMetadata testOsdfMetadata;
      populateFrameMetadata(testDataConfig.getSubConfiguration("frame metadata"), testOsdfMetadata);

      // Call the function to save the OSDF container to a netCDF file
      const eckit::LocalConfiguration ioPoolConfig =
        testConfig.getSubConfiguration("io pool");
      ioda::IoPool::IoPoolParameters ioPoolParams;
      ioPoolParams.deserialize(ioPoolConfig);
      std::unique_ptr<ObsIoPool::ObsIoPool> obsIoPool =
        std::make_unique<ObsIoPool::ObsIoPool>(ioPoolParams, commAll);
      ioda::writer::saveObs(dataOutParams, obsIoPool, commAll, "WriterSave test",
                            testOsdf, testOsdfMetadata);
    }
  }
}

// -----------------------------------------------------------------------------
class WriterSave : public oops::Test {
 public:
  WriterSave() {}
  virtual ~WriterSave() {}

 private:
  std::string testid() const override {return "test::WriterSave";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/WriterSave/testFrames")
      { testFrames(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
