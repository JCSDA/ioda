/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/distribution/RoundRobin.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <set>

#include "ioda/distribution/DistributionFactory.h"
#include "oops/mpi/mpi.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

namespace ioda {

namespace {

// Helper functions used by the implementation of allGatherv

/// \brief Join strings into a single character array before MPI transfer.
///
/// \param strings
///   Strings to join.
///
/// \returns A pair of two vectors. The first is a concatenation of all input strings
/// (without any separating null characters). The second is the list of lengths of these strings.
std::pair<std::vector<char>, std::vector<size_t>> encodeStrings(
        const std::vector<std::string> &strings) {
    std::pair<std::vector<char>, std::vector<size_t>> result;
    std::vector<char> &charArray = result.first;
    std::vector<size_t> &lengths = result.second;

    size_t totalLength = 0;
    lengths.reserve(strings.size());
    for (const std::string &s : strings) {
        lengths.push_back(s.size());
        totalLength += s.size();
    }

    charArray.reserve(totalLength);
    for (const std::string &s : strings) {
        charArray.insert(charArray.end(), s.begin(), s.end());
    }

    return result;
}

/// \brief Split a character array into multiple strings.
///
/// \param charArray
///   A character array storing a number of concatenated strings (without separating null
///   characters).
///
/// \param lengths
///  The list of lengths of the strings stored in \p charArray.
///
/// \returns A vector of strings extracted from \p charArray.
std::vector<std::string> decodeStrings(const std::vector<char> &charArray,
                                       const std::vector<size_t> &lengths) {
    std::vector<std::string> strings;
    strings.reserve(lengths.size());

    std::vector<char>::const_iterator nextStringBegin = charArray.begin();
    for (size_t length : lengths) {
        strings.emplace_back(nextStringBegin, nextStringBegin + length);
        nextStringBegin += length;
    }

    return strings;
}

// Implementation of allGatherv

/// Generic implementation (suitable for "plain old data")
template <typename T>
void allGathervImpl(const eckit::mpi::Comm & comm, std::vector<T> &x) {
    eckit::mpi::Buffer<T> buffer(comm.size());
    comm.allGatherv(x.begin(), x.end(), buffer);
    x = std::move(buffer.buffer);
}

/// Specialisation for util::DateTime
template <>
void allGathervImpl(const eckit::mpi::Comm & comm, std::vector<util::DateTime> &x) {
    size_t globalSize = x.size();
    comm.allReduceInPlace(globalSize, eckit::mpi::sum());
    std::vector<util::DateTime> globalX(globalSize);
    oops::mpi::allGathervUsingSerialize(comm, x.begin(), x.end(), globalX.begin());
    x = std::move(globalX);
}

/// Specialisation for std::string
template <>
void allGathervImpl(const eckit::mpi::Comm & comm, std::vector<std::string> &x) {
    std::pair<std::vector<char>, std::vector<size_t>> encodedX = encodeStrings(x);

    // Gather all character arrays
    eckit::mpi::Buffer<char> charBuffer(comm.size());
    comm.allGatherv(encodedX.first.begin(), encodedX.first.end(), charBuffer);

    // Gather all string lengths
    eckit::mpi::Buffer<size_t> lengthBuffer(comm.size());
    comm.allGatherv(encodedX.second.begin(), encodedX.second.end(), lengthBuffer);

    // Free memory
    encodedX = {};

    x = decodeStrings(charBuffer.buffer, lengthBuffer.buffer);
}

}  // namespace

// -----------------------------------------------------------------------------
static DistributionMaker<RoundRobin> maker("RoundRobin");

// -----------------------------------------------------------------------------
RoundRobin::RoundRobin(const eckit::mpi::Comm & Comm,
                       const eckit::Configuration & config)
                       : Distribution(Comm, config) {
  oops::Log::trace() << "RoundRobin constructed" << std::endl;
}

// -----------------------------------------------------------------------------
RoundRobin::~RoundRobin() {
  oops::Log::trace() << "RoundRobin destructed" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief Round-robin selector
 *
 * \details This method distributes observations according to a round-robin scheme.
 *          The round-robin scheme simply selects all locations where the modulus of
 *          the record number relative to the number of process elements equals
 *          the rank of the process element we are running on. This does a good job
 *          of distributing the observations evenly across processors which optimizes
 *          the load balancing.
 *
 * \param[in] RecNum Record number, checked if belongs on this process element
 */
bool RoundRobin::isMyRecord(std::size_t RecNum) const {
    return (RecNum % comm_.size() == comm_.rank());
}

// -----------------------------------------------------------------------------
void RoundRobin::patchObs(std::vector<bool> & patchObsVec) const {
  std::fill(patchObsVec.begin(), patchObsVec.end(), true);
}

// -----------------------------------------------------------------------------
double RoundRobin::dot_product(
                const std::vector<double> &v1, const std::vector<double> &v2) const {
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
double RoundRobin::dot_product(
                const std::vector<float> &v1, const std::vector<float> &v2) const {
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
double RoundRobin::dot_product(
                const std::vector<int> &v1, const std::vector<int> &v2) const {
  return dot_productImpl(v1, v2);
}

// -----------------------------------------------------------------------------
template <typename T>
double RoundRobin::dot_productImpl(
                   const std::vector<T> &v1, const std::vector<T> &v2) const {
  ASSERT(v1.size() == v2.size());
  T missingValue = util::missingValue(missingValue);

  double zz = 0.0;
  for (size_t jj = 0; jj < v1.size() ; ++jj) {
    if (v1[jj] != missingValue && v2[jj] != missingValue) {
      zz += v1[jj] * v2[jj];
    }
  }

  this->sum(zz);
  return zz;
}

// -----------------------------------------------------------------------------
size_t RoundRobin::globalNumNonMissingObs(const std::vector<double> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t RoundRobin::globalNumNonMissingObs(const std::vector<float> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t RoundRobin::globalNumNonMissingObs(const std::vector<int> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t RoundRobin::globalNumNonMissingObs(const std::vector<std::string> &v) const {
  return globalNumNonMissingObsImpl(v);
}

size_t RoundRobin::globalNumNonMissingObs(const std::vector<util::DateTime> &v) const {
  return globalNumNonMissingObsImpl(v);
}

template <typename T>
size_t RoundRobin::globalNumNonMissingObsImpl(const std::vector<T> &v) const {
  T missingValue = util::missingValue(missingValue);

  size_t nobs = 0;
  for (size_t jj = 0; jj < v.size() ; ++jj) {
    if (v[jj] != missingValue) ++nobs;
  }

  this->sum(nobs);
  return nobs;
}

// -----------------------------------------------------------------------------
void RoundRobin::sum(double &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::sum());
}

void RoundRobin::sum(float &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::sum());
}

void RoundRobin::sum(int &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::sum());
}

void RoundRobin::sum(size_t &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::sum());
}

void RoundRobin::sum(std::vector<double> &x) const {
    comm_.allReduceInPlace(x.begin(), x.end(), eckit::mpi::sum());
}

void RoundRobin::sum(std::vector<size_t> &x) const {
    comm_.allReduceInPlace(x.begin(), x.end(), eckit::mpi::sum());
}

void RoundRobin::min(double &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::min());
}

void RoundRobin::min(float &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::min());
}

void RoundRobin::min(int &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::min());
}

void RoundRobin::max(double &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::max());
}

void RoundRobin::max(float &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::max());
}

void RoundRobin::max(int &x) const {
    comm_.allReduceInPlace(x, eckit::mpi::max());
}

void RoundRobin::allGatherv(std::vector<size_t> &x) const {
    allGathervImpl(comm_, x);
}

void RoundRobin::allGatherv(std::vector<int> &x) const {
    allGathervImpl(comm_, x);
}

void RoundRobin::allGatherv(std::vector<float> &x) const {
    allGathervImpl(comm_, x);
}

void RoundRobin::allGatherv(std::vector<double> &x) const {
    allGathervImpl(comm_, x);
}

void RoundRobin::allGatherv(std::vector<util::DateTime> &x) const {
    allGathervImpl(comm_, x);
}

void RoundRobin::allGatherv(std::vector<std::string> &x) const {
    allGathervImpl(comm_, x);
}

void RoundRobin::exclusiveScan(size_t &x) const {
    // Could be done with MPI_Exscan, but there's no wrapper for it in eckit::mpi.

    std::vector<size_t> xs(comm_.size());
    comm_.allGather(x, xs.begin(), xs.end());
    x = std::accumulate(xs.begin(), xs.begin() + comm_.rank(), 0);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
