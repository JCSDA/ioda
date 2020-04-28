/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_LOCALOBSSPACEPARAMETERS_H_
#define IODA_LOCALOBSSPACEPARAMETERS_H_

#include <string>

#include "eckit/exception/Exceptions.h"
#include "eckit/geometry/KPoint.h"
#include "eckit/geometry/UnitSphere.h"

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace eckit {
  class Configuration;
}

namespace ioda {

enum class DistanceType {
  GEODESIC, CARTESIAN
};

enum class SearchMethod {
  BRUTEFORCE, KDTREE
};

}  // namespace ioda

namespace oops {

/// Extraction of DistanceType parameter from config
template <>
struct ParameterTraits<ioda::DistanceType> {
  static boost::optional<ioda::DistanceType> get(const eckit::Configuration &config,
                                                const std::string& name) {
    std::string value;
    if (config.get(name, value)) {
      if (value == "geodesic")
        return ioda::DistanceType::GEODESIC;
      if (value == "cartesian")
        return ioda::DistanceType::CARTESIAN;
      throw eckit::BadParameter("Bad conversion from std::string '" + value + "' to DistanceType",
                                Here());
    } else {
      return boost::none;
    }
  }
};

/// Extraction of SearchMethod parameter from config
template <>
struct ParameterTraits<ioda::SearchMethod> {
  static boost::optional<ioda::SearchMethod> get(const eckit::Configuration &config,
                                                 const std::string& name) {
    std::string value;
    if (config.get(name, value)) {
      if (value == "brute_force")
        return ioda::SearchMethod::BRUTEFORCE;
      if (value == "kd_tree")
        return ioda::SearchMethod::KDTREE;
      throw eckit::BadParameter("Bad conversion from std::string '" + value + "' to SearchMethod",
                                Here());
    } else {
      return boost::none;
    }
  }
};

}  // namespace oops

namespace ioda {

/// \brief Options controlling local observations subsetting
class LocalObsSpaceParameters : public oops::Parameters {
 public:
  /// Localization lengthscale (find all obs within the distance from reference point)
  oops::RequiredParameter<double> lengthscale{"lengthscale", this};

  /// Method for searching for nearest points: brute force or KD-tree
  /// Default: brute force (the only one implemented now)
  oops::Parameter<SearchMethod> searchMethod{"search_method", SearchMethod::BRUTEFORCE, this};

  /// Maximum number of obs
  oops::OptionalParameter<int> maxnobs{"max_nobs", this};

  /// Distance calculation type: geodesic (on sphere) or cartesian (euclidian)
  /// Default: geodesic
  oops::Parameter<DistanceType> distanceType{"distance_type", DistanceType::GEODESIC, this};

  /// returns distance between points \p p1 and \p p2, depending on the
  /// distance calculation type distanceType
  double distance(const eckit::geometry::Point2 & p1, const eckit::geometry::Point2 & p2) {
    if (distanceType == DistanceType::GEODESIC) {
      return eckit::geometry::Sphere::distance(radiusEarth_, p1, p2);
    } else {
      ASSERT(distanceType == DistanceType::CARTESIAN);
      return p1.distance(p2);
    }
  }

 private:
  const double radiusEarth_ = 6.371e6;  // Earth radius in km
};

}  // namespace ioda

#endif  // IODA_LOCALOBSSPACEPARAMETERS_H_
