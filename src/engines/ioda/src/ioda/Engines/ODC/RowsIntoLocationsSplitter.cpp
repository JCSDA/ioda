/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/RowsIntoLocationsSplitter.h"

#include "ioda/Engines/ODC/DataFromSQL.h"
#include "ioda/Engines/ODC/RowsIntoLocationsSplitterFactory.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {
RowsIntoLocationsSplitterMaker<RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno>
splitterBySeqnoThenByCounterOfRowsWithVarnoMaker(
    "by seqno, then by the counter of rows with a given varno");

RowsIntoLocationsSplitterMaker<RowsIntoLocationsSplitterBySeqno>
splitterBySeqnoMaker("by seqno");
}  // namespace

// -----------------------------------------------------------------------------

RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno::
RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno(
    const Parameters_ &parameters)
  : parameters_(parameters)
{}

RowsByLocation
RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno::groupRowsByLocation(
    const DataFromSQL &sqlData) const {
  const size_t numRows = sqlData.getNumberOfRows();
  const int seqnoColumnIndex = sqlData.getColumnIndex("seqno");
  const int varnoColumnIndex = sqlData.getColumnIndex("varno");
  const int numlevColumnIndex = parameters_.keepOnlyReportedLevels ?
        sqlData.getColumnIndex("numlev") : -1;

  if (seqnoColumnIndex < 0)
    throw eckit::UserError("'seqno' column not found", Here());
  if (varnoColumnIndex < 0)
    throw eckit::UserError("'varno' column not found", Here());
  if (parameters_.keepOnlyReportedLevels && numlevColumnIndex < 0)
    throw eckit::UserError("'numlev' column not found", Here());

  std::map<int, size_t> nextLevelIndexByVarno;

  RowsByLocation rowsByLocation;

  size_t previousSeqno = -1;
  size_t firstLocationInThisProfile = 0;
  size_t numReportedLevels = std::numeric_limits<size_t>::max();
  for (size_t row = 0; row < numRows; row++) {
    const size_t seqno = static_cast<size_t>(sqlData.getData(row, seqnoColumnIndex));
    if (seqno != previousSeqno || rowsByLocation.empty()) {
      for (int varno : sqlData.getVarnos())
        nextLevelIndexByVarno[varno] = 0;
      firstLocationInThisProfile = rowsByLocation.size();
      previousSeqno = seqno;
      if (parameters_.keepOnlyReportedLevels)
        numReportedLevels = static_cast<size_t>(sqlData.getData(row, numlevColumnIndex));
    }
    const int varno = static_cast<int>(sqlData.getData(row, varnoColumnIndex));
    size_t &nextLevelIndex = nextLevelIndexByVarno.at(varno);
    if (nextLevelIndex < numReportedLevels) {
      const size_t nextLocationIndex = firstLocationInThisProfile + nextLevelIndex;
      if (nextLocationIndex == rowsByLocation.size())
        rowsByLocation.push_back({row});
      else
        rowsByLocation[nextLocationIndex].push_back(row);
      ++nextLevelIndex;
    }
  }

  return rowsByLocation;
}

std::vector<std::string>
RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno::defaultRecordIdColumns() const {
  return {"seqno"};
}

// -----------------------------------------------------------------------------

RowsIntoLocationsSplitterBySeqno::RowsIntoLocationsSplitterBySeqno(
    const Parameters_ &parameters)
  : parameters_(parameters)
{}

RowsByLocation RowsIntoLocationsSplitterBySeqno::groupRowsByLocation(
    const DataFromSQL &sqlData) const {
  if (parameters_.maxNumChannels.value() < std::numeric_limits<size_t>::max())
    return groupRowsByLocationWithMaxNumChannelsConstraint(sqlData);
  else
    return groupRowsByLocationWithoutConstraints(sqlData);
}

RowsByLocation RowsIntoLocationsSplitterBySeqno::groupRowsByLocationWithoutConstraints(
    const DataFromSQL &sqlData) const {
  const size_t numRows = sqlData.getNumberOfRows();
  const int seqnoColumnIndex = sqlData.getColumnIndex("seqno");
  if (seqnoColumnIndex < 0)
    throw eckit::UserError("'seqno' column not found", Here());

  RowsByLocation rowsByLocation;

  size_t previousSeqno = -1;
  for (size_t row = 0; row < numRows; row++) {
    const size_t seqno = static_cast<size_t>(sqlData.getData(row, seqnoColumnIndex));
    if (seqno != previousSeqno || rowsByLocation.empty()) {
      rowsByLocation.push_back({row});
      previousSeqno = seqno;
    } else {
      rowsByLocation.back().push_back(row);
    }
  }
  return rowsByLocation;
}

RowsByLocation RowsIntoLocationsSplitterBySeqno::groupRowsByLocationWithMaxNumChannelsConstraint(
    const DataFromSQL &sqlData) const {
  const size_t numRows = sqlData.getNumberOfRows();
  const std::vector<int> &varnos = sqlData.getVarnos();
  const size_t maxNumChannels = parameters_.maxNumChannels.value();

  const int seqnoColumnIndex = sqlData.getColumnIndex("seqno");
  const int varnoColumnIndex = sqlData.getColumnIndex("varno");
  if (seqnoColumnIndex < 0)
    throw eckit::UserError("'seqno' column not found", Here());
  if (varnoColumnIndex < 0)
    throw eckit::UserError("'varno' column not found", Here());

  struct LocationProperties {
    size_t index = 0;  // Location index
    size_t numChannels = 0;  // Current number of channels at that location
  };

  // Keeps track of the location to which the next row with each varno will be assigned.
  std::map<int, LocationProperties> nextLocationByVarno;

  RowsByLocation rowsByLocation;

  size_t previousSeqno = -1;
  for (size_t row = 0; row < numRows; row++) {
    const size_t seqno = static_cast<size_t>(sqlData.getData(row, seqnoColumnIndex));
    if (seqno != previousSeqno || rowsByLocation.empty()) {
      for (int varno : varnos)
        nextLocationByVarno[varno] = {rowsByLocation.size(), 0};
      previousSeqno = seqno;
    }
    const int varno = static_cast<int>(sqlData.getData(row, varnoColumnIndex));
    LocationProperties &nextLocation = nextLocationByVarno.at(varno);
    if (nextLocation.index == rowsByLocation.size())
      rowsByLocation.push_back({row});
    else
      rowsByLocation[nextLocation.index].push_back(row);
    ++nextLocation.numChannels;
    if (nextLocation.numChannels == maxNumChannels) {
      ++nextLocation.index;
      nextLocation.numChannels = 0;
    }
  }

  return rowsByLocation;
}

std::vector<std::string> RowsIntoLocationsSplitterBySeqno::defaultRecordIdColumns() const {
  return {"seqno"};
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
