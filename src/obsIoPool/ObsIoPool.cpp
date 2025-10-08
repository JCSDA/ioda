/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/obsIoPool/ObsIoPool.hpp"

#include "oops/util/Logger.h"

namespace ioda {
namespace ObsIoPool {

constexpr int poolColor = 1;
constexpr int nonPoolColor = 0;
static const char poolCommName[] = "ObsIoPool";
static const char nonPoolCommName[] = "NonObsIoPool";

//--------------------------------------------------------------------------------------
ObsIoPool::ObsIoPool(
               const ioda::IoPool::IoPoolParameters & configParams,
               const eckit::mpi::Comm & commAll)
    : configParams_(configParams), commAll_(commAll) {
  // Construct the io pool communicator group by splitting the commAll_ communicator group
  // into two groups, one for the i/o pool and one for all the other ranks not in the pool.
  // Take into consideration the size of the commAll_ group and the maximum pool size spec.
  const int rankAll = commAll_.rank();
  const int sizeAll = commAll_.size();

  // Have rank 0 determine the color values (for the split comm function) for all the ranks
  // and scatter those values to all ranks. Then call the split comm function.
  std::vector<int> colorValues(sizeAll, nonPoolColor);
  int myColor;
  if (rankAll == 0) {
    // Determine the target pool size. The io pool parameter, maxPoolSize, is
    // where the default maximum pool size (currently 4) is set. The target pool
    // size will be the minimum of the maximum pool size spec and the number of
    // available mpi tasks.
    int targetPoolSize = configParams_.maxPoolSize.value();
    if (sizeAll < targetPoolSize) {
      targetPoolSize = sizeAll;
    }

    // Determine the color values for all ranks. It works out nicely to spread out the
    // pool ranks evenly across the full commAll_ group.
    int poolRank = 0;
    const int poolStride = sizeAll / targetPoolSize;
    const int remainder = sizeAll % targetPoolSize;
    for (int i = 0; i < targetPoolSize; ++i) {
      colorValues[poolRank] = poolColor;
      poolRank += poolStride;
      if (i < remainder) {
        poolRank++;
      }
    }
    commAll_.scatter(colorValues, myColor, 0);
  } else {
    // Other ranks just receive their color value
    commAll_.scatter(colorValues, myColor, 0);
  }

  // Create the io pool communicator groups
  if (myColor == poolColor) {
    inIoPool_ = true;
    commPool_ = &(commAll_.split(poolColor, poolCommName));
  } else {
    inIoPool_ = false;
    commPool_ = &(commAll_.split(nonPoolColor, nonPoolCommName));
  }
}

ObsIoPool::~ObsIoPool() {
  // At this point there are two split communicator groups: one for the io pool and the
  // other for the processes not included in the io pool.
  if (eckit::mpi::hasComm(poolCommName)) {
      eckit::mpi::deleteComm(poolCommName);
  }
  if (eckit::mpi::hasComm(nonPoolCommName)) {
      eckit::mpi::deleteComm(nonPoolCommName);
  }
}

//--------------------------------------------------------------------------------------
void ObsIoPool::print(std::ostream & os) const {
  // Dump out the pool status
  if (inIoPool_) {
    os << "(rankAll = " << commAll_.rank()
       << ", in pool, pool size, pool rank = " << commPool_->size()
       << ", " << commPool_->rank() << ")";
  } else {
    os << "(rankAll = " << commAll_.rank() << ", not in pool)";
  }
}

}  // namespace ObsIoPool
}  // namespace ioda
