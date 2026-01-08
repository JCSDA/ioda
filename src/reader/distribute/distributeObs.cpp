/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/distribute/distributeObs.hpp"

#include <numeric>

#include "eckit/mpi/Comm.h"

#include "ioda/core/ObsSourceStats.h"
#include "ioda/containers/IFrame.h"
#include "ioda/distribution/DistributionFactory.h"

#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace reader {

//---------------------------------------------------------------------
void distributeObs(const DistributionParametersBase & distParams,
                   const eckit::mpi::Comm & commAll,
                   ObsSourceStats & obsSourceStats,
                   std::shared_ptr<Distribution> & ospaceDist,
                   std::unique_ptr<osdf::IFrame> & osdfCont) {
  // todo(SRH): For now simply create the distribution object, and leave
  // the obs on the ranks as is from the load step. Eventually, we want
  // to fill in code here to move the obs according to the given distribution.
  // Until this is done, the ReaderDependentDistribution is the only
  // supported distribution type. Upstream code has already checked for
  // this and if we get to here we are good to go.
  ospaceDist = DistributionFactory::create(commAll, distParams);

  // todo(SRH): For now, set the number of locations in the obspaceDist
  // (which is ReaderDependentDistribution), to the number of rows
  // in the osdfCont. This will allow the allGather.v functions in the
  // distribution to operate correctly.
  ospaceDist->setNumberLocations(osdfCont->numRows());

  // todo(SRH): For now, update the ObsSourceStats assuming that there
  // was no obs grouping, and using the non-overlapping
  // ReaderDependentDistribution. This boils down to making the records
  // line up one-for-one with the location indices. However, the record
  // nubmers need to be assigned in a global consecutive manner.
  obsSourceStats.nrecs = obsSourceStats.nlocs;
  std::size_t startRecNum = obsSourceStats.nrecs;
  oops::mpi::exclusiveScan(commAll, startRecNum);
  obsSourceStats.recNums.resize(obsSourceStats.locIndices.size());
  std::iota(obsSourceStats.recNums.begin(), obsSourceStats.recNums.end(), startRecNum);
}

}  // namespace reader
}  // namespace ioda
