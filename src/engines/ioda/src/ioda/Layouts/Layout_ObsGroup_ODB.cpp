/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout_ObsGroup_ODB.cpp
/// \brief Contains implementations for how ODB data are arranged in ioda internally.

#include "ioda/Layouts/Layout_ObsGroup_ODB.h"

#include "ioda/Layouts/Layout_Utils.h"

#include "ioda/Group.h"
#include "ioda/Layout.h"
#include "ioda/defs.h"

#include "eckit/config/Configuration.h"
#include "eckit/config/LocalConfiguration.h"
#include "eckit/config/YAMLConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/filesystem/LocalPathName.h"
#include "eckit/filesystem/PathName.h"

#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

namespace ioda {
namespace detail {
DataLayoutPolicy_ObsGroup_ODB::~DataLayoutPolicy_ObsGroup_ODB(){}

DataLayoutPolicy_ObsGroup_ODB::DataLayoutPolicy_ObsGroup_ODB(const std::string &fileMappingName) {
  parseMappingFile(fileMappingName);
}

DataLayoutPolicy::MergeMethod DataLayoutPolicy_ObsGroup_ODB::parseMergeMethod(
    const std::string &method) {
  if (method != "concat") {
    throw eckit::MethodNotYetImplemented("Concatenation is the only supported merge method.");
  }
  return DataLayoutPolicy::MergeMethod::Concat;
}

void DataLayoutPolicy_ObsGroup_ODB::parseMappingFile(const std::string &nameMapFile) {
  eckit::PathName yamlPath = nameMapFile;
  eckit::YAMLConfiguration conf(yamlPath);
  eckit::LocalConfiguration ioda(conf, "ioda");
  parseNameChanges(ioda);
  parseComponentVariables(ioda);
}

void DataLayoutPolicy_ObsGroup_ODB::parseNameChanges(const eckit::LocalConfiguration &conf)
{
  std::vector<eckit::LocalConfiguration> listVariables;
  conf.get("variables", listVariables);
  for (auto const& variable : listVariables) {
    std::string name; // The naming scheme to be used in the ioda file
    std::string source; // The naming scheme in the source file
    variable.get("name", name);
    variable.get("source", source);
    Mapping[source] = name;
  }
}

void DataLayoutPolicy_ObsGroup_ODB::parseComponentVariables(const eckit::LocalConfiguration &conf)
{
  std::vector<eckit::LocalConfiguration> listVariables;
  conf.get("complementary variables", listVariables);
  for (auto const& variable : listVariables) {
    std::string outputVariableDataType;
    variable.get("output variable data type", outputVariableDataType);
    if (outputVariableDataType != std::string("string")) {
      throw eckit::MethodNotYetImplemented(std::string("YAML mapping file: the output variable") +
                                           std::string("data type for a derived variable is not") +
                                           std::string(" a 'string'"));
    }
    std::string mergeMethodString;
    variable.get("merge method", mergeMethodString);
    MergeMethod mergeMethod = parseMergeMethod(mergeMethodString);
    std::string outputName;
    variable.get("output name", outputName);
    std::vector<std::string> inputVariableNames;
    variable.get("input names", inputVariableNames);
    if (std::find(inputVariableNames.begin(), inputVariableNames.end(), outputName) !=
        inputVariableNames.end()) {
      throw eckit::ReadError(std::string("YAML mapping file has a complementary variable name") +
                             std::string("matching a derived variable name."));
    }
    std::type_index outputTypeIndex = typeid(std::string);
    //augustweinbren: outputTypeIndex will be defined using outputVariableDataType when other
    //merge methods are available

    std::shared_ptr<ComplementaryVariableOutputMetadata> sharedOutputMetaData(
          new ComplementaryVariableOutputMetadata{outputName, outputTypeIndex, mergeMethod, 0});
    size_t inputIndex = 0;
    for (const std::string &input : inputVariableNames) {
      complementaryVariableDataMap[input] = std::make_pair(inputIndex, sharedOutputMetaData);
      inputIndex++;
    }
    sharedOutputMetaData->inputVariableCount = inputIndex;
  }
}

void DataLayoutPolicy_ObsGroup_ODB::initializeStructure(Group_Base &g) const {
  // First, set an attribute to indicate that the data are managed
  // by this data policy.
  g.atts.add<std::string>("_ioda_layout", std::string("ObsGroup_ODB"));
  g.atts.add<int32_t>("_ioda_layout_version", ObsGroup_ODB_Layout_Version);

  // Create the default containers - currently ignored as these are
  // dynamically created.
  /*
  g.create("MetaData");
  g.create("ObsBias");
  g.create("ObsError");
  g.create("ObsValue");
  g.create("PreQC");
  */
}

std::string DataLayoutPolicy_ObsGroup_ODB::doMap(const std::string &str) const {
  // If the string contains '@', then it needs to be broken into
  // components and reversed. Additionally, if the string is a key in the mapping file,
  // it is replaced with its value. All other strings are passed through untouched.
  std::string mappedStr;
  auto it = Mapping.find(str);
  if (it != Mapping.end()) {
    mappedStr = it->second;
  } else {
    mappedStr = str;
  }

  mappedStr = reverseStringSwapDelimiter(mappedStr);
  return mappedStr;
}

bool DataLayoutPolicy_ObsGroup_ODB::isComplementary(const std::string &inputVariable) const
{
  if (complementaryVariableDataMap.find(inputVariable) != complementaryVariableDataMap.end())
    return true;
  else
    return false;
}

size_t DataLayoutPolicy_ObsGroup_ODB::getComplementaryPosition(const std::string &input) const
{
  if (!isComplementary(input))
    throw eckit::ReadError(input + " was not found to be a complementary variable.");
  return complementaryVariableDataMap.at(input).first;
}

size_t DataLayoutPolicy_ObsGroup_ODB::getInputsNeeded(const std::string &input) const
{
  if (!isComplementary(input))
    throw eckit::ReadError(input + std::string(" was not found to be a complementary variable."));
  return complementaryVariableDataMap.at(input).second->inputVariableCount;
}

std::string DataLayoutPolicy_ObsGroup_ODB::getOutputNameFromComponent(
    const std::string &input) const
{
  if (!isComplementary(input))
    throw eckit::ReadError(input + " was not found to be a complementary variable.");
  return complementaryVariableDataMap.at(input).second->outputName;
}

std::type_index DataLayoutPolicy_ObsGroup_ODB::getOutputVariableDataType(
    const std::string &input) const
{
  if (!isComplementary(input))
    throw eckit::ReadError(input + " was not found to be a complementary variable.");
  return complementaryVariableDataMap.at(input).second->outputVariableDataType;
}

DataLayoutPolicy::MergeMethod DataLayoutPolicy_ObsGroup_ODB::getMergeMethod(
    const std::string &input) const
{
  if (!isComplementary(input))
    throw eckit::ReadError(input + " was not found to be a complementary variable.");
  return complementaryVariableDataMap.at(input).second->mergeMethod;
}

std::string DataLayoutPolicy_ObsGroup_ODB::name() const { return std::string{"ObsGroup ODB v1"}; }

}  // namespace detail
}  // namespace ioda
