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
  ASSERT(first.nvars() == second.nvars());
  ASSERT(first.nlocs() == second.nlocs());
  for (size_t i = 0; i < first.nvars(); ++i) {
    for (size_t j = 0; j < first.nlocs(); ++j) {
      if (first[i][j] != 0 && second[i][j] == 0) {
        same = false;
      }
      if (first[i][j] == 0 && second[i][j] != 0) {
        same = false;
      }
    }
  }
  oops::Log::trace() << "compareFlags done" << std::endl;
  return same;
}

// -----------------------------------------------------------------------------

size_t numZero(const ObsDataVector<int> & data) {
  oops::Log::trace() << "numZero starting" << std::endl;
  size_t nzero = 0;
  for (size_t i = 0; i < data.nvars(); ++i) {
    for (size_t j = 0; j < data.nlocs(); ++j) {
      if (data[i][j] == 0) nzero++;
    }
  }
  oops::Log::trace() << "numZero done" << std::endl;
  return nzero;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
