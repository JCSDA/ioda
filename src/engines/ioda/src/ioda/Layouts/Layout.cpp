/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout.cpp
/// \brief Contains implementations for how data are arranged in ioda internally.

#include "ioda/Layout.h"

#include <exception>
#include <vector>

#include "ioda/defs.h"
#include "ioda/Exception.h"
#include "ioda/Layouts/Layout_ObsGroup.h"

#include "ioda/config.h"

#if (eckit_FOUND && oops_FOUND)
#  define ENABLE_ODB_LAYOUT
#  include "ioda/Layouts/Layout_ObsGroup_ODB.h"
#endif

namespace ioda {
namespace detail {
std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(const std::string &polid) {
  if (polid == "ObsGroup" || polid == "ObsGroupODB")
    return std::make_shared<DataLayoutPolicy_ObsGroup>();
  return std::make_shared<DataLayoutPolicy>();
}

std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(Policies pol) {
  if (pol == Policies::ObsGroup || pol == Policies::ObsGroupODB) return std::make_shared<DataLayoutPolicy_ObsGroup>();
  return std::make_shared<DataLayoutPolicy>();
}

std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(const std::string &polid, const std::string &mapPath) {
  if (polid != "ObsGroupODB")
    throw Exception("A mapping file is only relevant for the ODB Data Layout.", ioda_Here());
#ifdef ENABLE_ODB_LAYOUT
  return std::make_shared<DataLayoutPolicy_ObsGroup_ODB>(mapPath);
#else
  throw Exception("Cannot generate this policy, as either eckit or oops are disabled.",
                  ioda_Here());
#endif
}

std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(Policies pol,
                                                                   const std::string &mapPath) {
  if (pol != Policies::ObsGroupODB)
    throw Exception("A mapping file is only relevant for the ODB Data Layout.", ioda_Here());
#ifdef ENABLE_ODB_LAYOUT
  return std::make_shared<DataLayoutPolicy_ObsGroup_ODB>(mapPath);
#else
  throw Exception("Cannot generate this policy, as either eckit or oops are disabled.",
                  ioda_Here());
#endif
}

DataLayoutPolicy::~DataLayoutPolicy(){}
DataLayoutPolicy::DataLayoutPolicy(){}
void DataLayoutPolicy::initializeStructure(Group_Base &) const {
  // Do nothing in the default policy.
}

std::string DataLayoutPolicy::name() const { return std::string{"None / no policy"}; }

std::string DataLayoutPolicy::doMap(const std::string &str) const { return str; }

bool DataLayoutPolicy::isComplementary(const std::string &str) const { return false; }

bool DataLayoutPolicy::isMapped(const std::string &) const { return false; }

size_t DataLayoutPolicy::getComplementaryPosition(const std::string &str) const {
  throw Exception("Illogical operation for non-ODB data layout policies.", ioda_Here());
}

std::string DataLayoutPolicy::getOutputNameFromComponent(const std::string &str) const {
  throw Exception("Illogical operation for non-ODB data layout policies.", ioda_Here());
}

std::type_index DataLayoutPolicy::getOutputVariableDataType(const std::string &str) const {
  throw Exception("Illogical operation for non-ODB data layout policies.", ioda_Here());
}

DataLayoutPolicy::MergeMethod DataLayoutPolicy::getMergeMethod(const std::string &str) const {
  throw Exception("Illogical operation for non-ODB data layout policies.", ioda_Here());
}

size_t DataLayoutPolicy::getInputsNeeded(const std::string &str) const {
  throw Exception("Illogical operation for non-ODB data layout policies.", ioda_Here());
}

std::pair<bool, std::string> DataLayoutPolicy::getUnit(const std::string &) const {
  throw Exception("Illogical operation for non-ODB data layout policies.", ioda_Here());
}

}  // namespace detail
}  // namespace ioda
