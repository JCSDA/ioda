/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "distribution/InefficientDistribution.h"

#include <algorithm>
#include <iostream>
#include <numeric>

#include "oops/util/Logger.h"


namespace ioda {
// -----------------------------------------------------------------------------
InefficientDistribution::InefficientDistribution(const eckit::mpi::Comm & Comm) :
                                             Distribution(Comm) {
  oops::Log::trace() << "InefficientDistribution constructed" << std::endl;
}

// -----------------------------------------------------------------------------
InefficientDistribution::~InefficientDistribution() {
  oops::Log::trace() << "InefficientDistribution destructed" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Inefficient distribution
 *
 * \details This method distributes all observations to all processes.
 *
 * \param[in] RecNum Record number, checked if belongs on this process element
 */
bool InefficientDistribution::isMyRecord(std::size_t RecNum) const {
  return true;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
