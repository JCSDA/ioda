#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/ReaderFactory.h"

namespace util {
  class DateTime;
} // namespace util

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// GenRandom
//----------------------------------------------------------------------------------------

// Parameters

class GenRandomParameters : public ReaderParametersBase {
    OOPS_CONCRETE_PARAMETERS(GenRandomParameters, ReaderParametersBase)

  public:
    /// \brief number of observations
    oops::RequiredParameter<int> numObs{"nobs", this};

    /// \brief latitude range start
    oops::RequiredParameter<float> latStart{"lat1", this};

    /// \brief latitude range end
    oops::RequiredParameter<float> latEnd{"lat2", this};

    /// \brief longitude range start
    oops::RequiredParameter<float> lonStart{"lon1", this};

    /// \brief longitude range end
    oops::RequiredParameter<float> lonEnd{"lon2", this};

    /// \brief random seed
    oops::OptionalParameter<int> ranSeed{"random seed", this};

    /// \brief obs values
    oops::Parameter<std::vector<float>> obsValues{"obs values", { }, this};

    /// \brief obs error estimates
    oops::Parameter<std::vector<float>> obsErrors{"obs errors", { }, this};
};

// Classes

class GenRandom: public ReaderBase {
 public:
  typedef GenRandomParameters Parameters_;

  // Constructor via parameters
  GenRandom(const Parameters_ & params, const ReaderCreationParameters & createParams);

  bool applyLocationsCheck() const override { return false; }

 private:
  /// \brief generate observation locations using the random method
  /// \details This method will generate a set of latitudes and longitudes of which
  ///          can be used for testing without reading in an obs file. Two latitude
  ///          values, two longitude values, the number of locations (nobs keyword)
  ///          and an optional random seed are specified in the configuration given
  ///          by the conf parameter. Random locations between the two latitudes and
  ///          two longitudes are generated and stored in the obs container as meta data.
  ///          Random time stamps that fall inside the given timing window (which is
  ///          specified in the configuration file) are also generated and stored
  ///          in the obs container as meta data. These data are intended for use
  ///          with the MakeObs functionality.
  /// \param params Parameters structure specific to the generate random method
  void genDistRandom(const Parameters_ & params);

  void print(std::ostream & os) const override;
};

}  // namespace Engines
}  // namespace ioda
