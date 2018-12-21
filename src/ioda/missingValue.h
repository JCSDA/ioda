/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_MISSINGVALUE_H_
#define IODA_MISSINGVALUE_H_

#include <limits>

namespace ioda {

// Use missing value that is unlikely to be a useful value and not a remarquable
// number that could be used somewhere else (ie not exactly the max value)

template<typename DATATYPE> static DATATYPE missingValue() {
  return std::numeric_limits<DATATYPE>::max() - static_cast<DATATYPE>(1);
}

}  // namespace ioda

#endif  // IODA_MISSINGVALUE_H_
