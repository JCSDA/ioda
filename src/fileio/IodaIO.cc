/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <string>

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/Logger.h"

#include "fileio/IodaIO.h"

namespace ioda {
// -----------------------------------------------------------------------------

IodaIO::IodaIO(): commMPI_(oops::mpi::comm()) { }

// -----------------------------------------------------------------------------

IodaIO::~IodaIO() { }

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the path to the file.
 */

std::string IodaIO::fname() const {
  return fname_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the mode (read, write, etc) for access to the file.
 */

std::string IodaIO::fmode() const {
  return fmode_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique locations in the obs data.
 */

std::size_t IodaIO::nlocs() {
  return nlocs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique observations in the obs data.
 */

std::size_t IodaIO::nobs() {
  return nobs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique recoreds in the obs data.
 *          A record is an atomic unit that will remain intact during distribution
 *          across multiple process elements. An example is a single sounding in
 *          radiosonde obs data.
 */

std::size_t IodaIO::nrecs() {
  return nrecs_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique variables in the obs data.
 */

std::size_t IodaIO::nvars() {
  return nvars_;
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the list of VALID variables in the obs data.
 */

std::vector<std::tuple<std::string, std::string>> * const IodaIO::varlist() {
  return &vname_group_;
}

}  // namespace ioda
