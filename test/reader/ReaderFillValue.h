/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/load/loadObs.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Verify the OSDF netCDF reader's fill-value handling for all supported data types.
// These include: int, int64, float, byte (char) and string.
//
// getNcVarFillValue() has two ways to obtain a fill value, and each type must work in both:
//
//   - no _FillValue attribute -> getNcVarDefaultFillValue<T>(), the netCDF type default
//                                (NC_FILL_INT, NC_FILL_INT64, NC_FILL_FLOAT, NC_FILL_BYTE,
//                                 NC_FILL_STRING)
//   - an explicit _FillValue  -> read from the attribute
//
// replaceFillValuesWithMissing() then maps whatever it found to util::missingValue<T>().
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Render a value for the log. Specialized for char, whose stream insertion would emit
// unprintable control characters instead of the numeric value we care about.
template <typename VarType>
std::string renderValue(const VarType & value) {
  std::ostringstream ss;
  ss << std::setprecision(9) << value;
  return ss.str();
}

template <>
std::string renderValue<char>(const char & value) {
  return std::to_string(static_cast<int>(static_cast<signed char>(value)));
}

// -----------------------------------------------------------------------------
// Compare a column against its expected values, logging both sides so a failure says which
// element went wrong and what the column actually held.
template <typename VarType>
void checkColumn(const std::unique_ptr<osdf::IFrame> & testOsdf,
                 const std::string & colName,
                 const std::vector<VarType> & expectedValues) {
  EXPECT(testOsdf->hasColumn(colName));
  std::vector<VarType> values;
  testOsdf->getColumn(colName, values);

  std::ostringstream ss;
  ss << "ReaderFillValue: " << colName << ": values: [";
  for (const auto & value : values) {
    ss << " " << renderValue<VarType>(value);
  }
  ss << " ], expected: [";
  for (const auto & value : expectedValues) {
    ss << " " << renderValue<VarType>(value);
  }
  ss << " ]";
  oops::Log::info() << ss.str() << std::endl;

  EXPECT_EQUAL(values.size(), expectedValues.size());
  for (std::size_t i = 0; i < expectedValues.size(); ++i) {
    EXPECT(values[i] == expectedValues[i]);
  }
}

// -----------------------------------------------------------------------------
// Load the fill-value test file through the real reader pipeline.
void loadFillValueTestFrame(std::unique_ptr<osdf::IFrame> & testOsdf,
                            osdf::FrameMetadata & osdfMetadata) {
  eckit::LocalConfiguration obsDataInConfig;
  obsDataInConfig.set("engine.type", "H5File");
  obsDataInConfig.set("engine.obsfile", "Data/testinput_tier_1/osdf_reader_fillvalues.nc4");
  ioda::ObsDataInParameters dataInParams;
  dataInParams.deserialize(obsDataInConfig);

  eckit::LocalConfiguration ioPoolConfig;
  ioPoolConfig.set("max pool size", 1);
  ioda::IoPool::IoPoolParameters ioPoolParams;
  ioPoolParams.validateAndDeserialize(ioPoolConfig);

  // This is a file backend, so the simulated variable names and time window are unused by the
  // load (they only matter for the generator backends), but loadObs still requires them.
  const std::vector<std::string> obsVarNames;
  eckit::LocalConfiguration timeWindowConfig;
  timeWindowConfig.set("begin", "2018-01-01T00:00:00Z");
  timeWindowConfig.set("end", "2018-01-01T06:00:00Z");
  const util::TimeWindow timeWindow(timeWindowConfig);

  const eckit::mpi::Comm & commAll = oops::mpi::world();
  reader::loadObs(dataInParams, ioPoolParams, commAll, obsVarNames, timeWindow,
                  "ReaderFillValue test", testOsdf, osdfMetadata);

  EXPECT_EQUAL(testOsdf->numRows(), static_cast<std::size_t>(7));
}

// -----------------------------------------------------------------------------
void testCharFillValues() {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  loadFillValueTestFrame(testOsdf, osdfMetadata);

  const char missing = util::missingValue<char>();

  checkColumn<char>(testOsdf, "TestReference/flagNoFill",
                    {1, 1, 0, 1, 1, 0, 1});
  checkColumn<char>(testOsdf, "TestReference/flagNoFillWithFillData",
                    {1, 0, missing, 1, 0, missing, 1});
  checkColumn<char>(testOsdf, "TestReference/flagWithFill",
                    {1, 1, 0, 1, 1, 0, 1});
}

// -----------------------------------------------------------------------------
void testIntFillValues() {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  loadFillValueTestFrame(testOsdf, osdfMetadata);

  const int missing = util::missingValue<int>();

  checkColumn<int>(testOsdf, "TestReference/intNoFill",
                   {10, 20, missing, 40, 50, missing, 70});
  checkColumn<int>(testOsdf, "TestReference/intWithFill",
                   {10, missing, 30, 40, missing, 60, 70});
}

// -----------------------------------------------------------------------------
void testInt64FillValues() {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  loadFillValueTestFrame(testOsdf, osdfMetadata);

  const std::int64_t missing = util::missingValue<std::int64_t>();

  checkColumn<std::int64_t>(testOsdf, "TestReference/int64NoFill",
                            {100, 200, missing, 400, 500, missing, 700});
  checkColumn<std::int64_t>(testOsdf, "TestReference/int64WithFill",
                            {100, missing, 300, 400, missing, 600, 700});
}

// -----------------------------------------------------------------------------
void testFloatFillValues() {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  loadFillValueTestFrame(testOsdf, osdfMetadata);

  const float missing = util::missingValue<float>();

  checkColumn<float>(testOsdf, "TestReference/floatNoFill",
                     {1.5f, 2.25f, missing, 4.0f, 5.5f, missing, 7.25f});
  checkColumn<float>(testOsdf, "TestReference/floatWithFill",
                     {1.5f, missing, 3.25f, 4.0f, missing, 6.5f, 7.25f});

  // The reader will convert any NaN or infinity to the JEDI missing value.
  checkColumn<float>(testOsdf, "TestReference/floatNanInf",
                     {1.5f, missing, missing, missing, missing, 2.25f, 3.5f});
}

// -----------------------------------------------------------------------------
void testStringFillValues() {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  loadFillValueTestFrame(testOsdf, osdfMetadata);

  const std::string missing = util::missingValue<std::string>();

  checkColumn<std::string>(testOsdf, "TestReference/stringNoFill",
                           {"aa", missing, "cc", "dd", missing, "ff", "gg"});

  // Check both variable-length (vlen) and fixed-length string variables.
  checkColumn<std::string>(testOsdf, "TestReference/stringWithFillVlen",
                           {"aa", missing, "cc", "dd", missing, "ff", "gg"});
  checkColumn<std::string>(testOsdf, "TestReference/stringWithFillFixed",
                           {"aa", missing, "cc", "dd", missing, "ff", "gg"});
}

// -----------------------------------------------------------------------------
class ReaderFillValue : public oops::Test {
 public:
  ReaderFillValue() {}
  virtual ~ReaderFillValue() {}

 private:
  std::string testid() const override {return "test::ReaderFillValue";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFillValue/testCharFillValues")
      { testCharFillValues(); });
    ts.emplace_back(CASE("ioda/ReaderFillValue/testIntFillValues")
      { testIntFillValues(); });
    ts.emplace_back(CASE("ioda/ReaderFillValue/testInt64FillValues")
      { testInt64FillValues(); });
    ts.emplace_back(CASE("ioda/ReaderFillValue/testFloatFillValues")
      { testFloatFillValues(); });
    ts.emplace_back(CASE("ioda/ReaderFillValue/testStringFillValues")
      { testStringFillValues(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
