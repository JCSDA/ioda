/*
 * (C) Copyright 2024 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSDATAFRAME_H_
#define TEST_IODA_OBSDATAFRAME_H_

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/ObsDataFrameRows.h"
#include "ioda/Exception.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testRowPriority() {
  // Configuration contains a list of variables (columns). Use this list
  // to create the row priority data frame by appending the columns.
  eckit::LocalConfiguration configRowPriority;
  ::test::TestEnvironment::config().get("row priority", configRowPriority);

  std::vector<eckit::LocalConfiguration> configColumnData;
    configRowPriority.get("column data", configColumnData);
  double tolerance = configRowPriority.getDouble("tolerance");

  // Instantiate a row priority data frame, and fill it in by appending
  // column data from the test config. After the column is appended, check
  // to see if you get the same data after reading the column
  ObsDataFrameRows dfRow;

  for (std::size_t i = 0; i < configColumnData.size(); ++i) {
    std::string name = configColumnData[i].getString("name");
    std::string type = configColumnData[i].getString("type");
    if (type == "int") {
      // create and write column
      std::vector<int> expectedValues =
        configColumnData[i].getIntVector("values");
      dfRow.appendNewColumn(name, expectedValues);
      // read column and check values
      std::vector<int> values;
      dfRow.getColumn(name, values);
      EXPECT(values == expectedValues);
    } else if (type == "float") {
      // create and write column
      std::vector<float> expectedValues =
        configColumnData[i].getFloatVector("values");
      dfRow.appendNewColumn(name, expectedValues);
      // read column and check values
      std::vector<float> values;
      dfRow.getColumn(name, values);
      EXPECT(oops::are_all_close_relative<float>(
        values, expectedValues, tolerance));
    } else if (type == "double") {
      // create and write column
      std::vector<double> expectedValues =
        configColumnData[i].getDoubleVector("values");
      dfRow.appendNewColumn(name, expectedValues);
      // read column and check values
      std::vector<double> values;
      dfRow.getColumn(name, values);
      EXPECT(oops::are_all_close_relative<double>(
        values, expectedValues, tolerance));
    } else if (type == "string") {
      // create and write column
      std::vector<std::string> expectedValues =
        configColumnData[i].getStringVector("values");
      dfRow.appendNewColumn(name, expectedValues);
      // read column and check values
      std::vector<std::string> values;
      dfRow.getColumn(name, values);
      EXPECT(values == expectedValues);
    } else {
      std::string errMsg = std::string("Unrecognized data type: ") + type +
          std::string("\nMust use one of: 'int', 'float', 'double' or 'string'");
      throw ioda::Exception(errMsg.c_str(), ioda_Here());
    }
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
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSDATAFRAME_H_
