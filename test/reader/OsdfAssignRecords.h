/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/make_unique.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameRows.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/reader/distribute/osdfDistributeUtils.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

void testOsdfAssignRecords() {
  const auto &topLevelConf = ::test::TestEnvironment::config();

  const std::vector<eckit::LocalConfiguration> confs =
      topLevelConf.getSubConfigurations("test cases");

  for (const eckit::LocalConfiguration & conf : confs) {
    const std::vector<eckit::LocalConfiguration> configColumnData =
      conf.getSubConfigurations("test column data");

    // Create an instance of a row priority data frame and populate it with
    // test data from the config file.
    std::unique_ptr<osdf::IFrame> rankOsdf = std::make_unique<osdf::FrameRows>();
    std::vector<std::string> testColumnNames;
    std::vector<std::string> testColumnTypes;
    populateFrame(configColumnData, rankOsdf, testColumnNames, testColumnTypes);
    std::vector<std::size_t> calculatedRecNums;
    std::vector<std::string> groupingVars = conf.getStringVector("grouping variables");
    std::vector<std::size_t> expectedRecNums = conf.getUnsignedVector("expected records vector");

    ioda::reader::osdfAssignRecordNumbers(*rankOsdf, groupingVars, calculatedRecNums);
    oops::Log::info() << "Calculated record number vector:" << calculatedRecNums << std::endl;
    EXPECT(calculatedRecNums == expectedRecNums);
  }
}


// -----------------------------------------------------------------------------
class OsdfAssignRecords : public oops::Test {
 public:
  OsdfAssignRecords() {}
  virtual ~OsdfAssignRecords() {}

 private:
  std::string testid() const override {return "test::OsdfAssignRecords";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfAssignRecords/testOsdfAssignRecords")
      { testOsdfAssignRecords(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

