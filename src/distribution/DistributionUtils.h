/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_DISTRIBUTIONUTILS_H_
#define DISTRIBUTION_DISTRIBUTIONUTILS_H_

#include <memory>
#include <string>
#include <vector>

namespace eckit {
namespace mpi {
class Comm;
}
}

namespace util {
class DateTime;
}

namespace ioda {

class Distribution;

/// \brief Computes the dot product between two vectors of obs distributed across MPI ranks.
///
/// \param distribution
///   Distribution used to partition observations across MPI ranks.
/// \param numVariables
///   Number of variables whose observations are stored in `v1` and `v2`.
/// \param v1, v2
///   Vectors of observations. Observations of individual variables should be interleaved,
///   i.e. the observation of variable `ivar` at location `iloc` in the halo of the calling MPI
///   rank should be stored in element `(iloc * numVariables + ivar)` of each vector.
///
/// \return The dot product of the two vectors, with observations taken at locations belonging to
/// the halos of multiple MPI ranks counted only once and any missing values treated as if they
/// were zeros.
///
/// \relates Distribution
double dotProduct(const Distribution &dist, std::size_t numVariables,
                  const std::vector<double> &v1, const std::vector<double> &v2);
double dotProduct(const Distribution &dist, std::size_t numVariables,
                  const std::vector<float> &v1, const std::vector<float> &v2);
double dotProduct(const Distribution &dist, std::size_t numVariables,
                  const std::vector<int> &v1, const std::vector<int> &v2);
double dotProduct(const Distribution &dist, std::size_t numVariables,
                  const std::vector<int64_t> &v1, const std::vector<int64_t> &v2);

/// \brief Counts unique non-missing observations in a vector.
///
/// \param distribution
///   Distribution used to partition observations across MPI ranks.
/// \param numVariables
///   Number of variables whose observations are stored in `v`.
/// \param v
///   Vector of observations. Observations of individual variables should be interleaved,
///   i.e. the observation of variable `ivar` at location `iloc` in the halo of the calling MPI
///   rank should be stored at `v[iloc * numVariables + ivar]`.
///
/// \return The number of unique observations on all MPI ranks set to something else than the
/// missing value indicator. "Unique" means that observations taken at locations belonging to the
/// halos of multiple MPI ranks are counted only once.
///
/// \relates Distribution
std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   size_t numVariables, const std::vector<double> &v);
std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   size_t numVariables, const std::vector<float> &v);
std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   size_t numVariables, const std::vector<int> &v);
std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   size_t numVariables, const std::vector<std::string> &v);
std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   size_t numVariables, const std::vector<util::DateTime> &v);
std::size_t globalNumNonMissingObs(const Distribution &dist,
                                   size_t numVariables, const std::vector<bool> &v);

/// \brief Create a suitable replica distribution for the distribution `master`.
///
/// A replica distribution assigns each record `r` to a process if and only if another distribution
/// (the _master distribution_) has assigned the same record to that process.
///
/// \param comm
///   Communicator used by the master distribution.
/// \param master
///   Master distribution.
/// \param masterRecordNums
///   Records of all observations assigned by the master distribution to the calling process.
std::shared_ptr<Distribution> createReplicaDistribution(
    const eckit::mpi::Comm & comm,
    std::shared_ptr<const Distribution> master,
    const std::vector<std::size_t> &masterRecordNums);

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTIONUTILS_H_
