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
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/RequiredParameter.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionParametersBase.h"

namespace ioda {

class HaloDistributionParameters : public DistributionParametersBase {
  OOPS_CONCRETE_PARAMETERS(HaloDistributionParameters, DistributionParametersBase)

 public:
  oops::OptionalParameter<std::vector<double>> center{"center", this};
  oops::Parameter<double> radius{"radius", 50000000.0, this};
  oops::RequiredParameter<double> haloSize{"halo size", "halo size [m]", this};
};


// ---------------------------------------------------------------------
/*!
 * \brief Halo distribution
 *
 * \details All obs. are divided into compact overlapping sets assigned to each PE.
 * Specifically, a record is assigned to a PE if the first location belonging to that record
 * lies at most a certain distance (controlled by the option `radius`) from the center of the halo
 * associated with that PE (controlled by the option `center`).
 *
 * \author Sergey Frolov (CIRES)
 */
class Halo: public Distribution {
 public:
     typedef HaloDistributionParameters Parameters_;
     Halo(const eckit::mpi::Comm & Comm, const Parameters_ &);
     ~Halo();

     void assignRecord(const std::size_t RecNum, const std::size_t LocNum,
                      const eckit::geometry::Point2 & point) override;
     bool isMyRecord(std::size_t RecNum) const override;
     void computePatchLocs() override;
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
     // Record numbers held on this PE
     std::unordered_set<std::size_t> recordsInHalo_;
     // Indicates which observations held on this PE are "patch obs".
     std::vector<bool> patchObsBool_;
     // Maps indices of locations held on this PE to corresponding elements of vectors
     // produced by allGatherv()
     std::vector<size_t> globalUniqueConsecutiveLocIndices_;

     // The following four member variables are valid only during record assignment,
     // i.e. until the call to computePatchLocs().

     // Record numbers not to be held on this PE
     std::unordered_set<std::size_t> recordsOutsideHalo_;
     // The distance of the first location of each record held on this PE
     // to the center of this PE's halo.
     std::unordered_map<std::size_t, double> recordDistancesFromCenter_;
     // Record numbers of locations held on this PE
     std::vector<size_t> haloLocRecords_;
     // Indices of locations held on this PE
     std::vector<size_t> haloLocVector_;

     // Earth radius in m
     const double radius_earth_ = 6.371e6;
     // dist name
     const std::string distName_ = "Halo";
};

}  // namespace ioda

#endif  // DISTRIBUTION_HALO_H_
