/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_HALO_H_
#define DISTRIBUTION_HALO_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "eckit/geometry/Sphere.h"
#include "eckit/mpi/Comm.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Halo distribution
 *
 * \details All obs. are divided into compact overlaping sets assigned to each PE 
 *
 * \author Sergey Frolov (CIRES)
 */
class Halo: public Distribution {
 public:
     explicit Halo(const eckit::mpi::Comm & Comm,
                   const eckit::Configuration & config);
     ~Halo();
     void assignRecord(const std::size_t RecNum, const std::size_t LocNum,
                      const eckit::geometry::Point2 & point) override;
     bool isMyRecord(std::size_t RecNum) const override;
     void computePatchLocs(const std::size_t nglocs) override;
     void patchObs(std::vector<bool> &) const override;

     void min(int & x) const override;
     void min(std::size_t & x) const override;
     void min(float & x) const override;
     void min(double & x) const override;
     void min(std::vector<int> & x) const override;
     void min(std::vector<std::size_t> & x) const override;
     void min(std::vector<float> & x) const override;
     void min(std::vector<double> & x) const override;

     void max(int & x) const override;
     void max(std::size_t & x) const override;
     void max(float & x) const override;
     void max(double & x) const override;
     void max(std::vector<int> & x) const override;
     void max(std::vector<std::size_t> & x) const override;
     void max(std::vector<float> & x) const override;
     void max(std::vector<double> & x) const override;

     void allGatherv(std::vector<size_t> &x) const override;
     void allGatherv(std::vector<int> &x) const override;
     void allGatherv(std::vector<float> &x) const override;
     void allGatherv(std::vector<double> &x) const override;
     void allGatherv(std::vector<util::DateTime> &x) const override;
     void allGatherv(std::vector<std::string> &x) const override;

     size_t globalUniqueConsecutiveLocationIndex(size_t loc) const override;

     std::string name() const override {return distName_;}

 private:
     template <typename T>
     void minImpl(T & x) const;

     template <typename T>
     void maxImpl(T & x) const;

     template <typename T>
     void reductionImpl(T & x, eckit::mpi::Operation::Code op) const;

     template <typename T>
     void reductionImpl(std::vector<T> & x, eckit::mpi::Operation::Code op) const;

     std::unique_ptr<Accumulator<int>>
         createAccumulatorImpl(int init) const override;
     std::unique_ptr<Accumulator<std::size_t>>
         createAccumulatorImpl(std::size_t init) const override;
     std::unique_ptr<Accumulator<float>>
         createAccumulatorImpl(float init) const override;
     std::unique_ptr<Accumulator<double>>
         createAccumulatorImpl(double init) const override;
     std::unique_ptr<Accumulator<std::vector<int>>>
         createAccumulatorImpl(const std::vector<int> &init) const override;
     std::unique_ptr<Accumulator<std::vector<std::size_t>>>
         createAccumulatorImpl(const std::vector<std::size_t> &init) const override;
     std::unique_ptr<Accumulator<std::vector<float>>>
         createAccumulatorImpl(const std::vector<float> &init) const override;
     std::unique_ptr<Accumulator<std::vector<double>>>
         createAccumulatorImpl(const std::vector<double> &init) const override;

     template <typename T>
     std::unique_ptr<Accumulator<T>> createAccumulatorImplT(const T &init) const;

     template <typename T>
     void allGathervImpl(std::vector<T> &x) const;

     void computeGlobalUniqueConsecutiveLocIndices(
         const std::vector<std::pair<double, int>> &dist_and_lidx_glb);

     double radius_;
     eckit::geometry::Point2 center_;
     // Indices of records held on this PE
     std::unordered_set<std::size_t> haloObsRecord_;
     std::unordered_map<std::size_t, double> haloObsLoc_;
     // During record assignment holds indices of locations held on this PE
     std::vector<size_t> haloLocVector_;
     // Indicates which observations held on this PE are "patch obs".
     std::vector<bool> patchObsBool_;
     // Maps indices of locations held on this PE to corresponding elements of vectors
     // produced by allGatherv()
     std::vector<size_t> globalUniqueConsecutiveLocIndices_;
     // Earth radius in m
     const double radius_earth_ = 6.371e6;
     // dist name
     const std::string distName_ = "Halo";
};

}  // namespace ioda

#endif  // DISTRIBUTION_HALO_H_
