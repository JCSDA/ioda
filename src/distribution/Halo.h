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
     std::size_t computePatchLocs(const std::size_t nglocs) override;
     void patchObs(std::vector<bool> &) const override;

     double dot_product(const std::vector<double> &v1, const std::vector<double> &v2)
                      const override;
     double dot_product(const std::vector<float> &v1, const std::vector<float> &v2)
                      const override;
     double dot_product(const std::vector<int> &v1, const std::vector<int> &v2)
                      const override;

     size_t globalNumNonMissingObs(const std::vector<double> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<float> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<int> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<std::string> &v) const override;
     size_t globalNumNonMissingObs(const std::vector<util::DateTime> &v) const override;

     void sum(double &x) const override;
     void sum(int &x) const override;
     void sum(size_t &x) const override;
     void sum(std::vector<double> &x) const override;
     void sum(std::vector<size_t> &x) const override;

     void min(double &x) const override;
     void min(float &x) const override;
     void min(int &x) const override;

     void max(double &x) const override;
     void max(float &x) const override;
     void max(int &x) const override;

     void allGatherv(std::vector<size_t> &x) const override;
     void allGatherv(std::vector<int> &x) const override;
     void allGatherv(std::vector<float> &x) const override;
     void allGatherv(std::vector<double> &x) const override;
     void allGatherv(std::vector<util::DateTime> &x) const override;
     void allGatherv(std::vector<std::string> &x) const override;

     void exclusiveScan(size_t &x) const override;

     std::string name() const override {return distName_;}

 private:
     template <typename T>
     double dot_productImpl(const std::vector<T> &v1, const std::vector<T> &v2) const;
     template <typename T>
     size_t globalNumNonMissingObsImpl(const std::vector<T> &v) const;

     double radius_;
     eckit::geometry::Point2 center_;
     // storage container for halo ids
     std::unordered_set<std::size_t> haloObsRecord_;
     std::unordered_map<std::size_t, double> haloObsLoc_;
     std::vector<size_t> haloLocVector_;
     // storage container for patch ids
     std::unordered_set<std::size_t> patchObsLoc_;
     std::vector<bool> patchObsBool_;
     // Earth radius in m
     const double radius_earth_ = 6.371e6;
     // dist name
     const std::string distName_ = "Halo";
};

}  // namespace ioda

#endif  // DISTRIBUTION_HALO_H_
