/*
 * (C) Copyright 2018-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/ObsDataVector.h"

#include <vector>

namespace ioda {

// -----------------------------------------------------------------------------
bool compareFlags(const ObsDataVector<int> & first, const ObsDataVector<int> & second) {
  oops::Log::trace() << "compareFlags starting" << std::endl;
  bool same = true;
  ASSERT(first.size() == second.size());
  for (std::size_t i = 0; i < first.size(); ++i) {
    if (first[i] != 0 && second[i] == 0) {
      same = false;
    }
    if (first[i] == 0 && second[i] != 0) {
      same = false;
    }
  }
  oops::Log::trace() << "compareFlags done" << std::endl;
  return same;
}

// -----------------------------------------------------------------------------
size_t numZero(const ObsDataVector<int> & data) {
  oops::Log::trace() << "numZero starting" << std::endl;
  size_t nzero = 0;
  for (std::size_t i = 0; i < data.size(); ++i) {
    if (data[i] == 0) nzero++;
  }
  oops::Log::trace() << "numZero done" << std::endl;
  return nzero;
}

}  // namespace ioda
