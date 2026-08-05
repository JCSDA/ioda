/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_layout_internal Internal Groups and Data Layout
 * \brief Private API
 * \ingroup ioda_internals
 *
 * @{
 * \file Layout_ObsGroup_ODB.cpp
 * \brief Contains implementations for how ODB data are arranged in ioda internally.
 */

#include "./Layout_ObsGroup_ODB.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "./Layout_ObsGroup_ODB_Params.h"
#include "eckit/config/LocalConfiguration.h"
#include "eckit/config/YAMLConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/filesystem/LocalPathName.h"
#include "eckit/filesystem/PathName.h"
#include "ioda/Group.h"
#include "ioda/Layout.h"
#include "ioda/Misc/StringFuncs.h"

namespace ioda {
namespace detail {
DataLayoutPolicy_ObsGroup_ODB::~DataLayoutPolicy_ObsGroup_ODB() {}

DataLayoutPolicy_ObsGroup_ODB::DataLayoutPolicy_ObsGroup_ODB(
  const std::string &fileMappingName, const std::vector<std::string> &nonODBVariables) {
  parseMappingFile(fileMappingName);
  for (auto const &str : nonODBVariables) {
    addUnchangedVariableName(str);
  }
}

void DataLayoutPolicy_ObsGroup_ODB::parseMappingFile(const std::string &nameMapFile) {
  eckit::PathName yamlPath = nameMapFile;
  eckit::YAMLConfiguration conf(yamlPath);
  ODBLayoutParameters mappingParams;
  mappingParams.validateAndDeserialize(conf);
  parseNameChanges(mappingParams);
  parseVarnoDependentColumns(mappingParams);
}

void DataLayoutPolicy_ObsGroup_ODB::parseNameChanges(const ODBLayoutParameters &params) {
  for (VariableParameters const &variable : params.variables.value()) {
    if (variable.mode != IoMode::WRITE) addMapping(variable.source, variable.name, variable.unit);
  }
}

/// Add an unchanged variable to the mapping. Used to ensure that
/// all of the fundamental variables do not falsely throw an exception.
void DataLayoutPolicy_ObsGroup_ODB::addUnchangedVariableName(const std::string &str) {
  if (isMapped(str) || isMapOutput(str)) {
    throw eckit::Exception("Attempting to re-add existing variable to mapping: " + str, Here());
  }
  Mapping[str] = {str, {false, ""}};
}

void DataLayoutPolicy_ObsGroup_ODB::addMapping(const std::string &inputName,
                                               const std::string &outputName,
                                               const boost::optional<std::string> &unit) {
  if (unit)
    Mapping[inputName] = {outputName, {true, *unit}};
  else
    Mapping[inputName] = {outputName, {false, ""}};
}

void DataLayoutPolicy_ObsGroup_ODB::parseVarnoDependentColumns(const ODBLayoutParameters &params) {
  for (VarnoDependentColumnParameters const &column : params.varnoDependentColumns.value()) {
    const std::string inputPrefix  = column.source.value() + "/";
    const std::string outputPrefix = convertV1PathToV2Path(column.groupName.value()) + "/";
    for (VarnoToVariableNameMappingParameters const &mapping : column.mappings.value()) {
      addMapping(inputPrefix + std::to_string(mapping.varno.value()),
                 outputPrefix + mapping.name.value(), mapping.unit);
    }
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
  // components and reversed. Additionally, if the string is a key (ODB name) in the mapping file,
  // it is replaced with its value. All other strings are passed through untouched.

  std::string mappedStr;

  auto it = Mapping.find(str);
  if (it != Mapping.end()) {
    mappedStr = (it->second).iodaName;
  } else {
    mappedStr = str;
  }

  mappedStr = convertV1PathToV2Path(mappedStr);
  return mappedStr;
}

bool DataLayoutPolicy_ObsGroup_ODB::isMapped(const std::string &input) const {
  return (Mapping.find(input) != Mapping.end());
}

bool DataLayoutPolicy_ObsGroup_ODB::isMapOutput(const std::string &output) const {
  for (const std::pair<const std::string, variableStorageInformation> &entry : Mapping) {
    if (entry.second.iodaName == output) return true;
  }

  return false;
}

// Gets units given ioda style name
std::pair<bool, std::string> DataLayoutPolicy_ObsGroup_ODB::getUnitFromIodaName(
  const std::string &iodaName) const {
  for (const std::pair<const std::string, variableStorageInformation> &entry : Mapping) {
    if (entry.second.iodaName == iodaName) {
      return entry.second.inputUnit;
    }
  }
  throw eckit::ReadError(iodaName + " was not found to correspond to an ODB source variable.");
}

std::string DataLayoutPolicy_ObsGroup_ODB::name() const { return std::string{"ObsGroup ODB v1"}; }

}  // namespace detail
}  // namespace ioda
