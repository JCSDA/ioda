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
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/reader/load/loadObsContainerFromNetcdf.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void populateOsdfFromNetcdf(const ioda::ObsDataInParameters & dataInParams,
                            const ioda::ObsIoPool::ObsIoPool & obsIoPool,
                            std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Collectively call the loadOsdfFromNetcdf function with all io pool members.
  if (obsIoPool.inIoPool()) {
    reader::loadOsdfFromNetcdf(dataInParams, obsIoPool.commPool(), testOsdf);
  }

  // Distribute the column metadata (definitions) from an io pool rank to all
  // non-io pool ranks so that all ranks have consistent column definitions.
  reader::distributeOsdfColumnMetadata(obsIoPool.commAll(), obsIoPool.inIoPool(), testOsdf);
}

// -----------------------------------------------------------------------------
void checkOsdf(const eckit::LocalConfiguration & testConfig,
               const ioda::ObsIoPool::ObsIoPool & obsIoPool,
               const std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Check if we are in the correct io pool communicator
  const int myMainRank = obsIoPool.commAll().rank();
  const int myMainSize = obsIoPool.commAll().size();
  const int myPoolRank = obsIoPool.commPool().rank();
  const int myPoolSize = obsIoPool.commPool().size();

  const std::string sizeRankKey = "mpi size" + std::to_string(myMainSize) +
                                  ".rank" + std::to_string(myMainRank);

  // Verify the io pool size and rank
  const int expectedPoolSize = testConfig.getInt(sizeRankKey + ".pool comm size");
  const int expectedPoolRank = testConfig.getInt(sizeRankKey + ".pool comm rank");
  EXPECT_EQUAL(myPoolSize, expectedPoolSize);
  EXPECT_EQUAL(myPoolRank, expectedPoolRank);

  // Verify the shape of the OSDF container
  const std::size_t expectedNumRows = testConfig.getUnsigned(sizeRankKey + ".nlocs");
  const std::size_t expectedNumCols = testConfig.getUnsigned(sizeRankKey + ".nvars");
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
    const eckit::LocalConfiguration ioPoolConfig = config.getSubConfiguration("io pool");
    ioda::IoPool::IoPoolParameters ioPoolParams;
    ioPoolParams.validateAndDeserialize(ioPoolConfig);
    std::unique_ptr<ioda::ObsIoPool::ObsIoPool> obsIoPool =
      std::make_unique<ioda::ObsIoPool::ObsIoPool>(ioPoolParams, oops::mpi::world());

    // Create a row-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();

    // Collectively call the populateOsdfFromNetcdf with all io pool members
    populateOsdfFromNetcdf(dataInParams, *obsIoPool, testOsdf);
    checkOsdf(testConfig, *obsIoPool, testOsdf);
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
    const eckit::LocalConfiguration ioPoolConfig = config.getSubConfiguration("io pool");
    ioda::IoPool::IoPoolParameters ioPoolParams;
    ioPoolParams.validateAndDeserialize(ioPoolConfig);
    std::unique_ptr<ioda::ObsIoPool::ObsIoPool> obsIoPool =
      std::make_unique<ioda::ObsIoPool::ObsIoPool>(ioPoolParams, oops::mpi::world());

    // Create a column-oriented OSDF
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();

    // Collectively call the populateOsdfFromNetcdf with all io pool members
    populateOsdfFromNetcdf(dataInParams, *obsIoPool, testOsdf);
    checkOsdf(testConfig, *obsIoPool, testOsdf);
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
