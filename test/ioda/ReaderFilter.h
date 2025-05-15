/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_READERFILTER_H_
#define TEST_IODA_READERFILTER_H_

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
#include "ioda/reader/filter/filterObsContainer.hpp"
#include "ioda/test/ioda/OsdfTestUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

void testFrameRows() {
  // Configuration contains a time window spec and a list of variables (columns).
  // Construct a time window object and use the variable list to create the
  // the row priority data frame by appending the columns.
  const eckit::LocalConfiguration timeWinConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");
  const util::TimeWindow timeWindow(timeWinConfig);

  const std::vector<eckit::LocalConfiguration> configColumnData =
      ::test::TestEnvironment::config().getSubConfigurations("test column data");
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  // Create an instance of a row priority data frame and populate it with
  // test data from the config file.
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(configColumnData, testOsdf, testColumnNames, testColumnTypes);
  oops::Log::info() << "testFrameRows: initial contents" << std::endl;
  testOsdf->print();

  // Read in the expected results after filtering
  const std::vector<eckit::LocalConfiguration> configRefData =
  ::test::TestEnvironment::config().getSubConfigurations("expected filtered data");
  std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefData, refOsdf, refColumnNames, refColumnTypes);

  // Run the filters and check the results
  reader::filterObsContainer(timeWindow, testOsdf);
  oops::Log::info() << "testFrameRows: after filtering" << std::endl;
  compareFrames(testOsdf, testColumnNames, testColumnTypes, refOsdf, refColumnNames, refColumnTypes,
    tolerance);
  testOsdf->print();
}

void testFrameCols() {
  // Configuration contains a time window spec and a list of variables (columns).
  // Construct a time window object and use the variable list to create the
  // column priority data frame by appending the columns.
  const eckit::LocalConfiguration timeWinConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");
  const util::TimeWindow timeWindow(timeWinConfig);

  const std::vector<eckit::LocalConfiguration> configColumnData =
      ::test::TestEnvironment::config().getSubConfigurations("test column data");
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  // Create an instance of a row priority data frame and populate it with
  // data from the config file.
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(configColumnData, testOsdf, testColumnNames, testColumnTypes);
  oops::Log::info() << "testFrameRows: initial contents" << std::endl;
  testOsdf->print();

  // Read in the expected results after filtering
  const std::vector<eckit::LocalConfiguration> configRefData =
  ::test::TestEnvironment::config().getSubConfigurations("expected filtered data");
  std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameCols>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefData, refOsdf, refColumnNames, refColumnTypes);

  // Run the filters and check the results
  reader::filterObsContainer(timeWindow, testOsdf);
  oops::Log::info() << "testFrameCols: after filtering" << std::endl;
  compareFrames(testOsdf, testColumnNames, testColumnTypes, refOsdf, refColumnNames, refColumnTypes,
    tolerance);
  testOsdf->print();
}

// -----------------------------------------------------------------------------
class ReaderFilter : public oops::Test {
 public:
  ReaderFilter() {}
  virtual ~ReaderFilter() {}

 private:
  std::string testid() const override {return "test::ReaderFilter";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameRows")
      { testFrameRows(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameCols")
      { testFrameCols(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_READERFILTER_H_
