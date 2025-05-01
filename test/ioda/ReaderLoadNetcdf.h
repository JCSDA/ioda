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
#include "ioda/reader/load/loadObsContainerFromNetcdf.hpp"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void populateOsdfFromNetcdf(const eckit::LocalConfiguration & loadConfig,
                            std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Grab the configuration: file name, start location index, and location count
  const std::string fileName = loadConfig.getString("file name");
  const std::size_t startLoc = loadConfig.getLong("start location");
  const std::size_t locCount = loadConfig.getLong("location count");

  // Populate the data frame from the input file.
  const int loadRc =
    reader::loadObsContainerFromNetcdf(fileName, startLoc, locCount, testOsdf);
  EXPECT_EQUAL(loadRc, 0);
}

// -----------------------------------------------------------------------------
void checkOsdf(const eckit::LocalConfiguration & loadConfig,
               const std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Grab expected values from the configuration
  const std::size_t expectedNumRows = loadConfig.getLong("location count");
  const std::size_t expectedNumCols = loadConfig.getUnsigned("expected number of columns");

  // Number of rows in testOsdf should be equal to locCount
  const std::size_t numRows = testOsdf->numRows();
  EXPECT_EQUAL(numRows, expectedNumRows);

  // Number of columns
  const std::size_t numCols = testOsdf->numCols();
  EXPECT_EQUAL(numCols, expectedNumCols);
}

// -----------------------------------------------------------------------------
void testFrameRows() {
  // Configuration contains a list of subconfigs that each contain a file name,
  // start location index, and a location count
  const std::vector<eckit::LocalConfiguration> loadConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("input files");

  for (auto & config : loadConfigs) {
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
    populateOsdfFromNetcdf(config, testOsdf);
    checkOsdf(config, testOsdf);
  }
}

// -----------------------------------------------------------------------------
void testFrameCols() {
  // Configuration contains a list of subconfigs that each contain a file name,
  // start location index, and a location count
  const std::vector<eckit::LocalConfiguration> loadConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("input files");

  for (auto & config : loadConfigs) {
    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();
    populateOsdfFromNetcdf(config, testOsdf);
    checkOsdf(config, testOsdf);
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
