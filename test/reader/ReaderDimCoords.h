/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/load/loadObs.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Tests for the OSDF reader's handling of slice dimension coordinate variables.
//
// Accepted: int, int64, and float holding whole numbers. Channel coordinate values must also
// be unique, since they are the channel numbers that select the channels. Every other slice
// dimension is numbered by position (1..N), so its coordinate values may repeat.
// Rejected: any other type (e.g. double, short, string), a float holding a fractional value,
// and repeated Channel numbers.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
/// \brief run the reader over the given input file
void loadDimCoordsFile(const std::string & fileName, std::unique_ptr<osdf::IFrame> & destOsdf,
                       osdf::FrameMetadata & osdfMetadata) {
  eckit::LocalConfiguration dataInConfig;
  dataInConfig.set("engine.type", "H5File");
  dataInConfig.set("engine.obsfile", "Data/testinput_tier_1/" + fileName);
  ioda::ObsDataInParameters dataInParams;
  dataInParams.deserialize(dataInConfig);

  eckit::LocalConfiguration ioPoolConfig;
  ioPoolConfig.set("max pool size", 1);
  ioda::IoPool::IoPoolParameters ioPoolParams;
  ioPoolParams.validateAndDeserialize(ioPoolConfig);

  const std::vector<std::string> obsVarNames;
  const util::TimeWindow timeWindow(
      ::test::TestEnvironment::config().getSubConfiguration("time window"));

  reader::loadObs(dataInParams, ioPoolParams, oops::mpi::world(), obsVarNames, timeWindow,
                  "ReaderDimCoords test", destOsdf, osdfMetadata);
}

// -----------------------------------------------------------------------------
void testAcceptedCoords() {
  const std::vector<eckit::LocalConfiguration> testCases =
      ::test::TestEnvironment::config().getSubConfigurations("accepted");
  for (const auto & testCase : testCases) {
    const std::string fileName = testCase.getString("file");
    const std::string dimName = testCase.getString("dimension");
    oops::Log::info() << "Accepted case: " << fileName << std::endl;

    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
    osdf::FrameMetadata osdfMetadata;
    loadDimCoordsFile(fileName, testOsdf, osdfMetadata);

    EXPECT_EQUAL(osdfMetadata.getDimNums(dimName), testCase.getIntVector("numbers"));

    EXPECT(osdfMetadata.hasDimCoords(dimName));
    const osdf::DimCoords & dimCoords = osdfMetadata.getDimCoords(dimName);
    const std::vector<long> expectedValues =  // NOLINT(runtime/int)
        testCase.getLongVector("coordinate values");
    EXPECT_EQUAL(dimCoords.values,
                 std::vector<std::int64_t>(expectedValues.begin(), expectedValues.end()));
    const std::string coordType = testCase.getString("coordinate type");
    const osdf::DimCoordType expectedType =
        (coordType == "int64") ? osdf::DimCoordType::eInt64
        : (coordType == "float") ? osdf::DimCoordType::eFloat : osdf::DimCoordType::eInt;
    EXPECT(dimCoords.type == expectedType);
  }
}

// -----------------------------------------------------------------------------
void testRejectedCoords() {
  const std::vector<eckit::LocalConfiguration> testCases =
      ::test::TestEnvironment::config().getSubConfigurations("rejected");
  for (const auto & testCase : testCases) {
    const std::string fileName = testCase.getString("file");
    oops::Log::info() << "Rejected case: " << fileName << std::endl;

    std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
    osdf::FrameMetadata osdfMetadata;
    bool threw = false;
    std::string errMsg;
    try {
      loadDimCoordsFile(fileName, testOsdf, osdfMetadata);
    } catch (const eckit::Exception & e) {
      threw = true;
      errMsg = e.what();
    }
    EXPECT(threw);
    if (!threw) continue;

    // The message names the file and says what was wrong
    for (const std::string & expected : testCase.getStringVector("message contains")) {
      const bool found = (errMsg.find(expected) != std::string::npos);
      if (!found) {
        oops::Log::info() << "ReaderDimCoords: error message is missing: " << expected
                          << std::endl << "  message: " << errMsg << std::endl;
      }
      EXPECT(found);
    }
    EXPECT(errMsg.find(fileName) != std::string::npos);
  }
}

// -----------------------------------------------------------------------------
class ReaderDimCoords : public oops::Test {
 public:
  ReaderDimCoords() {}
  virtual ~ReaderDimCoords() {}

 private:
  std::string testid() const override {return "test::ReaderDimCoords";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderDimCoords/testAcceptedCoords")
      { testAcceptedCoords(); });
    ts.emplace_back(CASE("ioda/ReaderDimCoords/testRejectedCoords")
      { testRejectedCoords(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
