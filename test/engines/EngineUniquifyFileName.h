/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_ENGINES_ENGINEUNIQUIFYFILENAME_H_
#define TEST_ENGINES_ENGINEUNIQUIFYFILENAME_H_

#include <memory>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/Engines/EngineUtils.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

CASE("ioda/CheckRankTimeRankCombos") {
  // This test is checking to make sure the uniquifyFileName utility in EngineUtils
  // is working properly.
  std::vector<eckit::LocalConfiguration> configs;
  ::test::TestEnvironment::config().get("uniquify file name tests", configs);

  for (std::size_t jj = 0; jj < configs.size(); ++jj) {
    // Each case contains values for the arguments to the uniquifyFileName function.
    // uniquifyFileName is called and the output it returns is checked against
    // an expected value.
    eckit::LocalConfiguration testCaseConfig = configs[jj].getSubConfiguration("case");
    oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

    std::size_t rank = testCaseConfig.getUnsigned("rank");
    int timeRank = testCaseConfig.getInt("time rank");
    bool createMultipleFiles = testCaseConfig.getBool("create multiple files");
    std::string fileName = testCaseConfig.getString("file name");
    std::string expectedFileName = testCaseConfig.getString("expected file name");

    std::string testFileName =
        Engines::uniquifyFileName(fileName, createMultipleFiles, rank, timeRank);

    oops::Log::debug() << "  test file name: " << testFileName << std::endl;
    oops::Log::debug() << "  expected file name: " << expectedFileName << std::endl;
    EXPECT(testFileName == expectedFileName);
  }
}

class EngineUniquifyFileName : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::EngineUniquifyFileName";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_ENGINES_ENGINEUNIQUIFYFILENAME_H_
