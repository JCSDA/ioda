/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_DISTRIBUTIONFACTORY_H_
#define DISTRIBUTION_DISTRIBUTIONFACTORY_H_

#include <string>
#include <vector>
#include <memory>

#include "eckit/config/Configuration.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionParametersBase.h"
#include "oops/util/AssociativeContainers.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/PolymorphicParameter.h"

namespace ioda {

// -----------------------------------------------------------------------------
class DistributionFactory;

/// \brief Contains a polymorphic parameter holding an instance of a subclass of
/// DistributionParametersBase.
class DistributionParametersWrapper : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(DistributionParametersWrapper, Parameters)

 public:
  /// After deserialization, holds an instance of a subclass of DistributionParametersBase
  /// controlling the behavior of the observation distribution. The type of the subclass is
  /// determined by the value of the "name" key in the Configuration object from which this
  /// object is deserialized.
  oops::PolymorphicParameter<DistributionParametersBase, DistributionFactory>
    params{"name", "type of the observation MPI distribution", "RoundRobin", this};
};

/// \brief Distribution factory.
class DistributionFactory {
 public:
  virtual ~DistributionFactory() = default;

  /// \brief Create a Distribution object implementing a particular method of distributing
  /// observations across multiple process elements..
  ///
  /// This method creates an instance of the Distribution subclass indicated by the
  /// `name` attribute of \p params. \p params must be an instance of the subclass of
  /// DistributionParametersBase associated with that distribution, otherwise an exception
  /// will be thrown.
  /// \param[in] comm   Local MPI communicator
  /// \param[in] params Distribution parameters.
  static std::unique_ptr<Distribution> create(const eckit::mpi::Comm & comm,
                                              const DistributionParametersBase & params);

  /// \brief Create and return an instance of the subclass of DistributionParametersBase
  /// storing parameters of distribution of the specified type.
  static std::unique_ptr<DistributionParametersBase> createParameters(const std::string &name);

  /// \brief Return the names of all distributions that can be created by one of the registered
  /// makers.
  static std::vector<std::string> getMakerNames() {
    return oops::keys(getMakers());
  }

 protected:
  /// \brief Register a maker able to create distributions of type \p name.
  explicit DistributionFactory(const std::string &name);

 private:
  virtual std::unique_ptr<Distribution> make(const eckit::mpi::Comm & comm,
                                             const DistributionParametersBase &) = 0;
  virtual std::unique_ptr<DistributionParametersBase> makeParameters() const = 0;

  static std::map < std::string, DistributionFactory * > & getMakers() {
    static std::map < std::string, DistributionFactory * > makers_;
    return makers_;
  }
};

// -----------------------------------------------------------------------------

/// \brief A class able to instantiate objects of type T, which should be a subclass of
/// Distribution.
template<class T>
class DistributionMaker : public DistributionFactory {
  typedef typename T::Parameters_ Parameters_;

  std::unique_ptr<Distribution> make(const eckit::mpi::Comm & comm,
                                     const DistributionParametersBase & params) override {
    const auto &stronglyTypedParams = dynamic_cast<const Parameters_&>(params);
    return std::make_unique<T>(comm, stronglyTypedParams);
  }

  std::unique_ptr<DistributionParametersBase> makeParameters() const override {
    return std::make_unique<Parameters_>();
  }

 public:
  explicit DistributionMaker(const std::string & name) : DistributionFactory(name) {}
};

// ---------------------------------------------------------------------

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTIONFACTORY_H_
