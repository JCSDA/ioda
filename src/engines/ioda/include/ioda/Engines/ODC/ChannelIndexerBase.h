#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>

#include "RowsByLocation.h"

#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace ioda {
namespace Engines {
namespace ODC {

class DataFromSQL;

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of a subclass of ChannelIndexerBase.
class ChannelIndexerParametersBase : public oops::Parameters {
  OOPS_ABSTRACT_PARAMETERS(ChannelIndexerParametersBase, Parameters)

 public:
  /// \brief Channel indexing method.
  oops::Parameter<std::string> method{"method", "sequential", this};
};

// -----------------------------------------------------------------------------

/// \brief Creates channel indices for data loaded from an ODB file.
///
/// Each subclass needs to typedef `Parameters_` to the type of a subclass of
/// ChannelIndexerParametersBase storing its configuration options, and to provide a constructor
/// taking a reference to that class as its sole argument.
class ChannelIndexerBase {
public:
  virtual ~ChannelIndexerBase() {}

  /// \brief Fills and returns a vector of channel indices.
  ///
  /// \param rowsByLocation
  ///   Maps location indices to indices of rows associated with those locations.
  /// \param sqlData
  ///   Data loaded from an ODB file.
  virtual std::vector<int> channelIndices(const RowsByLocation &rowsByLocation,
                                          const DataFromSQL &sqlData) const = 0;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
