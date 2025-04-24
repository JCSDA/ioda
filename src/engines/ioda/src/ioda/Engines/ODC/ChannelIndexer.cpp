/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/ChannelIndexer.h"

#include "ioda/Engines/ODC/ChannelIndexerFactory.h"
#include "ioda/Engines/ODC/DataFromSQL.h"
#include "oops/util/IntSetParser.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {
ChannelIndexerMaker<ConstantChannelIndexer> constantMaker("constant");
ChannelIndexerMaker<SequentialChannelIndexer> sequentialMaker("sequential");
ChannelIndexerMaker<ChannelIndexReaderFromFirstLocation>
  readerFromFirstLocationMaker("read from first location");
ChannelIndexerMaker<ChannelIndexReaderFromYaml> readerFromYamlMaker("read from yaml");
}  // namespace

// -----------------------------------------------------------------------------

ConstantChannelIndexer::ConstantChannelIndexer(const Parameters_ &parameters)
  : parameters_(parameters)
{}

std::vector<int> ConstantChannelIndexer::channelIndices(
    const RowsByLocation &rowsByLocation, const DataFromSQL &sqlData) const
{
  std::vector<int> channels;
  if (rowsByLocation.empty())
    return channels;

  std::vector<int> varnos;
  if (parameters_.varnos.value() != boost::none)
    varnos = *parameters_.varnos.value();
  else if (!sqlData.getVarnos().empty())
    varnos = {sqlData.getVarnos().front()};
  else
    return channels;

  const int varnoColumnIndex = sqlData.getColumnIndex("varno");
  if (varnoColumnIndex < 0)
    throw eckit::UserError("'varno' column not found", Here());

  int numChannels = 0;
  for (size_t row : rowsByLocation.front()) {
    const int varno = static_cast<int>(sqlData.getData(row, varnoColumnIndex));
    if (std::find(varnos.begin(), varnos.end(), varno) != varnos.end())
      ++numChannels;
  }

  channels.assign(numChannels, parameters_.channelIndex.value());
  return channels;
}

// -----------------------------------------------------------------------------

ChannelIndexReaderFromYaml::ChannelIndexReaderFromYaml(const Parameters_ &parameters)
  : parameters_(parameters)
{}

std::vector<int> ChannelIndexReaderFromYaml::channelIndices(
    const RowsByLocation &rowsByLocation, const DataFromSQL &sqlData) const
{
  std::vector<int> channels;
  if (rowsByLocation.empty())
    return channels;
  // Channel numbers are passed in as a string, this is converted to a set
  // Then populate a vector of integers with this set.
  const std::set<int> channelSet = oops::parseIntSet(parameters_.channelNumbers);
  std::copy(channelSet.begin(), channelSet.end(), std::back_inserter(channels));
  return channels;
}
// -----------------------------------------------------------------------------

SequentialChannelIndexer::SequentialChannelIndexer(const Parameters_ &parameters)
  : parameters_(parameters)
{}

std::vector<int> SequentialChannelIndexer::channelIndices(
    const RowsByLocation &rowsByLocation, const DataFromSQL &sqlData) const
{
  std::vector<int> channels;
  if (rowsByLocation.empty())
    return channels;

  int numChannels = 0;
  if (parameters_.numChannels.value() != boost::none) {
    numChannels = *parameters_.numChannels.value();
  } else {
    std::vector<int> varnos;
    if (parameters_.varnos.value() != boost::none)
      varnos = *parameters_.varnos.value();
    else if (!sqlData.getVarnos().empty())
      varnos = {sqlData.getVarnos().front()};
    else
      return channels;

    const int varnoColumnIndex = sqlData.getColumnIndex("varno");
    if (varnoColumnIndex < 0)
      throw eckit::UserError("'varno' column not found", Here());

    for (size_t row : rowsByLocation.front()) {
      const int varno = static_cast<int>(sqlData.getData(row, varnoColumnIndex));
      if (std::find(varnos.begin(), varnos.end(), varno) != varnos.end())
        ++numChannels;
    }
  }

  channels.resize(numChannels);
  std::iota(channels.begin(), channels.end(), parameters_.firstChannelIndex.value());
  return channels;
}

// -----------------------------------------------------------------------------

ChannelIndexReaderFromFirstLocation::ChannelIndexReaderFromFirstLocation(
    const Parameters_ &parameters)
  : parameters_(parameters)
{}

std::vector<int> ChannelIndexReaderFromFirstLocation::channelIndices(
    const RowsByLocation &rowsByLocation, const DataFromSQL &sqlData) const
{
  std::vector<int> channels;
  if (rowsByLocation.empty())
    return channels;

  int referenceVarno;
  if (parameters_.varno.value() != boost::none)
    referenceVarno = *parameters_.varno.value();
  else if (!sqlData.getVarnos().empty())
    referenceVarno = sqlData.getVarnos().front();
  else
    return channels;

  const int varnoColumnIndex = sqlData.getColumnIndex("varno");
  if (varnoColumnIndex < 0)
    throw eckit::UserError("'varno' column not found.", Here());

  const int channelColumnIndex = sqlData.getColumnIndex(parameters_.column);
  if (channelColumnIndex < 0)
    throw eckit::UserError("'" + parameters_.column.value() +
                           "' column, expected to contain channel indices, not found.", Here());

  for (size_t row : rowsByLocation.front()) {
    const int varno = static_cast<int>(sqlData.getData(row, varnoColumnIndex));
    if (varno == referenceVarno)
      channels.push_back(static_cast<int>(sqlData.getData(row, channelColumnIndex)));
  }
  return channels;
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
