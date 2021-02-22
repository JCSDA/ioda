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

void DataLayoutPolicy_ObsGroup_ODB::parseMappingFile(const std::string &nameMapFile) {
  eckit::PathName yamlPath = nameMapFile;
  eckit::YAMLConfiguration conf(yamlPath);
  eckit::LocalConfiguration ioda(conf, "ioda");
  std::vector<eckit::LocalConfiguration> listVariables;
  ioda.get("variables", listVariables);
  for (auto const& variable : listVariables) {
    std::string name; // The naming scheme to be used in the ioda file
    std::string source; // The naming scheme in the source file
    variable.get("name", name);
    variable.get("source", source);
    Mapping[source] = name;
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
  // components and reversed. Additionally, if the string is a value in the mapping file,
  // it is replaced with its value. All other strings are passed through untouched.
  std::string mappedStr;
  // If the input string is present as a key in the mapping, use its corresponding value instead.
  auto it = Mapping.find(str);
  if (it != Mapping.end()) {
    mappedStr = it->second;
  } else {
    mappedStr = str;
  }

  mappedStr = reverseStringSwapDelimiter(mappedStr);
  return mappedStr;
}

std::string DataLayoutPolicy_ObsGroup_ODB::name() const { return std::string{"ObsGroup ODB v1"}; }

}  // namespace detail
}  // namespace ioda
