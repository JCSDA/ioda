/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/core/OsdfSchemaCheck.h"
#include "ioda/test/containers/OsdfTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"

namespace ioda {
namespace test {

typedef std::vector<std::pair<std::string, osdf::consts::eDataTypes>> SchemaColumns;

// The schema shared by the cases below, before any rank-specific change.
SchemaColumns referenceSchema() {
  return {{"MetaData/latitude", osdf::consts::eFloat},
          {"MetaData/longitude", osdf::consts::eFloat},
          {"MetaData/stationIdentification", osdf::consts::eString},
          {"DiagnosticFlags/flag", osdf::consts::eChar},
          {"DerivedModelValue/var1", osdf::consts::eFloat},
          {"ObsValue/count", osdf::consts::eInt}};
}

// Only rank 0 holds locations, as when the io pool collects the obs or when the reader's
// Identity distribution leaves the ranks outside the io pool empty.
std::size_t numRowsOnThisRank() {
  return (oops::mpi::world().rank() == 0) ? 3 : 0;
}

bool isLastRank() {
  return oops::mpi::world().rank() == oops::mpi::world().size() - 1;
}

// Fails the current case when osdfCheckSchemaAcrossRanks returned although the schemas differ.
// TestException is not an eckit::BadValue, so the cases' catch blocks let it through.
void failBecauseCheckPassed(const std::string & difference) {
  throw eckit::testing::TestException(
    "osdfCheckSchemaAcrossRanks should have thrown eckit::BadValue, since " + difference +
    ", but it returned normally", Here());
}

// -----------------------------------------------------------------------------
template <typename frameType>
void testIdenticalSchemas() {
  // Identical schemas pass, whatever the number of rows on each rank.
  std::unique_ptr<osdf::IFrame> frame =
    createSchemaFrame<frameType>(referenceSchema(), numRowsOnThisRank());
  ioda::osdfCheckSchemaAcrossRanks(*frame, oops::mpi::world(), "identical");

  // So do frames holding no columns at all.
  frame = createSchemaFrame<frameType>({}, 0);
  ioda::osdfCheckSchemaAcrossRanks(*frame, oops::mpi::world(), "no columns");
}

// -----------------------------------------------------------------------------
template <typename frameType>
void testReorderedColumns() {
  const eckit::mpi::Comm & comm = oops::mpi::world();
  if (skipOnSingleTask(comm, "testReorderedColumns")) {
    return;
  }
  // The last rank holds two columns of different types in swapped order: the situation in
  // which distributeObs would pair columns of different types in allGatherv.
  SchemaColumns schema = referenceSchema();
  if (isLastRank()) {
    std::swap(schema[3], schema[4]);
  }
  std::unique_ptr<osdf::IFrame> frame = createSchemaFrame<frameType>(schema, numRowsOnThisRank());
  try {
    ioda::osdfCheckSchemaAcrossRanks(*frame, comm, "reordered");
    failBecauseCheckPassed("the last rank holds two columns in swapped order");
  } catch (const eckit::BadValue & e) {
    // Every rank throws the same message.
    const std::string msg = e.what();
    const std::string lastRank = "rank " + std::to_string(comm.size() - 1);
    EXPECT(msg.find("reordered") != std::string::npos);
    EXPECT(msg.find("Ranks whose schema differs from rank 0: " +
                    std::to_string(comm.size() - 1)) != std::string::npos);
    EXPECT(msg.find("First difference at column 3: rank 0 has 'DiagnosticFlags/flag' (char), " +
                    lastRank + " has 'DerivedModelValue/var1' (float)") != std::string::npos);
    EXPECT(msg.find("Columns on rank 0 but not on " + lastRank + ": none") != std::string::npos);
  }
}

// -----------------------------------------------------------------------------
template <typename frameType>
void testMissingColumn() {
  const eckit::mpi::Comm & comm = oops::mpi::world();
  if (skipOnSingleTask(comm, "testMissingColumn")) {
    return;
  }
  // The last rank lacks a column, as when a filter skips creating it on a rank holding no
  // locations.
  SchemaColumns schema = referenceSchema();
  if (isLastRank()) {
    schema.pop_back();
  }
  std::unique_ptr<osdf::IFrame> frame = createSchemaFrame<frameType>(schema, numRowsOnThisRank());
  try {
    ioda::osdfCheckSchemaAcrossRanks(*frame, comm, "missing");
    failBecauseCheckPassed("the last rank lacks a column");
  } catch (const eckit::BadValue & e) {
    const std::string msg = e.what();
    const std::string lastRank = "rank " + std::to_string(comm.size() - 1);
    EXPECT(msg.find("rank 0 has 6 columns, " + lastRank + " has 5") != std::string::npos);
    EXPECT(msg.find("First difference at column 5: rank 0 has 'ObsValue/count' (int), " +
                    lastRank + " has no column") != std::string::npos);
    EXPECT(msg.find("Columns on rank 0 but not on " + lastRank + ": ObsValue/count") !=
           std::string::npos);
  }
}

// -----------------------------------------------------------------------------
template <typename frameType>
void testRetypedColumn() {
  const eckit::mpi::Comm & comm = oops::mpi::world();
  if (skipOnSingleTask(comm, "testRetypedColumn")) {
    return;
  }
  // The last rank holds a column with the same name but a different type.
  SchemaColumns schema = referenceSchema();
  if (isLastRank()) {
    schema.back().second = osdf::consts::eInt64;
  }
  std::unique_ptr<osdf::IFrame> frame = createSchemaFrame<frameType>(schema, numRowsOnThisRank());
  try {
    ioda::osdfCheckSchemaAcrossRanks(*frame, comm, "retyped");
    failBecauseCheckPassed("the last rank holds a column with a different type");
  } catch (const eckit::BadValue & e) {
    const std::string msg = e.what();
    EXPECT(msg.find("First difference at column 5: rank 0 has 'ObsValue/count' (int), rank " +
                    std::to_string(comm.size() - 1) + " has 'ObsValue/count' (int64)") !=
           std::string::npos);
  }
}

// -----------------------------------------------------------------------------
class OsdfCheckSchema : public oops::Test {
 public:
  OsdfCheckSchema() {}
  virtual ~OsdfCheckSchema() {}

 private:
  std::string testid() const override {return "test::OsdfCheckSchema";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testIdenticalSchemasFrameRows")
      { testIdenticalSchemas<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testIdenticalSchemasFrameCols")
      { testIdenticalSchemas<osdf::FrameCols>(); });
    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testReorderedColumnsFrameRows")
      { testReorderedColumns<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testReorderedColumnsFrameCols")
      { testReorderedColumns<osdf::FrameCols>(); });
    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testMissingColumnFrameRows")
      { testMissingColumn<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testMissingColumnFrameCols")
      { testMissingColumn<osdf::FrameCols>(); });
    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testRetypedColumnFrameRows")
      { testRetypedColumn<osdf::FrameRows>(); });
    ts.emplace_back(CASE("ioda/OsdfCheckSchema/testRetypedColumnFrameCols")
      { testRetypedColumn<osdf::FrameCols>(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
