/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_DISTRIBUTION_H_
#define IODA_DISTRIBUTION_H_

#include <vector>

#include "eckit/mpi/Comm.h"
#include "oops/util/Logger.h"

namespace ioda {

// ---------------------------------------------------------------------

class Distribution{
 public:
     Distribution() {}
     ~Distribution() {}
     void random_distribution(const eckit::mpi::Comm & comm, const int & gnlocs);
     const std::vector<int> & distribution() const {return indx_;}
     void erase(const std::size_t &);
     const std::size_t size() const {return indx_.size();}
 private:
     int nproc_;
     int myproc_;
     std::vector<int> indx_;
};

}  // namespace ioda

#endif  // IODA_DISTRIBUTION_H_
