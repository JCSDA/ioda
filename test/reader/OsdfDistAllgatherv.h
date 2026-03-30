/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

// #include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/distribution/IdentityDistribution.h"
// #include "ioda/reader/filter/filterObsContainer.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
// #include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

void testDistAllgatherv() {
  int myMpiSize = oops::mpi::world().size();
  int myMpiRank = oops::mpi::world().rank();

  std::string MyPath = "test column data.mpi size" + std::to_string(myMpiSize) +
                          ".rank" + std::to_string(myMpiRank);

  const std::vector<eckit::LocalConfiguration> configColumnData =
      ::test::TestEnvironment::config().getSubConfigurations(MyPath);
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  // Create an instance of a row priority data frame and populate it with
  // test data from the config file.
  std::unique_ptr<osdf::IFrame> rankOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(configColumnData, rankOsdf, testColumnNames, testColumnTypes);

  std::unique_ptr<osdf::IFrame> gatheredOsdf = std::make_unique<osdf::FrameRows>();
  IdentityDistribution dist(oops::mpi::world(), IdentityDistribution::Parameters_());
  dist.setNumberLocations(rankOsdf->numRows());
  auto colNames = rankOsdf->columnNames();
  EXPECT(colNames.size() > 0);
  for (const auto & colName : colNames) {
    osdf::FrameUtils::callWithSupportedType(
      rankOsdf->getColumnType(colName),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> values;
        rankOsdf->getColumn(colName, values);
        dist.allGatherv(values);
        gatheredOsdf->appendNewColumn(colName, values);
        });
  }
  // Read in the expected results after gathering
  const std::vector<eckit::LocalConfiguration> configRefData =
  ::test::TestEnvironment::config().getSubConfigurations("expected gathered data");
  std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefData, refOsdf, refColumnNames, refColumnTypes);

  // Check the results
  compareFrames(gatheredOsdf, testColumnNames, testColumnTypes, refOsdf, refColumnNames,
    refColumnTypes, tolerance, false);
}

// -----------------------------------------------------------------------------
class OsdfDistAllgatherv : public oops::Test {
 public:
  OsdfDistAllgatherv() {}
  virtual ~OsdfDistAllgatherv() {}

 private:
  std::string testid() const override {return "test::OsdfDistAllgatherv";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfDistAllgatherv/testDistAllgatherv")
      { testDistAllgatherv(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
