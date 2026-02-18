#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/ObsGroupTransformBase.h"
#include "ioda/Engines/ODC/OdbQueryParameters.h"
#include "ioda/Engines/ODC.h"

#include "oops/util/parameters/ArrayConstraints.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"

namespace ioda {

class Variable;

namespace Engines {
namespace ODC {

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of a CreateDateTimeTransform.
class CreateDateTimeTransformParameters : public ObsGroupTransformParametersBase {
  OOPS_CONCRETE_PARAMETERS(CreateDateTimeTransformParameters, ObsGroupTransformParametersBase)
public:
  /// \brief Name of an integer-valued input variable storing dates loaded from an ODB column,
  /// in the format YYYYMMDD (e.g. with 19871015 standing for 15 Oct 1987).
  oops::Parameter<std::string> inputDate{"input date", "MetaData/__date", this};

  /// \brief Name of an integer-valued input variable storing times loaded from an ODB column,
  /// in the format HHMMSS (e.g. with 175400 standing for 17:54:00).
  oops::Parameter<std::string> inputTime{"input time", "MetaData/__time", this};

  /// \brief Name of the output variable to be created.
  oops::Parameter<std::string> output{"output", "MetaData/dateTime", this};

  /// \brief If true and an extended lower bound on the time window has been set, datetimes lying
  /// between that bound and the start of the time window will be moved to the start of that window.
  /// This ensures that the corresponding observations will be accepted by the time window cutoff
  /// applied in oops.
  oops::Parameter<bool> clampToWindowStart{"clamp to window start", false, this};

  /// \brief Name of an integer-valued variable whose elements represent the number of seconds that
  /// should be added to the datetimes constructed from elements of the `inputDate` and `inputTime`
  /// variables.
  oops::OptionalParameter<std::string> displaceBy{"displace by", this};
};

/// \brief Converts a pair of integer-valued ioda variables storing dates and times loaded from ODB
/// columns into a single ioda variable of type `int64_t` storing these dates and times as offsets
/// from the epoch specified in OdbQueryParameters.
class CreateDateTimeTransform : public ObsGroupTransformBase {
public:
  typedef CreateDateTimeTransformParameters Parameters_;

  explicit CreateDateTimeTransform(const Parameters_ &transformParameters,
                                   const ODC_Parameters &odcParameters,
                                   const OdbVariableCreationParameters &varCreationParameters);

  void transform(ContainerFacade &container) const override;

private:
  Parameters_ transformParameters_;
  ODC_Parameters odcParameters_;
  OdbVariableCreationParameters variableCreationParameters_;
};

// -----------------------------------------------------------------------------

/// \brief Parameters controlling extraction of station IDs from a single ioda variable.
class VariableSourceParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(VariableSourceParameters, Parameters)
public:
  /// \brief Name of the variable from which station IDs will be extracted. The variable must be of
  /// type `int` or `string`.
  oops::RequiredParameter<std::string> name{"name", this};
  /// \brief If this option is set and the specified variable is of type `int`, numerical station
  /// IDs extracted from it will be padded on the left with spaces or zeros (see `pad with zeros`)
  /// to this number of characters.
  oops::OptionalParameter<int> width{"width", this};
  /// \brief True to pad numerical station IDs with zeros rather than spaces. Ignored if `width` is
  /// not set.
  oops::OptionalParameter<bool> padWithZeros{"pad with zeros", this};
};

/// \brief Parameters controlling extraction of 5-digit WMO station IDs from a pair of ioda
/// variables.
class WmoIdSourceParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(WmoIdSourceParameters, Parameters)
public:
  /// \brief Name of an ioda variable containing WMO block numbers.
  oops::RequiredParameter<std::string> blockNumber{"block number", this};
  /// \brief Name of an ioda variable containing WMO station numbers.
  oops::RequiredParameter<std::string> stationNumber{"station number", this};
};

/// \brief Parameters of a source of station IDs.
///
/// Either the `variable` or `wmo id` option must be set, but not both.
class StationIdSourceParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(StationIdSourceParameters, Parameters)
public:
  /// \brief Descriptor of an ioda variable serving as a source of station IDs.
  oops::OptionalParameter<VariableSourceParameters> variable{"variable", this};
  /// \brief Descriptor of a pair of ioda variables containing WMO block and station numbers,
  /// from which 5-digit WMO station IDs should be constructed.
  oops::OptionalParameter<WmoIdSourceParameters> wmoId{"wmo id", this};

  // Import both overloads of deserialize() from the base class. We will override one of them.
  using Parameters::deserialize;

  /// \brief Load the values of all registered parameters from the configuration \p config.
  ///
  /// The base class implementation is overridden to throw an exception if both or none of the
  /// `variable` and `wmo id` options is set.
  void deserialize(util::CompositePath &path, const eckit::Configuration &config) override;
};

/// \brief Parameters controlling the behavior of CreateStationIdTransform.
class CreateStationIdTransformParameters : public ObsGroupTransformParametersBase {
  OOPS_CONCRETE_PARAMETERS(CreateStationIdTransformParameters, ObsGroupTransformParametersBase)
public:
  /// \brief List of the sources from which station IDs may be extracted, in descending priority
  /// order.
  ///
  /// \note If none of these sources contains a non-missing value at a given location, the initial
  /// value of the destination variable is inspected. If it is non-empty, it is used as the station
  /// ID. Otherwise the station ID is set to the missing value indicator, i.e. `MISSING*`.
  oops::RequiredParameter<std::vector<StationIdSourceParameters>> sources{"sources", this};
  /// \brief Name of the variable to be filled with station IDs. This variable must already exist
  /// at the time CreateStationIdTransform is invoked.
  oops::Parameter<std::string> destination{"destination", "MetaData/stationIdentification", this};
};

/// \brief Fills a variable with station IDs extracted from one of the sources specified in the
/// parameters, namely the highest-priority source that contains a non-missing value.
class CreateStationIdTransform : public ObsGroupTransformBase {
public:
  typedef CreateStationIdTransformParameters Parameters_;

  explicit CreateStationIdTransform(const Parameters_ &,
                                    const ODC_Parameters &,
                                    const OdbVariableCreationParameters &);

  void transform(ContainerFacade &container) const override;

private:
  Parameters_ parameters_;
};


// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of ConcatenateVariablesTransform.
class ConcatenateVariablesTransformParameters : public ObsGroupTransformParametersBase {
  OOPS_CONCRETE_PARAMETERS(ConcatenateVariablesTransformParameters, ObsGroupTransformParametersBase)
public:
  /// \brief List of variables to be concatenated. All these variables must be of type string.
  /// The list must not be empty.
  oops::RequiredParameter<std::vector<std::string>> sources{
      "sources", this, {oops::nonEmptyConstraint<std::vector<std::string>>()}};
  /// \brief Name of the destination variable that will store the concatenated strings.
  oops::RequiredParameter<std::string> destination{"destination", this};
};

/// \brief Concatenates elements stored at the corresponding locations (and channels, if present) in
/// multiple variables of type `string` and stores the result in a single new variable.
class ConcatenateVariablesTransform : public ObsGroupTransformBase {
public:
  typedef ConcatenateVariablesTransformParameters Parameters_;

  ConcatenateVariablesTransform(const Parameters_ &,
                                const ODC_Parameters &,
                                const OdbVariableCreationParameters &);

  void transform(ContainerFacade &container) const override;

private:
  /// \brief Return the vector of dimension scales to be attached to the destination variable.
  static std::vector<Variable> destinationDimensionScales(const ObsGroup &og,
                                                          const Variable &firstSource);

  Parameters_ transformParameters_;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
