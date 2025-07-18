/*
 * (C) Crown Copyright 2025 UK Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/io/FileHandle.h"
#include "eckit/testing/Test.h"
#include "ioda/Engines/ODC/OdbTablesRange.h"
#include "odc/api/Odb.h"
#include "oops/runs/Run.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

// -----------------------------------------------------------------------------
class TestsParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(TestsParameters, Parameters)
 public:
  oops::RequiredParameter<std::vector<std::string>> odbFiles{"odb files", this};
};

// -----------------------------------------------------------------------------

CASE("typicalUsage") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  TestsParameters params;
  params.validateAndDeserialize(conf);
  for (const std::string &path : params.odbFiles.value()) {
    oops::Log::test() << "Testing file " << path << std::endl;

    size_t expectedNumTables = 0;
    size_t expectedNumRows = 0;
    odc::api::Reader reader(path, false /*aggregated?*/);
    while (odc::api::Frame frame = reader.next()) {
      ++expectedNumTables;
      expectedNumRows += frame.rowCount();
    }

    size_t numTables = 0;
    size_t numRows = 0;
    for (const odc::core::Table &table :
         ioda::Engines::ODC::OdbTablesRange(std::make_unique<eckit::FileHandle>(path))) {
      ++numTables;
      numRows += table.rowCount();
    }

    EXPECT_EQUAL(numTables, expectedNumTables);
    EXPECT_EQUAL(numRows, expectedNumRows);
  }
}

// -----------------------------------------------------------------------------

// Tests the (rather unlikely) scenario of a multiple OdbTablesIterators traversing the same
// OdbTablesRange at different speeds.
CASE("multipleIterators") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  TestsParameters params;
  params.validateAndDeserialize(conf);
  for (const std::string &path : params.odbFiles.value()) {
    oops::Log::test() << "Testing file " << path << std::endl;

    size_t expectedNumTables = 0;
    size_t expectedNumRows = 0;
    odc::api::Reader reader(path, false /*aggregated?*/);
    while (odc::api::Frame frame = reader.next()) {
      ++expectedNumTables;
      expectedNumRows += frame.rowCount();
    }

    ioda::Engines::ODC::OdbTablesRange range(std::make_unique<eckit::FileHandle>(path));

    ioda::Engines::ODC::OdbTablesIterator it1 = range.begin();
    ioda::Engines::ODC::OdbTablesIterator it2 = range.begin();
    ioda::Engines::ODC::OdbTablesSentinel end = range.end();

    size_t numTables1 = 0;
    size_t numRows1 = 0;
    size_t numTables2 = 0;
    size_t numRows2 = 0;
    while (it1 != end || it2 != end) {
      // Increment it1 twice for each incrementation of it2.

      if (it1 != end) {
        if (it1 != end) {
          numTables1 += 1;
          numRows1 += it1->rowCount();
        }
        ++it1;
      }

      if (it1 != end) {
        if (it1 != end) {
          numTables1 += 1;
          numRows1 += it1->rowCount();
        }
        ++it1;
      }

      if (it2 != end) {
        if (it2 != end) {
          numTables2 += 1;
          numRows2 += it2->rowCount();
        }
        ++it2;
      }
    }

    EXPECT_EQUAL(numTables1, expectedNumTables);
    EXPECT_EQUAL(numRows1, expectedNumRows);
    EXPECT_EQUAL(numTables2, expectedNumTables);
    EXPECT_EQUAL(numRows2, expectedNumRows);
  }
}

// -----------------------------------------------------------------------------

class OdbTablesIterator : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::OdbTablesIterator";}

  void register_tests() const override {}

  void clear() const override {}
};

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
  oops::Run run(argc, argv);
  OdbTablesIterator tests;
  return run.execute(tests);
}
