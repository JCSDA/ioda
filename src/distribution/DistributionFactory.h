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
#include "eckit/geometry/Point2.h"
#include "ioda/distribution/Distribution.h"

namespace ioda {

// -----------------------------------------------------------------------------

/// \brief Distribution factory.
class DistributionFactory {
 public:
  virtual ~DistributionFactory() = default;

  /// \brief Create a Distribution object implementing a particular method of distributing
  /// observations across multiple process elements..
  ///
  /// This method creates an instance of the Distribution subclass indicated by the value of the
  /// `distribution` option in `config`. If this option is not present, an instance of the
  /// RoundRobin distribution is returned.
  ///
  /// \param[in] comm Local MPI communicator
  /// \param[in] config Top-level ObsSpace configuration.
  static std::unique_ptr<Distribution> create(const eckit::mpi::Comm & comm,
                                              const eckit::Configuration & config);

 protected:
  explicit DistributionFactory(const std::string &name);

 private:
  virtual std::unique_ptr<Distribution> make(const eckit::mpi::Comm & comm,
                                             const eckit::Configuration &config) = 0;

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
  std::unique_ptr<Distribution> make(const eckit::mpi::Comm & comm,
                                     const eckit::Configuration &config) override
  { return std::unique_ptr<Distribution>(new T(comm, config)); }

 public:
  explicit DistributionMaker(const std::string & name) : DistributionFactory(name) {}
};

// ---------------------------------------------------------------------

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTIONFACTORY_H_
