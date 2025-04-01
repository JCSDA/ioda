/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_READERFILTER_H_
#define TEST_IODA_READERFILTER_H_

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
#include "ioda/reader/filter/filterObsContainer.hpp"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
template <typename varType>
std::vector<varType> replaceMissingValues(const std::vector<std::string> & stringVals) {
  const varType missingValue = util::missingValue<varType>();
  std::vector<varType> varVals(stringVals.size());
  for (std::size_t i = 0; i < stringVals.size(); ++i) {
    if (stringVals[i] == "missing") {
      varVals[i] = missingValue;
    } else {
      std::stringstream ss(stringVals[i]);
      ss >> varVals[i];
    }
  }
  return varVals;
}

// -----------------------------------------------------------------------------
void populateFrame(const std::vector<eckit::LocalConfiguration> & config,
                  std::unique_ptr<osdf::IFrame> & osdf,
                  std::vector<std::string> & columnNames,
                  std::vector<std::string> & columnTypes) {
  columnNames.clear();
  columnTypes.clear();
  // Configuration contains a list of variables (columns). Use this list
  // to populate the data frame. To handle missing values in the test data,
  // read in the YAML specified values as a vector of strings. Then convert
  // those values to the target type except if the entry is "missing" in which
  // you assign the JEDI missing value for the target type.
  for (std::size_t i = 0; i < config.size(); ++i) {
    std::string name = config[i].getString("name");
    columnNames.push_back(name);
    std::string type = config[i].getString("type");
    columnTypes.push_back(type);
    std::vector<std::string> stringVals = config[i].getStringVector("values");
    if (type == "int") {
      // create and write column
      std::vector<int> expectedValues = replaceMissingValues<int>(stringVals);
      osdf->appendNewColumn(name, expectedValues);
    } else if (type == "int64") {
      // create and write column
      std::vector<int64_t> expectedValues = replaceMissingValues<int64_t>(stringVals);
      osdf->appendNewColumn(name, expectedValues);
    } else if (type == "float") {
      // create and write column
      std::vector<float> expectedValues = replaceMissingValues<float>(stringVals);
      osdf->appendNewColumn(name, expectedValues);
    } else if (type == "double") {
      // create and write column
      std::vector<double> expectedValues = replaceMissingValues<double>(stringVals);
      osdf->appendNewColumn(name, expectedValues);
    } else if (type == "string") {
      // create and write column
      std::vector<std::string> expectedValues = replaceMissingValues<std::string>(stringVals);
      osdf->appendNewColumn(name, expectedValues);
    } else {
      std::string errMsg = std::string("Unrecognized data type: ") + type +
          std::string("\nMust use one of: 'int', 'float', 'double' or 'string'");
      throw eckit::BadParameter(errMsg, Here());
    }
  }
}

void compareFrames(const std::unique_ptr<osdf::IFrame> & testOsdf,
                   const std::vector<std::string> & testColumnNames,
                   const std::vector<std::string> & testColumnTypes,
                   const std::unique_ptr<osdf::IFrame> & refOsdf,
                   const std::vector<std::string> & refColumnNames,
                   const std::vector<std::string> & refColumnTypes,
                   const double tolerance) {
  // Check that the test and reference data frames have the same columns
  EXPECT(testColumnNames.size() == refColumnNames.size());
  for (std::size_t i = 0; i < testColumnNames.size(); ++i) {
    EXPECT(testColumnNames[i] == refColumnNames[i]);
    EXPECT(testColumnTypes[i] == refColumnTypes[i]);
  }

  // Check that the test and reference data frames have the same data
  for (std::size_t i = 0; i < testColumnNames.size(); ++i) {
    if (testColumnTypes[i] == "int") {
      std::vector<int> testValues;
      testOsdf->getColumn(testColumnNames[i], testValues);
      std::vector<int> refValues;
      refOsdf->getColumn(refColumnNames[i], refValues);
      EXPECT(testValues.size() == refValues.size());
      for (std::size_t j = 0; j < testValues.size(); ++j) {
        EXPECT(testValues[j] == refValues[j]);
      }
    } else if (testColumnTypes[i] == "int64") {
      std::vector<int64_t> testValues;
      testOsdf->getColumn(testColumnNames[i], testValues);
      std::vector<int64_t> refValues;
      refOsdf->getColumn(refColumnNames[i], refValues);
      EXPECT(testValues.size() == refValues.size());
      for (std::size_t j = 0; j < testValues.size(); ++j) {
        EXPECT(testValues[j] == refValues[j]);
      }
    } else if (testColumnTypes[i] == "float") {
      std::vector<float> testValues;
      testOsdf->getColumn(testColumnNames[i], testValues);
      std::vector<float> refValues;
      refOsdf->getColumn(refColumnNames[i], refValues);
      EXPECT(testValues.size() == refValues.size());
      for (std::size_t j = 0; j
          < testValues.size(); ++j) {
        EXPECT(fabs(testValues[j] - refValues[j]) < tolerance);
      }
    } else if (testColumnTypes[i] == "double") {
      std::vector<double> testValues;
      testOsdf->getColumn(testColumnNames[i], testValues);
      std::vector<double> refValues;
      refOsdf->getColumn(refColumnNames[i], refValues);
      EXPECT(testValues.size() == refValues.size());
      for (std::size_t j = 0; j < testValues.size(); ++j) {
        EXPECT(fabs(testValues[j] - refValues[j]) < tolerance);
      }
    } else if (testColumnTypes[i] == "string") {
      std::vector<std::string> testValues;
      testOsdf->getColumn(testColumnNames[i], testValues);
      std::vector<std::string> refValues;
      refOsdf->getColumn(refColumnNames[i], refValues);
      EXPECT(testValues.size() == refValues.size());
      for (std::size_t j = 0; j < testValues.size(); ++j) {
        EXPECT(testValues[j] == refValues[j]);
      }
    } else {
      std::string errMsg = std::string("Unrecognized data type: ") + testColumnTypes[i] +
          std::string("\nMust use one of: 'int', 'float', 'double' or 'string'");
      throw eckit::BadParameter(errMsg, Here());
    }
  }
}

void testFrameRows() {
  // Configuration contains a time window spec and a list of variables (columns).
  // Construct a time window object and use the variable list to create the
  // the row priority data frame by appending the columns.
  const eckit::LocalConfiguration timeWinConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");
  const util::TimeWindow timeWindow(timeWinConfig);

  const std::vector<eckit::LocalConfiguration> configColumnData =
      ::test::TestEnvironment::config().getSubConfigurations("test column data");
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  // Create an instance of a row priority data frame and populate it with
  // test data from the config file.
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(configColumnData, testOsdf, testColumnNames, testColumnTypes);
  oops::Log::info() << "testFrameRows: initial contents" << std::endl;
  testOsdf->print();

  // Read in the expected results after filtering
  const std::vector<eckit::LocalConfiguration> configRefData =
  ::test::TestEnvironment::config().getSubConfigurations("expected filtered data");
  std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameRows>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefData, refOsdf, refColumnNames, refColumnTypes);

  // Run the filters and check the results
  reader::filterObsContainer(timeWindow, testOsdf);
  oops::Log::info() << "testFrameRows: after filtering" << std::endl;
  compareFrames(testOsdf, testColumnNames, testColumnTypes, refOsdf, refColumnNames, refColumnTypes,
    tolerance);
  testOsdf->print();
}

void testFrameCols() {
  // Configuration contains a time window spec and a list of variables (columns).
  // Construct a time window object and use the variable list to create the
  // column priority data frame by appending the columns.
  const eckit::LocalConfiguration timeWinConfig =
      ::test::TestEnvironment::config().getSubConfiguration("time window");
  const util::TimeWindow timeWindow(timeWinConfig);

  const std::vector<eckit::LocalConfiguration> configColumnData =
      ::test::TestEnvironment::config().getSubConfigurations("test column data");
  const double tolerance = ::test::TestEnvironment::config().getDouble("tolerance");

  // Create an instance of a row priority data frame and populate it with
  // data from the config file.
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameCols>();
  std::vector<std::string> testColumnNames;
  std::vector<std::string> testColumnTypes;
  populateFrame(configColumnData, testOsdf, testColumnNames, testColumnTypes);
  oops::Log::info() << "testFrameRows: initial contents" << std::endl;
  testOsdf->print();

  // Read in the expected results after filtering
  const std::vector<eckit::LocalConfiguration> configRefData =
  ::test::TestEnvironment::config().getSubConfigurations("expected filtered data");
  std::unique_ptr<osdf::IFrame> refOsdf = std::make_unique<osdf::FrameCols>();
  std::vector<std::string> refColumnNames;
  std::vector<std::string> refColumnTypes;
  populateFrame(configRefData, refOsdf, refColumnNames, refColumnTypes);

  // Run the filters and check the results
  reader::filterObsContainer(timeWindow, testOsdf);
  oops::Log::info() << "testFrameCols: after filtering" << std::endl;
  compareFrames(testOsdf, testColumnNames, testColumnTypes, refOsdf, refColumnNames, refColumnTypes,
    tolerance);
  testOsdf->print();
}

// -----------------------------------------------------------------------------
class ReaderFilter : public oops::Test {
 public:
  ReaderFilter() {}
  virtual ~ReaderFilter() {}

 private:
  std::string testid() const override {return "test::ReaderFilter";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameRows")
      { testFrameRows(); });
    ts.emplace_back(CASE("ioda/ReaderFilter/testFrameCols")
      { testFrameCols(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_READERFILTER_H_
