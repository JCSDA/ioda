#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

/// \file ChannelIndexer.h
/// Concrete implementations of the ChannelIndexerBase interface.

#include "oops/util/parameters/OptionalParameter.h"
#include "ChannelIndexerBase.h"

namespace ioda {
namespace Engines {
namespace ODC {

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of ConstantChannelIndexer.
class ConstantChannelIndexerParameters : public ChannelIndexerParametersBase {
  OOPS_CONCRETE_PARAMETERS(ConstantChannelIndexerParameters,
                           ChannelIndexerParametersBase)
public:
  /// \brief The index to be assigned to all channels.
  oops::Parameter<int> channelIndex{"channel index", 0, this};

  /// \brief A list of varnos. Defaults to a single-element list containing the varno present in
  /// the first row associated with the first location.
  ///
  /// The number of channels is determined by counting rows associated with the first location and
  /// containing any of these varnos.
  oops::OptionalParameter<std::vector<int>> varnos{"varnos", this};
};

/// \brief Assigns the same index (typically zero) to all channels.
///
/// The number of channels corresponds to the number of rows associated with the first location
/// and containing any of the specified varnos.
class ConstantChannelIndexer : public ChannelIndexerBase {
public:
  typedef ConstantChannelIndexerParameters Parameters_;

  explicit ConstantChannelIndexer(const Parameters_ &parameters);

  std::vector<int> channelIndices(const RowsByLocation &rowsByLocation,
                                  const DataFromSQL &sqlData) const override;

private:
  Parameters_ parameters_;
};

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of SequentialChannelIndexer.
class SequentialChannelIndexerParameters : public ChannelIndexerParametersBase {
  OOPS_CONCRETE_PARAMETERS(SequentialChannelIndexerParameters,
                           ChannelIndexerParametersBase)
public:
  /// \brief The index assigned to the first channel.
  oops::Parameter<int> firstChannelIndex{"first channel index", 1, this};

  /// \brief The number of channels to be created.
  ///
  /// If this parameter is not set, the number of channels is determined at runtime; see the
  /// description of the `varnos` parameter for more information.
  oops::OptionalParameter<int> numChannels{"number of channels", this};

  /// \brief A list of varnos. Defaults to a single-element list containing the varno in the first
  /// row associated with the first location.
  ///
  /// The number of channels is determined by counting rows associated with the first location and
  /// containing any of these varnos.
  ///
  /// If the `number of channels` parameter is set, this parameter is ignored.
  oops::OptionalParameter<std::vector<int>> varnos{"varnos", this};
};

/// \brief Creates sequential channel indices starting from a specified base.
///
/// The number of channels to be created can either be set in advance via the `number of channels`
/// parameter or determined at runtime by counting the number of rows with specific varnos
/// associated with the first location.
class SequentialChannelIndexer : public ChannelIndexerBase {
public:
  typedef SequentialChannelIndexerParameters Parameters_;

  explicit SequentialChannelIndexer(const Parameters_ &parameters);

  std::vector<int> channelIndices(const RowsByLocation &rowsByLocation,
                                  const DataFromSQL &sqlData) const override;

private:
  Parameters_ parameters_;
};

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of ChannelIndexReaderFromFirstLocation.
class ChannelIndexReaderFromFirstLocationParameters : public ChannelIndexerParametersBase {
  OOPS_CONCRETE_PARAMETERS(ChannelIndexReaderFromFirstLocationParameters,
                           ChannelIndexerParametersBase)
public:
  /// \brief Name of the ODB column from which channel indices will be read.
  oops::Parameter<std::string> column{"column", "initial_vertco_reference", this};

  /// \brief Channel indices will be read only from rows containing this varno.
  ///
  /// Defaults to the varno in the first row associated with the first location.
  oops::OptionalParameter<int> varno{"varno", this};
};

/// \brief Reads channel indices from rows with a specific varno associated with the first location.
class ChannelIndexReaderFromFirstLocation : public ChannelIndexerBase {
public:
  typedef ChannelIndexReaderFromFirstLocationParameters Parameters_;

  explicit ChannelIndexReaderFromFirstLocation(const Parameters_ &parameters);

  std::vector<int> channelIndices(const RowsByLocation &rowsByLocation,
                                  const DataFromSQL &sqlData) const override;
private:
  Parameters_ parameters_;
};

/// \brief Parameters controlling the behavior of ChannelIndexReaderFromYaml.
class ChannelIndexReaderFromYamlParameters : public ChannelIndexerParametersBase {
  OOPS_CONCRETE_PARAMETERS(ChannelIndexReaderFromYamlParameters,
                           ChannelIndexerParametersBase)
public:
  /// \brief A vector of channel numbers to be created from the odb query file
  oops::RequiredParameter<std::string> channelNumbers{"channel numbers", this};
};

/// \brief Reads channel indices from the odb query file
///
/// The channels must be passed in as a comma separated string
class ChannelIndexReaderFromYaml : public ChannelIndexerBase {
public:
  typedef ChannelIndexReaderFromYamlParameters Parameters_;

  explicit ChannelIndexReaderFromYaml(const Parameters_ &parameters);

  std::vector<int> channelIndices(const RowsByLocation &rowsByLocation,
                                  const DataFromSQL &sqlData) const override;

private:
  Parameters_ parameters_;
};

// -----------------------------------------------------------------------------

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
