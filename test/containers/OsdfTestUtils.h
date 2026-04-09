/*
 * (C) Copyright 2025 UCAR
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "ioda/Exception.h"

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/Datum.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

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
void populateColumnMetadata(const std::vector<eckit::LocalConfiguration> & configVec,
                            osdf::ColumnMetadata & colMetadata) {
  // The input configuration vector contains elements with the following specs:
  //    name: column name
  //    type: column data type
  //    units: column data units
  // Transfer each configuration element to a ColumnMetadatum object and push
  // these onto a vector of ColumnMetadatum which can then be used to
  // construct and return the resulting ColumnMetadata ojbect.
  std::vector<osdf::ColumnMetadatum> colMetadatumVec;
  for (std::size_t i = 0; i < configVec.size(); ++i) {
    const std::string colName = configVec[i].getString("name");
    const std::string colTypeStr = configVec[i].getString("type");
    const std::string colUnits = configVec[i].getString("units");

    // Convert type to osdf enum type
    osdf::consts::eDataTypes colType;
    if (colTypeStr == "int") {
      colType = osdf::consts::eDataTypes::eInt;
    } else if (colTypeStr == "int64") {
      colType = osdf::consts::eDataTypes::eInt64;
    } else if (colTypeStr == "float") {
      colType = osdf::consts::eDataTypes::eFloat;
    } else if (colTypeStr == "string") {
      colType = osdf::consts::eDataTypes::eString;
    } else if (colTypeStr == "char") {
      colType = osdf::consts::eDataTypes::eChar;
    } else {
      throw eckit::BadParameter(
        "Unrecognized type value in configuration: " + colTypeStr, Here());
    }

    // Add a new column metadatum to the output vector
    colMetadatumVec.emplace_back(colName, colUnits, colType);
  }
  colMetadata.add(colMetadatumVec);
}

bool testCompareTwoDataRows(osdf::DataRow testRow, osdf::DataRow compareRow,
                            const bool compareIds, const double tolerance ) {
  const std::int32_t testSize    = testRow.getSize();
  const std::int32_t compareSize = compareRow.getSize();

  if (testSize != compareSize) {
    return false;
  }

  if (compareIds) {
    const std::int64_t testId = testRow.getId();
    const std::int64_t compareId = compareRow.getId();

    if (testId != compareId) {
      return false;
    }
  }

  for (std::int32_t index = 0; index < testSize; ++index) {
    const std::int8_t testType = testRow.getColumn(index)->getType();
    const std::int8_t compareType = compareRow.getColumn(index)->getType();
    if (testType != compareType) {
      return false;
    }

    if (testType == osdf::consts::eFloat) {
      std::shared_ptr<osdf::Datum<float>> testDatum
        = std::static_pointer_cast<osdf::Datum<float>>(testRow.getColumn(index));
      std::shared_ptr<osdf::Datum<float>> compareDatum
        = std::static_pointer_cast<osdf::Datum<float>>(compareRow.getColumn(index));

      if (fabs(testDatum->getValue() - compareDatum->getValue()) > tolerance) {
        return false;
      }
    } else {
      bool areEqual
        = osdf::FrameUtils::callWithSupportedType(testType, [&](auto typeDiscriminator) -> bool {
            using T = decltype(typeDiscriminator);
            std::shared_ptr<osdf::Datum<T>> testDatum
              = std::static_pointer_cast<osdf::Datum<T>>(testRow.getColumn(index));
            std::shared_ptr<osdf::Datum<T>> compareDatum
              = std::static_pointer_cast<osdf::Datum<T>>(compareRow.getColumn(index));
            return (testDatum->getValue() == compareDatum->getValue());
          });
      if (!areEqual) {
        return false;
      }
    }
  }
  return true;
}

// ------------------------------------------------------------------------------
template <typename Action>
auto callWithSupportedTypeString(const std::string &typeString,
                                 const std::string& errMsg, const Action &action) {
  if (typeString == "int") {
    return action(int());
  } else if (typeString == "int64") {
    return action(int64_t());
  } else if (typeString == "float") {
    return action(float());
  } else if (typeString == "string") {
    return action(std::string());
  } else if (typeString == "char") {
    return action(char());
  } else {
    throw eckit::BadParameter(errMsg, Here());
  }
}

// -----------------------------------------------------------------------------
void populateFrameMetadata(const eckit::LocalConfiguration & frameMetadataConfig,
                           osdf::FrameMetadata & osdfMetadata) {
  oops::Log::debug() << "Frame metadata configuration: " << frameMetadataConfig << std::endl;
  std::vector<std::string> varsWithChans =
    frameMetadataConfig.getStringVector("variables with channels");
  std::unordered_set<std::string> varsWithChansSet(
    varsWithChans.begin(), varsWithChans.end());
  osdfMetadata.setVarsWithChans(varsWithChansSet);
  osdfMetadata.setChanNums(frameMetadataConfig.getIntVector("channel numbers"));
  osdfMetadata.setDateTimeEpoch(frameMetadataConfig.getString("datetime epoch"));
  osdfMetadata.setNumVars(frameMetadataConfig.getInt("number of variables"));
  const std::vector<eckit::LocalConfiguration> varDimNamesConfig =
    frameMetadataConfig.getSubConfigurations("variable dim names");
  for (const auto & varDimNameConfig : varDimNamesConfig) {
    const std::string varName = varDimNameConfig.getString("name");
    const std::vector<std::string> dimNames =
      varDimNameConfig.getStringVector("dim names");
    osdfMetadata.addVarDimNames(varName, dimNames);
  }
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

    std::string errMsg
      = std::string("Unrecognized data type: ") + type
        + std::string("\nMust use one of: 'int', 'int64', 'float', 'string', or 'char'");
    callWithSupportedTypeString(type, errMsg,
                                [&](auto typeDiscriminator) {
                                  using T = decltype(typeDiscriminator);
                                  std::vector<T> expectedValues
                                    = replaceMissingValues<T>(stringVals);
                                  try {
                                    std::string unit = config[i].getString("unit");
                                    osdf->appendNewColumn(name, expectedValues, unit);
                                  } catch(eckit::Exception&) {
                                    osdf->appendNewColumn(name, expectedValues);
                                  }
                                });
  }
}

// -----------------------------------------------------------------------------
template <typename varType>
void compareColumnExact(const std::string & testColumnName,
                        const std::string & refColumnName,
                        const std::unique_ptr<osdf::IFrame> & testOsdf,
                        const std::unique_ptr<osdf::IFrame> & refOsdf) {
      std::vector<varType> testValues;
      testOsdf->getColumn(testColumnName, testValues);
      std::vector<varType> refValues;
      refOsdf->getColumn(refColumnName, refValues);
      EXPECT(testValues.size() == refValues.size());
      std::string testColumnUnits = testOsdf->getColumnUnits(testColumnName);
      std::string refColumnUnits  = refOsdf->getColumnUnits(refColumnName);
      EXPECT(testColumnUnits == refColumnUnits);
      for (std::size_t j = 0; j < testValues.size(); ++j) {
        EXPECT(testValues[j] == refValues[j]);
      }
}

void compareFrames(const std::unique_ptr<osdf::IFrame> &testOsdf,
                   const std::vector<std::string> &testColumnNames,
                   const std::vector<std::string> &testColumnTypes,
                   const std::unique_ptr<osdf::IFrame> &refOsdf,
                   const std::vector<std::string> &refColumnNames,
                   const std::vector<std::string> &refColumnTypes,
                   const double tolerance, bool compareFrameTypes) {
  // Check that the frames have the same underlying type (Row or Col)
  if (compareFrameTypes) {
    EXPECT(testOsdf->frameType() == refOsdf->frameType());
  }
  // Check that the test and reference data frames have the same columns
  EXPECT(testColumnNames.size() == refColumnNames.size());
  for (std::size_t i = 0; i < testColumnNames.size(); ++i) {
    EXPECT(testColumnNames[i] == refColumnNames[i]);
    EXPECT(testColumnTypes[i] == refColumnTypes[i]);
  }

  // Check that the test and reference data frames have the same data
  for (std::size_t i = 0; i < testColumnNames.size(); ++i) {
    std::string errMsg
      = std::string("Unrecognized data type: ") + testColumnTypes[i]
        + std::string("\nMust use one of: 'int', 'int64', 'float', 'string', or 'char'");

    callWithSupportedTypeString(testColumnTypes[i], errMsg, [&](auto typeDiscriminator) {
      using T = decltype(typeDiscriminator);
      if (testColumnTypes[i] == "float") {
        std::string testColumnUnits = testOsdf->getColumnUnits(testColumnNames[i]);
        std::string refColumnUnits  = refOsdf->getColumnUnits(refColumnNames[i]);
        EXPECT(testColumnUnits == refColumnUnits);
        std::vector<float> testValues;
        testOsdf->getColumn(testColumnNames[i], testValues);
        std::vector<float> refValues;
        refOsdf->getColumn(refColumnNames[i], refValues);
        EXPECT(testValues.size() == refValues.size());
        for (std::size_t j = 0; j < testValues.size(); ++j) {
          EXPECT(fabs(testValues[j] - refValues[j]) < tolerance);
        }
        return;
      }
      compareColumnExact<T>(testColumnNames[i], refColumnNames[i], testOsdf, refOsdf);
    });
  }
}

// --------------------------------------------------------------------------------------------------
// Horrible repeat so functions can take FrameCols and FrameRows directly
template <typename frameType>
void populateFrame(const std::vector<eckit::LocalConfiguration> &config,
                   frameType &osdf, std::vector<std::string> &columnNames,
                   std::vector<std::string> &columnTypes) {
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

    std::string errMsg
      = std::string("Unrecognized data type: ") + type
        + std::string("\nMust use one of: 'int', 'int64', 'float', 'string', or 'char'");
    callWithSupportedTypeString(type, errMsg, [&](auto typeDiscriminator) {
      using T = decltype(typeDiscriminator);
      std::vector<T> expectedValues = replaceMissingValues<T>(stringVals);
      try {
        std::string unit = config[i].getString("unit");
        osdf.appendNewColumn(name, expectedValues, unit);
      } catch (eckit::Exception&) {
        osdf.appendNewColumn(name, expectedValues);
      }
    });
  }
}

template <typename varType, typename frameType>
void compareColumnExact(const std::string &testColumnName, const std::string &refColumnName,
                        const frameType &testOsdf,
                        const frameType &refOsdf) {
  std::vector<varType> testValues;
  testOsdf.getColumn(testColumnName, testValues);
  std::vector<varType> refValues;
  refOsdf.getColumn(refColumnName, refValues);
  EXPECT(testValues.size() == refValues.size());
  for (std::size_t j = 0; j < testValues.size(); ++j) {
    EXPECT(testValues[j] == refValues[j]);
  }
}

template <typename frameType>
void compareFrames(const frameType &testOsdf,
                   const std::vector<std::string> &testColumnNames,
                   const std::vector<std::string> &testColumnTypes,
                   const frameType &refOsdf,
                   const std::vector<std::string> &refColumnNames,
                   const std::vector<std::string> &refColumnTypes,
                   const double tolerance,
                   bool compareFrameTypes) {
  // Check that the frames have the same underlying type (Row or Col)
  if (compareFrameTypes) {
    EXPECT(testOsdf.frameType() == refOsdf.frameType());
  }
  // Check that the test and reference data frames have the same columns
  EXPECT(testColumnNames.size() == refColumnNames.size());
  for (std::size_t i = 0; i < testColumnNames.size(); ++i) {
    EXPECT(testColumnNames[i] == refColumnNames[i]);
    EXPECT(testColumnTypes[i] == refColumnTypes[i]);
  }

  // Check that the test and reference data frames have the same data
  for (std::size_t i = 0; i < testColumnNames.size(); ++i) {
    std::string errMsg
      = std::string("Unrecognized data type: ") + testColumnTypes[i]
        + std::string("\nMust use one of: 'int', 'int64', 'float', 'string', or 'char'");

    callWithSupportedTypeString(testColumnTypes[i], errMsg, [&](auto typeDiscriminator) {
      using T = decltype(typeDiscriminator);
      if (testColumnTypes[i] == "float") {
        std::string testColumnUnits = testOsdf.getColumnUnits(testColumnNames[i]);
        std::string refColumnUnits  = refOsdf.getColumnUnits(refColumnNames[i]);
        EXPECT(testColumnUnits == refColumnUnits);
        std::vector<float> testValues;
        testOsdf.getColumn(testColumnNames[i], testValues);
        std::vector<float> refValues;
        refOsdf.getColumn(refColumnNames[i], refValues);
        EXPECT(testValues.size() == refValues.size());
        for (std::size_t j = 0; j < testValues.size(); ++j) {
          EXPECT(fabs(testValues[j] - refValues[j]) < tolerance);
        }
        return;
      }
      compareColumnExact<T>(testColumnNames[i], refColumnNames[i], testOsdf, refOsdf);
    });
  }
}

}  // namespace test
}  // namespace ioda
