/*
 * (C) Crown copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <iostream>
#include <unordered_set>

#include <boost/make_unique.hpp>

#include "atlas/mesh.h"
#include "atlas/meshgenerator.h"
#include "atlas/util/PolygonLocator.h"
#include "atlas/util/PolygonXY.h"

#include "eckit/mpi/Comm.h"
#include "ioda/distribution/AtlasDistribution.h"
#include "ioda/distribution/DistributionFactory.h"
#include "oops/util/Logger.h"

namespace ioda {

namespace  {
const char DIST_NAME[] = "Atlas";
}  // namespace

// -----------------------------------------------------------------------------

/// \brief Assigns records to MPI ranks for the AtlasDistribution.
class AtlasDistribution::RecordAssigner {
 public:
  /// Constructs an Atlas grid and mesh using settings loaded from the `grid` section
  /// of `config`; then partitions the mesh across processes making up the `atlas::mpi::Comm()`
  /// communicator.
  explicit RecordAssigner(const eckit::Configuration & config);

  /// If this record hasn't been assigned to any process yet, assigns it to the process
  /// owning the partition containing `point`.
  ///
  /// It is assumed that records will be assigned in consecutive order.
  void assignRecord(std::size_t recNum, const eckit::geometry::Point2 & point);

  /// Returns true if record `recNum` has been assigned to the calling process, false otherwise.
  bool isMyRecord(std::size_t recNum) const;

 private:
  bool isInMyDomain(const eckit::geometry::Point2 & point) const;

 private:
  atlas::Mesh mesh_;
  std::unique_ptr<atlas::util::PolygonLocator> locator_;

  std::unordered_set<std::size_t> myRecords_;
  std::size_t nextRecordToAssign_ = 0;
};

AtlasDistribution::RecordAssigner::RecordAssigner(const eckit::Configuration & config)
{
  eckit::LocalConfiguration gridConfig(config, "grid");
  atlas::util::Config atlasConfig(gridConfig);

  atlas::Grid grid(atlasConfig);

  atlasConfig.set("type", grid.meshgenerator().getString("type"));
  atlas::MeshGenerator generator(atlasConfig);

  mesh_ = generator.generate(grid);
  if (mesh_->nb_partitions() != atlas::mpi::comm().size()) {
    std::stringstream msg;
    msg << "The number of mesh partitions, " << mesh_->nb_partitions()
        << ", is different from the number of MPI processes, " << atlas::mpi::comm().size();
    throw eckit::Exception(msg.str(), Here());
  }

  locator_ = boost::make_unique<atlas::util::PolygonLocator>(
        atlas::util::ListPolygonXY(mesh_.polygons()), mesh_.projection());
}

void AtlasDistribution::RecordAssigner::assignRecord(std::size_t recNum,
                                                     const eckit::geometry::Point2 & point) {
  if (recNum == nextRecordToAssign_) {
    const bool myRecord = isInMyDomain(point);
    oops::Log::debug() << "RecordAssigner::assignRecord(): is " << recNum << " my record? "
                       << myRecord << std::endl;
    if (myRecord)
      myRecords_.insert(recNum);
    ++nextRecordToAssign_;
  } else {
    // We assume records will be assigned in consecutive order
    ASSERT(recNum < nextRecordToAssign_);
  }
}

bool AtlasDistribution::RecordAssigner::isMyRecord(std::size_t recNum) const {
  return myRecords_.find(recNum) != myRecords_.end();
}

bool AtlasDistribution::RecordAssigner::isInMyDomain(const eckit::geometry::Point2 & point) const {
  const atlas::idx_t partition = (*locator_)(point);
  oops::Log::debug() << "RecordAssigner::isInMyDomain(): Polygon locator says "
                     << point << " is in domain " << partition << std::endl;
  return partition == atlas::mpi::comm().rank();
}

// -----------------------------------------------------------------------------

static DistributionMaker<AtlasDistribution> maker(DIST_NAME);

AtlasDistribution::AtlasDistribution(const eckit::mpi::Comm & comm,
                                   const eckit::Configuration & config)
  : NonoverlappingDistribution(comm),
    recordAssigner_(boost::make_unique<RecordAssigner>(config))
{
  oops::Log::trace() << "AtlasDistribution constructed" << std::endl;
}

AtlasDistribution::~AtlasDistribution() {
  oops::Log::trace() << "AtlasDistribution destructed" << std::endl;
}

void AtlasDistribution::assignRecord(const std::size_t recNum,
                                    const std::size_t locNum,
                                    const eckit::geometry::Point2 & point) {
  recordAssigner_->assignRecord(recNum, point);
  NonoverlappingDistribution::assignRecord(recNum, locNum, point);
}

bool AtlasDistribution::isMyRecord(std::size_t RecNum) const {
  return recordAssigner_->isMyRecord(RecNum);
}

std::string AtlasDistribution::name() const {
  return DIST_NAME;
}
// -----------------------------------------------------------------------------

}  // namespace ioda
