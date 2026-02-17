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

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/reader/distribute/osdfDistributeUtils.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

void testSelectRankIFrame(std::unique_ptr<osdf::IFrame> & globalOsdf,
                          std::unique_ptr<osdf::IFrame> & refOsdf,
                          std::unique_ptr<osdf::IFrame> & testOsdf)
{
  const std::vector<eckit::LocalConfiguration> configGlobalColumnData =
      ::test::TestEnvironment::config().getSubConfigurations("global column data");
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  // Create the global data frame and populate it with data from the config file.
  // std::unique_ptr<osdf::IFrame> globalOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> globalColumnNames;
  std::vector<std::string> globalColumnTypes;
  populateFrame(configGlobalColumnData, globalOsdf, globalColumnNames, globalColumnTypes);

  // Read in the expected results for this rank after selection
  int myMpiSize = oops::mpi::world().size();
  int myMpiRank = oops::mpi::world().rank();

  std::string MyPath = "mpisize" + std::to_string(myMpiSize) +
                          ".rank" + std::to_string(myMpiRank);
  const std::vector<eckit::LocalConfiguration> configRefData =
  ::test::TestEnvironment::config().getSubConfigurations(MyPath + ".expected local data");
  // std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefData, refOsdf, refColumnNames, refColumnTypes);

  // Run the function and check the results
  // std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::size_t> sourceLocIndices =
                ::test::TestEnvironment::config().getUnsignedVector(MyPath + ".local loc indices");
  auto colNames = globalOsdf->columnNames();
  for (const auto & colName : colNames) {
    ioda::reader::osdfSelectRankData(globalOsdf, testOsdf, colName, sourceLocIndices);
  }
  auto testColumnNames = testOsdf->columnNames();
  oops::Log::debug() << "Rank: " << myMpiRank << ", Test column names:" << std::endl;
  for (const auto & name : testColumnNames) {
    oops::Log::debug() << "  " << name << std::endl;
  }
  compareFrames(testOsdf, testColumnNames, refColumnTypes, refOsdf, refColumnNames,
                refColumnTypes, tolerance);
}

void testSelectRankFrameRows() {
  std::unique_ptr<osdf::IFrame> globalOsdf = std::make_unique<osdf::FrameRows>();
  std::unique_ptr<osdf::IFrame> refOsdf =  std::make_unique<osdf::FrameRows>();
  std::unique_ptr<osdf::IFrame> testOsdf =  std::make_unique<osdf::FrameRows>();

  testSelectRankIFrame(globalOsdf, refOsdf, testOsdf);
}

void testSelectRankFrameCols() {
  std::unique_ptr<osdf::IFrame> globalOsdf = std::make_unique<osdf::FrameCols>();
  std::unique_ptr<osdf::IFrame> refOsdf =  std::make_unique<osdf::FrameCols>();
  std::unique_ptr<osdf::IFrame> testOsdf =  std::make_unique<osdf::FrameCols>();

  testSelectRankIFrame(globalOsdf, refOsdf, testOsdf);
}



// -----------------------------------------------------------------------------
class DistributeSelectRank : public oops::Test {
 public:
  DistributeSelectRank() {}
  virtual ~DistributeSelectRank() {}

 private:
  std::string testid() const override {return "test::DistributeSelectRank";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/DistributeSelectRank/testSelectRankFrameRows")
      { testSelectRankFrameRows(); });
    ts.emplace_back(CASE("ioda/DistributeSelectRank/testSelectRankFrameCols")
      { testSelectRankFrameCols(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
