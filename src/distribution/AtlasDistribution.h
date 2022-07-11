/*
 * (C) Crown copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_ATLASDISTRIBUTION_H_
#define DISTRIBUTION_ATLASDISTRIBUTION_H_

#include <memory>

#include "ioda/distribution/DistributionParametersBase.h"
#include "ioda/distribution/NonoverlappingDistribution.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace eckit {
  class LocalConfiguration;
}

namespace ioda {

/// \brief Parameters describing the AtlasDistribution.
class AtlasDistributionParameters : public DistributionParametersBase {
  OOPS_CONCRETE_PARAMETERS(AtlasDistributionParameters, DistributionParametersBase)

 public:
  oops::RequiredParameter<eckit::LocalConfiguration> grid{"grid",
                                                          "atlas grid and mesh parameters", this};
};

/// \brief Distribution assigning each record to the process owning the Atlas mesh partition
/// containing the location of the first observation in that record.
///
/// The Atlas grid and mesh is created and partitioned using settings taken from the `grid` section
/// of the Configuration passed to the constructor.
class AtlasDistribution: public NonoverlappingDistribution {
 public:
    typedef AtlasDistributionParameters Parameters_;

    AtlasDistribution(const eckit::mpi::Comm & comm, const Parameters_ &);
    ~AtlasDistribution() override;

    void assignRecord(const std::size_t recNum, const std::size_t locNum,
                      const eckit::geometry::Point2 & point) override;

    bool isMyRecord(std::size_t recNum) const override;

    std::string name() const override;

 private:
    class RecordAssigner;
    // Perhaps at some point ioda will tell us when record assignment is complete
    // and we'll be able to deallocate this object.
    std::unique_ptr<RecordAssigner> recordAssigner_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_ATLASDISTRIBUTION_H_
