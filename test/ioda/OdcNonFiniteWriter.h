/*
 * (C) Crown Copyright 2026 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_ODCNONFINITEWRITER_H_
#define TEST_IODA_ODCNONFINITEWRITER_H_

#include <limits>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/testing/Test.h"
#include "odc/api/Odb.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/Engines/ODC.h"
#include "ioda/Layout.h"
#include "ioda/OsdfFrameFacade.hpp"

namespace ioda {
namespace test {

// This test verifies that the OSDF-ODB writer converts NaN and infinities
// to the ODB missing value even when no fill value is registered.
// It does this by passing an already populated OSDF into a facade
// (as the OSDF-ODB writer does)
void testWriteNonFiniteValuesWithNoFill() {
  Engines::ODC::ODC_Parameters params{};
  params.mappingFile = ::test::TestEnvironment::config().getString("mapping file");
  params.queryFile = ::test::TestEnvironment::config().getString("query file");
  params.outputFile = "testoutput/nonfinite-no-fill.odc";
  params.odbType     = "diag";

  const auto layout = detail::DataLayoutPolicy::generate(
    detail::DataLayoutPolicy::Policies::ObsGroupODB, params.mappingFile, {"Location", "Channel"});

  // Adding values through the facade would normalize NaN/Inf before the writer sees
  // them. Populate the frame directly to isolate the writer's missing-value handling.
  osdf::FrameCols frame;
  osdf::FrameMetadata metadata;
  frame.appendNewColumn("MetaData/latitude", std::vector<float>({50.f, 51.f, 52.f, 53.f}));
  frame.appendNewColumn("ObsValue/airTemperature",
                       std::vector<float>({270.f, std::numeric_limits<float>::quiet_NaN(),
                                           std::numeric_limits<float>::infinity(),
                                           -std::numeric_limits<float>::infinity()}));
  OsdfFrameFacade facade(frame, metadata, true);
  facade.initialize(4, std::nullopt, layout, Engines::ContainerOptions());

  // Wrapping existing columns leaves their fill values unset, reproducing the
  // empty-optional case where the fill value is dereferenced (does not occur in normal use)
  // initial_obsvalue/2 maps to ObsValue/airTemperature in the name map.
  EXPECT(facade.missingValue<float>("initial_obsvalue/2") == std::nullopt);
  Engines::ODC::createFile(params, facade);

  // Read with ODC rather than IODA: an IODA read would normalize NaN/Inf and could
  // conceal faulty output. The file itself must contain ODB missing, not NaN/Inf.
  odc::api::Reader reader(params.outputFile, false /* aggregated? */);
  const odc::api::Frame writtenFrame = reader.next();
  EXPECT(static_cast<bool>(writtenFrame));
  EXPECT_EQUAL(writtenFrame.rowCount(), 4);

  std::vector<double> writtenValues(4);
  std::vector<odc::api::StridedData> decodedColumns;
  decodedColumns.emplace_back(reinterpret_cast<char *>(writtenValues.data()),
                              writtenValues.size(), sizeof(double), sizeof(double));
  odc::api::Decoder decoder({"initial_obsvalue"}, decodedColumns);
  decoder.decode(writtenFrame);
  const double missing = odc::api::Settings::doubleMissingValue();
  EXPECT_EQUAL(writtenValues, std::vector<double>({270., missing, missing, missing}));
}

class OdcNonFiniteWriter : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::OdcNonFiniteWriter";}

  void register_tests() const override {
    auto &tests = eckit::testing::specification();
    tests.emplace_back(CASE("testWriteNonFiniteValuesWithNoFill_OsdfFrame")
      { testWriteNonFiniteValuesWithNoFill(); });
  }

  void clear() const override {}
};

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_ODCNONFINITEWRITER_H_
