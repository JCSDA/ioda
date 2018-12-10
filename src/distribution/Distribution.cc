/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <iostream>

#include "distribution/Distribution.h"

namespace ioda {
// -----------------------------------------------------------------------------

Distribution::~Distribution() {}

// -----------------------------------------------------------------------------

void Distribution::erase(const std::size_t & index) {
  auto spos = std::find(indx_.begin(), indx_.end(), index);
  if (spos != indx_.end())
    indx_.erase(spos);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
