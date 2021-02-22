#pragma once
/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout_Utils.h
/// \brief Contains definitions for Layout Utilities.

#include <string>

#include "Layout_ObsGroup.h"
#include "Layout_ObsGroup_ODB.h"

namespace ioda {
namespace detail {
/// Methods used by at least two Data Layout Policies

/// \brief Switch "@" for "/" and switch the group/variable order
std::string reverseStringSwapDelimiter(const std::string &str);

}  // namespace detail
}  // namespace ioda
