#pragma once
/*
 * (C) Crown Copyright Met Office 2021
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_engines_pub_ODC
 *
 * @{
 * \file OdbQueryParameters.h
 * \brief ODB / ODC engine bindings
 */

#include <string>
#include <vector>

#include "ioda/Variables/Variable.h"
#include "oops/util/AnyOf.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/ParameterTraitsAnyOf.h"
#include "oops/util/parameters/PolymorphicParameter.h"
#include "oops/util/parameters/RequiredParameter.h"

#include "ChannelIndexerFactory.h"
#include "ObsGroupTransformFactory.h"
#include "RowsIntoLocationsSplitterFactory.h"
#include "VariableReaderFactory.h"

namespace eckit {
  class Configuration;
}

namespace ioda {
namespace Engines {
namespace ODC {

enum class StarParameter {
  ALL
};

struct StarParameterTraitsHelper{
  typedef StarParameter EnumType;
  static constexpr char enumTypeName[] = "StarParameter";
  static constexpr util::NamedEnumerator<StarParameter> namedValues[] = {
    { StarParameter::ALL, "ALL" }
  };
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda

namespace oops {

template<>
struct ParameterTraits<ioda::Engines::ODC::StarParameter> :
    public EnumParameterTraits<ioda::Engines::ODC::StarParameterTraitsHelper>
{};

}  // namespace oops

namespace ioda {
namespace Engines {
namespace ODC {

/// \brief A container for the configuration options of an ObsGroup transform.
class ObsGroupTransformParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(ObsGroupTransformParameters, Parameters)
 public:
  /// After deserialization, holds an instance of a subclass of
  /// ObsGroupTransformParametersBase controlling the transform's behavior. The type of the
  /// subclass is determined by the value of the "name" key in the Configuration object from which
  /// this object is deserialized.
  oops::RequiredPolymorphicParameter<Engines::ODC::ObsGroupTransformParametersBase,
                                     Engines::ODC::ObsGroupTransformFactory> params{"name", this};
};

class OdbVariableParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbVariableParameters, Parameters)

 public:
  /// The column to use to match the conditions
  oops::RequiredParameter<std::string> name{"name", this};

  /// Select locations at which the condition variable is greater than or equal to the specified
  /// value. Can be set to an int, float or datetime in the ISO 8601 format (if any datetime
  /// components are zero, they are ignored).
  oops::OptionalParameter<util::AnyOf<int, float, util::PartialDateTime>> minvalue{
    "min value", this
  };

  /// Select locations at which the condition variable is less than or equal to the specified
  /// value. Can be set to an int, float or datetime in the ISO 8601 format (if any datetime
  /// components are zero, they are ignored).
  oops::OptionalParameter<util::AnyOf<int, float, util::PartialDateTime>> maxvalue{
    "max value", this};

  /// Select locations at which the condition variable is not set to the missing value indicator.
  oops::OptionalParameter<bool> isDefined{"is defined", this};
};

class OdbWhereParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbWhereParameters, Parameters)

 public:
  /// The varnos to query data from
  oops::RequiredParameter<util::AnyOf<StarParameter, std::vector<int>>> varno{
      "varno", this};
  /// Optional free-form query
  oops::Parameter<std::string> query{
      "query", "", this};
};

/// \brief A container for the configuration options of an object associating ODB rows with JEDI
/// locations.
class RowsIntoLocationsSplitterParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(RowsIntoLocationsSplitterParameters, Parameters)
 public:
  /// After deserialization, holds an instance of a subclass of
  /// RowsIntoLocationsSplitterParametersBase controlling the splitter's behavior. The type of that
  /// subclass is determined by the value of the `method` key in the Configuration object from which
  /// this object is deserialized.
  oops::PolymorphicParameter<RowsIntoLocationsSplitterParametersBase,
                             RowsIntoLocationsSplitterFactory> params{"method", "by seqno", this};
};

/// \brief A container for the configuration options of an object enumerating channel indices.
class ChannelIndexerParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(ChannelIndexerParameters, Parameters)
 public:
  /// After deserialization, holds an instance of a subclass of ChannelIndexerParametersBase
  /// controlling the indexer's behavior. The type of that subclass is determined by the value of
  /// the `method` key in the Configuration object from which this object is deserialized.
  oops::PolymorphicParameter<ChannelIndexerParametersBase,
                             ChannelIndexerFactory> params{"method", "sequential", this};
};

/// \brief A container for the configuration options of the default object used to extract variable
/// values from varno-independent columns.
class DefaultVariableReaderParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(DefaultVariableReaderParameters, Parameters)
 public:
  oops::PolymorphicParameter<VariableReaderParametersBase, VariableReaderFactory> params{
      "type", "from rows with non-missing values", this};
};

/// \brief Configuration options controlling how the results of an ODB query are converted into
/// ioda variables.
class OdbVariableCreationParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbVariableCreationParameters, Parameters)

 public:
  /// The epoch to use for DateTime variables
  oops::Parameter<std::string> epoch{"epoch",
                                     "seconds since 1970-01-01T00:00:00Z", this};

  /// The int64 missing value
  oops::Parameter<int64_t> missingInt64{"missingInt64",
                                        -9223372036854775806, this};

  /// The name of a time displacement variable which is added to the dateTime at each location.
  /// If this name is blank (the default) then no displacement will be applied.
  oops::Parameter<std::string> timeDisplacement{"time displacement variable", "", this};

  /// Configuration of the object mapping ODB rows to JEDI locations.
  oops::Parameter<RowsIntoLocationsSplitterParameters> rowsIntoLocationsSplit{
    "rows into locations split", {}, this};

  /// List of columns used to group *consecutive* ODB rows into records.
  ///
  /// Sequences of consecutive ODB rows with identical values in all these columns will be kept on a
  /// single MPI process even if parallel I/O is enabled.
  ///
  /// If not set, this list of columns will be determined automatically in a way that will ensure
  /// all rows belonging to a single *location* are kept on the same MPI process.
  oops::OptionalParameter<std::vector<std::string>> recordGroupingColumns{
    "record grouping columns", this};

  /// List of multichannel varnos. Variables storing data extracted from rows containing these
  /// varnos in varno-dependent columns will be equipped with a Channel dimension.
  ///
  /// If this list is non-empty, the `channel indexing` option must be set as well.
  oops::Parameter<std::vector<int>> multichannelVarnos{"multichannel varnos", {}, this};

  /// Configuration of the object assigning channel indices.
  ///
  /// If not set, the Channel variable is not created and no variables can have the Channel
  /// dimension.
  oops::OptionalParameter<ChannelIndexerParameters> channelIndexing{
    "channel indexing", this};

  /// Configuration of the object used by default to extract values from varno-independent columns
  /// into ioda variables. This choice can be overridden for individual variables through the
  /// VariableParameters::reader option.
  oops::Parameter<DefaultVariableReaderParameters> defaultReader{
    "default reader", {}, this};

  /// Set this option to false if variables should be created also for varnos present in the query
  /// but absent from the ODB file. By default such variables are not created.
  oops::Parameter<bool> skipMissingVarnos{"skip variables corresponding to missing varnos",
                                          true, this};

  /// \brief A list of configuration options of transforms applied to the ObsGroup after filling it
  /// with variables created from individual columns produced by an ODB query.
  ///
  /// Such transforms can be used, in particular, to create variables composed of data held in
  /// multiple ODB columns, such as station identifiers.
  oops::Parameter<std::vector<ObsGroupTransformParameters>> transforms{"post-read transforms",
                                                                       {}, this};
  // An analogous 'pre-write transforms' option could be added to allow the ObsGroup to be
  // transformed (perhaps temporarily) before it is written to an ODB file, for example to split
  // station IDs into components written to separate ODB columns.

  // Import both overloads of deserialize() from the base class. We will override one of them.
  using Parameters::deserialize;

  /// \brief Load the values of all registered parameters from the configuration \p config.
  ///
  /// The base class implementation is overridden to throw an exception if the `multichannel varnos`
  /// list is non-empty but the `channel indexing` option has not been set.
  void deserialize(util::CompositePath &path, const eckit::Configuration &config) override;
};

class OdbQueryParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(OdbQueryParameters, Parameters)

 public:
  /// Variables to select
  oops::Parameter<std::vector<OdbVariableParameters>> variables{ "variables", {}, this };

  /// Selection criteria
  oops::RequiredParameter<OdbWhereParameters> where{"where", this};

  /// Parameters related to variable creation
  OdbVariableCreationParameters variableCreation{this};
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
