#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/ReaderFactory.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// GenList
//----------------------------------------------------------------------------------------

// Parameters

class GenListParameters : public ReaderParametersBase {
    OOPS_CONCRETE_PARAMETERS(GenListParameters, ReaderParametersBase)

  public:
    /// \brief latitude values
    oops::RequiredParameter<std::vector<float>> lats{"lats", this};

    /// \brief longitude values
    oops::RequiredParameter<std::vector<float>> lons{"lons", this};

    /// \brief time offsets (s) relative to epoch
    oops::RequiredParameter<std::vector<int64_t>> dateTimes{"dateTimes", this};

    /// \brief epoch (ISO 8601 string) relative to which datetimes are computed
    oops::Parameter<std::string> epoch{"epoch", "seconds since 1970-01-01T00:00:00Z", this};

    /// \brief obs values
    oops::Parameter<std::vector<float>> obsValues{"obs values", { }, this};

    /// \brief obs error estimates
    oops::Parameter<std::vector<float>> obsErrors{"obs errors", { }, this};
};

// Classes

class GenList: public ReaderBase {
 public:
  typedef GenListParameters Parameters_;

  // Constructor via parameters
  GenList(const Parameters_ & params, const ReaderCreationParameters & createParams);

  bool applyLocationsCheck() const override { return false; }

 private:
  /// \brief generate observation locations using the list method
  /// \details This method will generate a set of latitudes and longitudes of which
  ///          can be used for testing without reading in an obs file. The values
  ///          are simply read from lists in the configuration file. The purpose of
  ///          this method is to allow the user to exactly specify obs locations.
  ///          these data are intended for use with the MakeObs functionality.
  /// \param params Parameters structure specific to the generate list method
  void genDistList(const Parameters_ & params);

  void print(std::ostream & os) const override;
};

}  // namespace Engines
}  // namespace ioda
