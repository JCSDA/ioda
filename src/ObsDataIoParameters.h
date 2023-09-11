#pragma once
/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ReaderFactory.h"
#include "ioda/Engines/WriterFactory.h"

namespace ioda {

enum class MissingSortValueTreatment {
  SORT, NO_SORT, IGNORE_MISSING
};

struct MissingSortValueTreatmentParameterTraitsHelper {
  typedef MissingSortValueTreatment EnumType;
  static constexpr char enumTypeName[] = "MissingSortValueTreatment";
  static constexpr util::NamedEnumerator<MissingSortValueTreatment> namedValues[] = {
    { MissingSortValueTreatment::SORT, "sort" },
    { MissingSortValueTreatment::NO_SORT, "do not sort" },
    { MissingSortValueTreatment::IGNORE_MISSING, "ignore missing" }
  };
};

}  // namespace ioda


namespace oops {

template <>
struct ParameterTraits<ioda::MissingSortValueTreatment> :
    public EnumParameterTraits<ioda::MissingSortValueTreatmentParameterTraitsHelper>
{};

}  // namespace oops


namespace ioda {

/// \brief Options controlling the manner in which observations are grouped into records.
class ObsGroupingParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsGroupingParameters, Parameters)

 public:
    /// variable of which to base obs record grouping
    oops::Parameter<std::vector<std::string>> obsGroupVars{"group variables", {}, this};

    /// variable of which to base obs record sorting
    oops::Parameter<std::string> obsSortVar{"sort variable", "", this};

    /// name of group of which to base obs record sorting
    oops::Parameter<std::string> obsSortGroup{"sort group", "MetaData", this};

    /// direction for sort
    oops::Parameter<std::string> obsSortOrder{"sort order", "ascending", this};

    /// treatment of missing sort values
    oops::Parameter<MissingSortValueTreatment> missingSortValueTreatment{
      "missing sort value treatment", MissingSortValueTreatment::SORT, this};
};

class ObsDataInParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsDataInParameters, oops::Parameters)

 public:
    /// options controlling obs record grouping
    oops::Parameter<ObsGroupingParameters> obsGrouping{"obsgrouping", { }, this};

    /// option controlling the creation of the backend
    oops::RequiredParameter<Engines::ReaderParametersWrapper> engine{"engine", this};
};

class ObsDataOutParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsDataOutParameters, oops::Parameters)

 public:
    /// option controlling the creation of the backend
    oops::RequiredParameter<Engines::WriterParametersWrapper> engine{"engine", this};
};

}  // namespace ioda
