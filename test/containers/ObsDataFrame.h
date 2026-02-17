/*
 * (C) Copyright 2024 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameRows.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void populateOsdf(const eckit::LocalConfiguration & config,
                  std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Configuration contains a list of variables (columns) that
  // specify the data to be added to the data frame.
  std::vector<eckit::LocalConfiguration> configColumns;
  config.get("columns", configColumns);

  // Fill in the OSDF by appending column data from the test config
  for (std::size_t i = 0; i < configColumns.size(); ++i) {
    const std::string name = configColumns[i].getString("name");
    const std::string type = configColumns[i].getString("type");
    if (type == "int") {
      const std::vector<int> values =
        configColumns[i].getIntVector("values");
      testOsdf->appendNewColumn(name, values);
    } else if (type == "int64") {
      const std::vector<int64_t> values =
        configColumns[i].getInt64Vector("values");
      testOsdf->appendNewColumn(name, values);
    } else if (type == "float") {
      const std::vector<float> values =
        configColumns[i].getFloatVector("values");
      testOsdf->appendNewColumn(name, values);
    } else if (type == "char") {
      const std::string tempStr =
        configColumns[i].getString("values");
      const std::vector<char> values(tempStr.begin(), tempStr.end());
      testOsdf->appendNewColumn(name, values);
    } else if (type == "string") {
      const std::vector<std::string> values =
        configColumns[i].getStringVector("values");
      testOsdf->appendNewColumn(name, values);
    } else {
      const std::string errMsg = std::string("Unrecognized data type: ") + type +
          std::string("\nMust use one of: 'int', 'int64', 'float', 'char' or 'string'");
      throw eckit::BadValue(errMsg, Here());
    }
  }
}

// -----------------------------------------------------------------------------
void checkOsdf(const eckit::LocalConfiguration & config,
                  std::unique_ptr<osdf::IFrame> & testOsdf) {
  // Configuration contains a list of variables (columns) that
  // specify the data to be added to the data frame. Plus a
  // tolerance value for floating point comparisons.
  std::vector<eckit::LocalConfiguration> configColumns;
  config.get("columns", configColumns);
  double tolerance = config.getDouble("tolerance");

  // Instantiate a row priority data frame, and fill it in by appending
  // column data from the test config. After the column is appended, check
  // to see if you get the same data after reading the column
  std::vector<std::string> expectedColumnNames(configColumns.size());
  for (std::size_t i = 0; i < configColumns.size(); ++i) {
    std::string name = configColumns[i].getString("name");
    expectedColumnNames[i] = name;
    std::string type = configColumns[i].getString("type");
    if (type == "int") {
      const std::vector<int> expectedValues =
        configColumns[i].getIntVector("values");
      std::vector<int> values;
      testOsdf->getColumn(name, values);
      EXPECT(values == expectedValues);
    } else if (type == "int64") {
      const std::vector<int64_t> expectedValues =
        configColumns[i].getInt64Vector("values");
      std::vector<int64_t> values;
      testOsdf->getColumn(name, values);
      EXPECT(values == expectedValues);
    } else if (type == "float") {
      const std::vector<float> expectedValues =
        configColumns[i].getFloatVector("values");
      std::vector<float> values;
      testOsdf->getColumn(name, values);
      EXPECT(oops::are_all_close_relative<float>(
        values, expectedValues, tolerance));
    } else if (type == "char") {
      const std::string tempStr =
        configColumns[i].getString("values");
      const std::vector<char> expectedValues(tempStr.begin(), tempStr.end());
      std::vector<char> values;
      testOsdf->getColumn(name, values);
      EXPECT(values == expectedValues);
    } else if (type == "string") {
      const std::vector<std::string> expectedValues =
        configColumns[i].getStringVector("values");
      std::vector<std::string> values;
      testOsdf->getColumn(name, values);
      EXPECT(values == expectedValues);
    } else {
      std::string errMsg = std::string("Unrecognized data type: ") + type +
          std::string("\nMust use one of: 'int', 'int64', 'float', 'char' or 'string'");
      throw eckit::BadValue(errMsg, Here());
    }
  }
  std::vector<std::string> columnNames = testOsdf->columnNames();
  EXPECT(columnNames == expectedColumnNames);
}

// -----------------------------------------------------------------------------
void testRowPriority() {
  // Configuration contains a list of variables (columns) that
  // specify the data to be added to the data frame. Plus a
  // tolerance value for floating point comparisons.
  eckit::LocalConfiguration configOsdf;
  ::test::TestEnvironment::config().get("osdf test data", configOsdf);

  // Instantiate a row priority data frame, then populate and check it.
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  populateOsdf(configOsdf, testOsdf);
  checkOsdf(configOsdf, testOsdf);
}

// -----------------------------------------------------------------------------
void testColPriority() {
  // Configuration contains a list of variables (columns) that
  // specify the data to be added to the data frame. Plus a
  // tolerance value for floating point comparisons.
  eckit::LocalConfiguration configOsdf;
  ::test::TestEnvironment::config().get("osdf test data", configOsdf);

  // Instantiate a column priority data frame, then populate and check it.
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();
  populateOsdf(configOsdf, testOsdf);
  checkOsdf(configOsdf, testOsdf);
}

// -----------------------------------------------------------------------------
void testSerializeDeserializeColMetadata() {
  eckit::LocalConfiguration configOsdf;
  ::test::TestEnvironment::config().get("osdf test data", configOsdf);

  // Instantiate a row priority data frame, then populate and check it.
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  populateOsdf(configOsdf, testOsdf);

  // Serialize the column metadata from one data frame, then
  // deserialize it into a new data frame. Check that the
  // column names and types are the same.
  std::string serializedColMetadata = testOsdf->serializeColumnMetadata();
  std::unique_ptr<osdf::IFrame> testOsdf2 = std::make_unique<osdf::FrameRows>();
  testOsdf2->deserializeColumnMetadata(serializedColMetadata);

  // The number of rows in the new data frame should be zero
  EXPECT(testOsdf2->numRows() == 0);

  // The number of columns, column names and types should be the same
  EXPECT(testOsdf->columnNames() == testOsdf2->columnNames());
  for (const auto & name : testOsdf->columnNames()) {
    EXPECT(testOsdf->getColumnType(name) == testOsdf2->getColumnType(name));
  }
}

// -----------------------------------------------------------------------------
class ObsDataFrame : public oops::Test {
 public:
  ObsDataFrame() {}
  virtual ~ObsDataFrame() {}

 private:
  std::string testid() const override {return "test::ObsDataFrame";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsDataFrame/testRowPriority")
      { testRowPriority(); });
    ts.emplace_back(CASE("ioda/ObsDataFrame/testColPriority")
      { testColPriority(); });
    ts.emplace_back(CASE("ioda/ObsDataFrame/testSerializeDeserializeColMetadata")
      { testSerializeDeserializeColMetadata(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

