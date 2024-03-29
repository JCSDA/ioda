#pragma once
/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout_ObsGroup_ODB.h
/// \brief Contains definitions for how ODB data are arranged in ioda internally.

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "ioda/Layout.h"
#include "ioda/defs.h"

namespace boost {
template <class T> class optional;
}

namespace eckit {
class LocalConfiguration;
}

namespace ioda {
namespace detail {

class ODBLayoutParameters;

/// Layout for ObsGroup-like data.
class IODA_DL DataLayoutPolicy_ObsGroup_ODB : public DataLayoutPolicy {
  struct variableStorageInformation {
    std::string iodaName;
    std::pair<bool,std::string> inputUnit;
  };

  /// \brief Record versioning information for this layout in the ioda object. Provides forward compatability.
  const int32_t ObsGroup_ODB_Layout_Version = 0;
  /// \brief Mapping with ODB equivalents as keys and IODA naming/unit pairs as values
  std::unordered_map<std::string, variableStorageInformation> Mapping;
  /// \brief Metadata for generating a variable in IODA from multiple component variables (same across components).
  struct ComplementaryVariableOutputMetadata {
    std::string outputName;
    std::type_index outputVariableDataType;
    DataLayoutPolicy::MergeMethod mergeMethod;
    size_t inputVariableCount;
  };
  typedef std::pair<size_t, std::shared_ptr<ComplementaryVariableOutputMetadata>>
  complementaryVariableMetaData; //the position and the derived variable's MetaData
  //The component strings mapped to the common-across-components information for creating a derived variable
  std::unordered_map<std::string, complementaryVariableMetaData> complementaryVariableDataMap;

 public:
  virtual ~DataLayoutPolicy_ObsGroup_ODB();
  void initializeStructure(Group_Base &) const override;
  std::string doMap(const std::string &) const override;
  bool isComplementary(const std::string &) const override;
  bool isMapped(const std::string &) const override;
  bool isMapOutput(const std::string &) const override;
  size_t getComplementaryPosition(const std::string &) const override;
  size_t getInputsNeeded(const std::string &) const override;
  DataLayoutPolicy::MergeMethod getMergeMethod(const std::string &) const override;
  std::pair<bool,std::string> getUnit(const std::string &) const override;
  std::string getOutputNameFromComponent(const std::string &) const override;
  std::type_index getOutputVariableDataType(const std::string &) const override;
  DataLayoutPolicy_ObsGroup_ODB(const std::string &, const std::vector<std::string> & = {});
  /// A descriptive name for the policy.
  std::string name() const override;
 private:
  void parseMappingFile(const std::string &);
  void addUnchangedVariableName(const std::string &);
  void addMapping(const std::string &inputName,
                  const std::string &outputName,
                  const boost::optional<std::string> &unit);
  void parseNameChanges(const ODBLayoutParameters &params);
  void parseComponentVariables(const ODBLayoutParameters &params);
  void parseVarnoDependentColumns(const ODBLayoutParameters &params);
  DataLayoutPolicy::MergeMethod parseMergeMethod(const std::string &);
};

}  // namespace detail
}  // namespace ioda
