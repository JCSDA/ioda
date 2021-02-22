/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout.cpp
/// \brief Contains implementations for how data are arranged in ioda internally.

#include "ioda/Layout.h"

#include <exception>
#include <vector>

#include "eckit/config/Configuration.h"

#include "ioda/Layouts/Layout_ObsGroup.h"
#include "ioda/Layouts/Layout_ObsGroup_ODB.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(const std::string &polid) {
  if (polid == "ObsGroup" || polid == "ObsGroupODB") return std::make_shared<DataLayoutPolicy_ObsGroup>();
  return std::make_shared<DataLayoutPolicy>();
}

std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(
    const std::string &polid, const std::string &mapPath) {
  if (polid != "ObsGroupODB")
    throw std::invalid_argument("A mapping file is only relevant for the ODB Data Layout.");
  return std::make_shared<DataLayoutPolicy_ObsGroup_ODB>(mapPath);
}

std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(Policies pol) {
  if (pol == Policies::ObsGroup || pol == Policies::ObsGroupODB) return std::make_shared<DataLayoutPolicy_ObsGroup>();
  return std::make_shared<DataLayoutPolicy>();
}

std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(
    Policies pol, const std::string &mapPath) {
  if (pol != Policies::ObsGroupODB)
    throw std::invalid_argument("A mapping file is only relevant for the ODB Data Layout.");
  return std::make_shared<DataLayoutPolicy_ObsGroup_ODB>(mapPath);
}

DataLayoutPolicy::~DataLayoutPolicy(){}
DataLayoutPolicy::DataLayoutPolicy(){}
void DataLayoutPolicy::initializeStructure(Group_Base &) const {
  // Do nothing in the default policy.
}

std::string DataLayoutPolicy::name() const { return std::string{"None / no policy"}; }

std::string DataLayoutPolicy::doMap(const std::string &str) const { return str; }
}  // namespace detail
}  // namespace ioda
