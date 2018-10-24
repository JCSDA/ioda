/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#ifndef IODA_RANDOM_F_H_
#define IODA_RANDOM_F_H_

namespace qg {
extern "C" {
  void random_f(const int &, double *);
}
}  // namespace qg

#endif  // IODA_RANDOM_F_H_
