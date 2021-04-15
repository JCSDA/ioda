/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_OBSLOCGC99_H_
#define CORE_OBSLOCGC99_H_

#include <ostream>
#include <vector>

#include "atlas/util/Earth.h"

#include "eckit/config/Configuration.h"
#include "eckit/container/KDTree.h"
#include "eckit/geometry/Point2.h"
#include "eckit/geometry/Point3.h"
#include "eckit/geometry/UnitSphere.h"

#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"
#include "ioda/core/ObsLocParameters.h"

#include "oops/generic/gc99.h"
#include "oops/util/Printable.h"

// Forward declarations
namespace ioda {

/// Horizontal Gaspari-Cohn observation space localization
template<class MODEL>
class ObsLocGC99: public util::Printable {
  typedef typename MODEL::GeometryIterator   GeometryIterator_;

 public:
  struct TreeTrait {
    typedef eckit::geometry::Point3 Point;
    typedef double                  Payload;
  };
  typedef eckit::KDTreeMemory<TreeTrait> KDTree;
  ObsLocGC99(const eckit::Configuration &, const ObsSpace &);

  /// compute localization and save localization values in \p obsvector and
  /// localization flags (1: outside of localization; 0: inside localization area)
  /// in \p outside
  void computeLocalization(const GeometryIterator_ &, ObsDataVector<int> & outside,
                           ObsVector & obsvector) const;

 private:
  void print(std::ostream &) const;

  ObsLocParameters options_;

  /// KD-tree for searching for local obs
  std::unique_ptr<KDTree> kd_;

  std::vector<float> lats_;
  std::vector<float> lons_;
};

// -----------------------------------------------------------------------------

/*!
 * \details Creates a KDTree class member that can be used for searching for local obs
 */
template<typename MODEL>
ObsLocGC99<MODEL>::ObsLocGC99(const eckit::Configuration & config, const ObsSpace & obsspace)
  : lats_(obsspace.nlocs()), lons_(obsspace.nlocs())
{
  options_.deserialize(config);

  // check that this distribution supports local obs space
  std::string distName = obsspace.distribution().name();
  if ( distName != "Halo" && distName != "InefficientDistribution" ) {
    std::string message = "Can not use local ObsSpace with distribution=" + distName;
    throw eckit::BadParameter(message);
  }

  const size_t nlocs = obsspace.nlocs();
  // Get latitudes and longitudes of all observations.
  obsspace.get_db("MetaData", "longitude", lons_);
  obsspace.get_db("MetaData", "latitude", lats_);

  if (options_.searchMethod == SearchMethod::KDTREE) {
    kd_ = std::unique_ptr<KDTree> ( new KDTree() );
    // Define points list from lat/lon values
    typedef typename KDTree::PointType Point;
    std::vector<typename KDTree::Value> points;
    for (unsigned int i = 0; i < nlocs; i++) {
      eckit::geometry::Point2 lonlat(lons_[i], lats_[i]);
      Point xyz = Point();
      // FIXME: get geometry from yaml, for now assume spherical Earth radius.
      atlas::util::Earth::convertSphericalToCartesian(lonlat, xyz);
      double index = static_cast<double>(i);
      typename KDTree::Value v(xyz, index);
      points.push_back(v);
    }
    // Create KDTree class member from points list.
    kd_->build(points.begin(), points.end());
  }
}

// -----------------------------------------------------------------------------

template<typename MODEL>
void ObsLocGC99<MODEL>::computeLocalization(const GeometryIterator_ & i,
                                            ObsDataVector<int> & outside,
                                            ObsVector & locvector) const {
  oops::Log::trace() << "ioda::ObsSpace for LocalObs starting" << std::endl;

  eckit::geometry::Point2 refPoint = *i;
  std::vector<int> localobs;
  std::vector<double> obsdist;
  size_t nlocs = lons_.size();
  if ( options_.searchMethod == SearchMethod::BRUTEFORCE ) {
    oops::Log::trace() << "ioda::ObsSpace searching via brute force." << std::endl;

    for (unsigned int jj = 0; jj < nlocs; ++jj) {
      eckit::geometry::Point2 searchPoint(lons_[jj], lats_[jj]);
      double localDist = options_.distance(refPoint, searchPoint);
      if ( localDist < options_.lengthscale ) {
        localobs.push_back(jj);
        obsdist.push_back(localDist);
      }
    }
    const boost::optional<int> & maxnobs = options_.maxnobs;
    if ( (maxnobs != boost::none) && (localobs.size() > *maxnobs ) ) {
      for (unsigned int jj = 0; jj < localobs.size(); ++jj) {
          oops::Log::debug() << "Before sort [i, d]: " << localobs[jj]
              << " , " << obsdist[jj] << std::endl;
      }
      // Construct a temporary paired vector to do the sorting
      std::vector<std::pair<std::size_t, double>> localObsIndDistPair;
      for (unsigned int jj = 0; jj < obsdist.size(); ++jj) {
        localObsIndDistPair.push_back(std::make_pair(localobs[jj], obsdist[jj]));
      }

      // Use a lambda function to implement an ascending sort.
      sort(localObsIndDistPair.begin(), localObsIndDistPair.end(),
           [](const std::pair<std::size_t, double> & p1,
              const std::pair<std::size_t, double> & p2){
                return(p1.second < p2.second);
              });

      // Unpair the sorted pair vector
      for (unsigned int jj = 0; jj < obsdist.size(); ++jj) {
        localobs[jj] = localObsIndDistPair[jj].first;
        obsdist[jj] = localObsIndDistPair[jj].second;
      }

      // Truncate to maxNobs length
      localobs.resize(*maxnobs);
      obsdist.resize(*maxnobs);
    }
  } else if (nlocs > 0) {
    // Check (nlocs > 0) is needed,
    // otherwise, it will cause ASERT check fail in kdtree.findInSphere, and hang.

    oops::Log::trace() << "ioda::ObsSpace searching via KDTree" << std::endl;

    if ( options_.distanceType == DistanceType::CARTESIAN)
      ABORT("ObsSpace:: search method must be 'brute_force' when using 'cartesian' distance");

    // Using the radius of the earth
    eckit::geometry::Point3 refPoint3D;
    atlas::util::Earth::convertSphericalToCartesian(refPoint, refPoint3D);
    double alpha =  (options_.lengthscale / options_.radius_earth)/ 2.0;  // angle in radians
    double chordLength = 2.0*options_.radius_earth * sin(alpha);  // search radius in 3D space

    auto closePoints = kd_->findInSphere(refPoint3D, chordLength);

    // put closePoints back into localobs and obsdist
    for (unsigned int jloc = 0; jloc < closePoints.size(); ++jloc) {
       localobs.push_back(closePoints[jloc].payload());  // observation
       obsdist.push_back(closePoints[jloc].distance());  // distance
    }

    // The obs are sorted in the kdtree call
    const boost::optional<int> & maxnobs = options_.maxnobs;
    if ( (maxnobs != boost::none) && (localobs.size() > *maxnobs ) ) {
      // Truncate to maxNobs length
      localobs.resize(*maxnobs);
      obsdist.resize(*maxnobs);
    }
  }
  for (size_t jloc = 0; jloc < outside.nlocs(); ++jloc) {
    for (size_t jvar = 0; jvar < outside.nvars(); ++jvar) {
      outside[jvar][jloc] = 1;
    }
  }
  const size_t nvars = locvector.nvars();
  for (size_t jlocal = 0; jlocal < localobs.size(); ++jlocal) {
    double gc = oops::gc99(obsdist[jlocal] / options_.lengthscale);
    // obsdist is calculated at each location; need to update R for each variable
    for (size_t jvar = 0; jvar < nvars; ++jvar) {
      outside[jvar][localobs[jlocal]] = 0;
      locvector[jvar + localobs[jlocal] * nvars] = gc;
    }
  }
}

// -----------------------------------------------------------------------------

template<typename MODEL>
void ObsLocGC99<MODEL>::print(std::ostream & os) const {
  os << "Gaspari-Cohn horizontal localization with " << options_.lengthscale
     << " lengthscale" << std::endl;
}

}  // namespace ioda

#endif  // CORE_OBSLOCGC99_H_
