/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_READERLOADNETCDF_H_
#define TEST_IODA_READERLOADNETCDF_H_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/load/loadObsContainerFromNetcdf.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

constexpr const char _ioPoolCommName[] = "ioPool";
constexpr const char _nonIoPoolCommName[] = "nonIoPool";

// -----------------------------------------------------------------------------
eckit::mpi::Comm & createIoPoolComm(const eckit::mpi::Comm & mainComm,
                                    const eckit::LocalConfiguration & testConfig) {
  const int mySize = mainComm.size();
  const int myRank = mainComm.rank();

  // Grab the configuration for this size and rank
  const std::string sizeRankKey = "mpi size" + std::to_string(mySize) +
                                  ".rank" + std::to_string(myRank);
  const int mySplitColor = testConfig.getInt(sizeRankKey + ".comm split");

  // Split the main communicator into io pool and non-io pool communicators
  if (mySplitColor == 1) {
    return mainComm.split(mySplitColor, _ioPoolCommName);
  } else {
    return mainComm.split(mySplitColor, _nonIoPoolCommName);
  }
}

// -----------------------------------------------------------------------------
void populateOsdfFromNetcdf(const ioda::ObsDataInParameters & dataInParams,
                            const eckit::mpi::Comm & mainComm,
                            const eckit::mpi::Comm & ioPoolComm,
                            std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Collectively call the loadOsdfFromNetcdf function with all io pool members.
  const int inIoPool = (ioPoolComm.name() == _ioPoolCommName) ? 1 : 0;
  if (inIoPool == 1) {
    reader::loadOsdfFromNetcdf(dataInParams, ioPoolComm, testOsdf);
  }

  // Distribute the column metadata (definitions) from an io pool rank to all
  // non-io pool ranks so that all ranks have consistent column definitions.
  reader::distributeOsdfColumnMetadata(mainComm, inIoPool, testOsdf);
}

// -----------------------------------------------------------------------------
void checkOsdf(const eckit::LocalConfiguration & testConfig,
               const eckit::mpi::Comm & mainComm,
               const eckit::mpi::Comm & ioPoolComm,
               const std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Check if we are in the correct io pool communicator
  const int myMainRank = mainComm.rank();
  const int myMainSize = mainComm.size();
  const int myPoolRank = ioPoolComm.rank();
  const int myPoolSize = ioPoolComm.size();

  const std::string sizeRankKey = "mpi size" + std::to_string(myMainSize) +
                                  ".rank" + std::to_string(myMainRank);
  const int mySplitColor = testConfig.getInt(sizeRankKey + ".comm split");
  if (mySplitColor == 1) {
    EXPECT_EQUAL(ioPoolComm.name(), _ioPoolCommName);
  } else {
    EXPECT_EQUAL(ioPoolComm.name(), _nonIoPoolCommName);
  }

  // Grab the expected data config
  const eckit::LocalConfiguration expectedDataConfig =
      testConfig.getSubConfiguration(sizeRankKey + ".expected data");

  // Verify the io pool size and rank
  const int expectedPoolSize = expectedDataConfig.getInt("pool comm size");
  const int expectedPoolRank = expectedDataConfig.getInt("pool comm rank");
  EXPECT_EQUAL(myPoolSize, expectedPoolSize);
  EXPECT_EQUAL(myPoolRank, expectedPoolRank);

  // Verify the shape of the OSDF container
  const std::size_t expectedNumRows = expectedDataConfig.getLong("nlocs");
  const std::size_t expectedNumCols = expectedDataConfig.getUnsigned("nvars");
  const std::size_t numRows = testOsdf->numRows();
  const std::size_t numCols = testOsdf->numCols();
  EXPECT_EQUAL(numRows, expectedNumRows);
  EXPECT_EQUAL(numCols, expectedNumCols);

  // Check for the existence of a few sample columns
  const std::vector<std::string> sampleColNames =
      testConfig.getStringVector("sample column names");
  for (const auto & colName : sampleColNames) {
    EXPECT(testOsdf->hasColumn(colName));
  }
}

// -----------------------------------------------------------------------------
void testFrameRows() {
  // Configuration contains a list of subconfigs that each contain a file name,
  // start location index, and a location count
  const std::vector<eckit::LocalConfiguration> loadConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("obs types");
  const eckit::LocalConfiguration timeWindowConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");

  for (auto & config : loadConfigs) {
    // Create parameters for obsdatain and io pool
    oops::Log::info() << "testFrameRows: config = " << config << std::endl;
    const eckit::LocalConfiguration obsDataInConfig = config.getSubConfiguration("obsdatain");
    const eckit::LocalConfiguration testConfig = config.getSubConfiguration("test data");
    ioda::ObsDataInParameters dataInParams;
    dataInParams.deserialize(obsDataInConfig);

    // Create an IoPool object, and pass the pool communicator to the
    // populateOsdfFromNetcdf function.
    const eckit::mpi::Comm & mainComm = oops::mpi::world();
    eckit::mpi::Comm & ioPoolComm = createIoPoolComm(mainComm, testConfig);

    // Create a row-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();

    // Collectively call the populateOsdfFromNetcdf with all io pool members
    populateOsdfFromNetcdf(dataInParams, mainComm, ioPoolComm, testOsdf);
    checkOsdf(testConfig, mainComm, ioPoolComm, testOsdf);

    // Clean up the io pool communicator
    eckit::mpi::deleteComm(ioPoolComm.name());
  }
}

// -----------------------------------------------------------------------------
void testFrameCols() {
  // Configuration contains a list of subconfigs that each contain a file name,
  // start location index, and a location count
  const std::vector<eckit::LocalConfiguration> loadConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("obs types");
  const eckit::LocalConfiguration timeWindowConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");

  for (auto & config : loadConfigs) {
    // Create parameters for obsdatain and io pool
    oops::Log::info() << "testFrameCols: config = " << config << std::endl;
    const eckit::LocalConfiguration obsDataInConfig = config.getSubConfiguration("obsdatain");
    const eckit::LocalConfiguration testConfig = config.getSubConfiguration("test data");
    ioda::ObsDataInParameters dataInParams;
    dataInParams.deserialize(obsDataInConfig);

    // Create an IoPool object, and pass the pool communicator to the
    // populateOsdfFromNetcdf function.
    const eckit::mpi::Comm & mainComm = oops::mpi::world();
    eckit::mpi::Comm & ioPoolComm = createIoPoolComm(mainComm, testConfig);

    // Create a column-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();

    // Collectively call the populateOsdfFromNetcdf with all io pool members
    populateOsdfFromNetcdf(dataInParams, mainComm, ioPoolComm, testOsdf);
    checkOsdf(testConfig, mainComm, ioPoolComm, testOsdf);

    // Clean up the io pool communicator
    eckit::mpi::deleteComm(ioPoolComm.name());
  }
}

// -----------------------------------------------------------------------------
class ReaderLoadNetcdf : public oops::Test {
 public:
  ReaderLoadNetcdf() {}
  virtual ~ReaderLoadNetcdf() {}

 private:
  std::string testid() const override {return "test::ReaderLoadNetcdf";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderLoadNetcdf/testFrameRows")
      { testFrameRows(); });
    ts.emplace_back(CASE("ioda/ReaderLoadNetcdf/testFrameCols")
      { testFrameCols(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_READERLOADNETCDF_H_
