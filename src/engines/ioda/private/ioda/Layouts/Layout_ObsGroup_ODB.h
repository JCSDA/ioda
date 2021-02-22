#pragma once
/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout_ObsGroup_ODB.h
/// \brief Contains definitions for how ODB data are arranged in ioda internally.

#include <unordered_map>
#include <memory>
#include <string>

#include "ioda/Layout.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
/// Layout for ObsGroup-like data.
class IODA_DL DataLayoutPolicy_ObsGroup_ODB : public DataLayoutPolicy {
  /// \brief Record versioning information for this layout in the ioda object. Provides forward compatability.
  const int32_t ObsGroup_ODB_Layout_Version = 0;
  /// \brief Mapping with ODB equivalents as keys and IODA namings as values
  std::unordered_map<std::string, std::string> Mapping;

public:
  virtual ~DataLayoutPolicy_ObsGroup_ODB();
  void initializeStructure(Group_Base &) const override;
  std::string doMap(const std::string &) const override;
  DataLayoutPolicy_ObsGroup_ODB(const std::string &);
  /// A descriptive name for the policy.
  std::string name() const override;
private:
  void parseMappingFile(const std::string &);
};

}  // namespace detail
}  // namespace ioda
