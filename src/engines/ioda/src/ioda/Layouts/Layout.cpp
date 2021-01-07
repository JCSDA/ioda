/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout.cpp
/// \brief Contains implementations for how data are arranged in ioda internally.

#include "ioda/Layout.h"

#include <vector>

#include "ioda/Layouts/Layout_ObsGroup.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(const std::string &polid) {
  if (polid == "ObsGroup") return std::make_shared<DataLayoutPolicy_ObsGroup>();
  return std::make_shared<DataLayoutPolicy>();
}

std::shared_ptr<const DataLayoutPolicy> DataLayoutPolicy::generate(Policies pol) {
  if (pol == Policies::ObsGroup) return std::make_shared<DataLayoutPolicy_ObsGroup>();
  return std::make_shared<DataLayoutPolicy>();
}

DataLayoutPolicy::~DataLayoutPolicy() = default;
DataLayoutPolicy::DataLayoutPolicy() = default;
void DataLayoutPolicy::initializeStructure(Group_Base &) const {
  // Do nothing in the default policy.
}

std::string DataLayoutPolicy::name() const { return std::string{"None / no policy"}; }

std::string DataLayoutPolicy::doMap(const std::string &str) const { return str; }
}  // namespace detail
}  // namespace ioda
