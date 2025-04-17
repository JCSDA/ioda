#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <vector>

#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"

#include "RowsByLocation.h"

namespace ioda {
namespace Engines {
namespace ODC {

class DataFromSQL;

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of an object splitting ODB rows into groups
/// associated with individual locations.
class RowsIntoLocationsSplitterParametersBase : public oops::Parameters {
  OOPS_ABSTRACT_PARAMETERS(RowsIntoLocationsSplitterParametersBase, Parameters)

 public:
  /// \brief Identifier of the row splitting method.
  oops::Parameter<std::string> method{"method", "by seqno", this};
};

// -----------------------------------------------------------------------------

/// \brief Splits rows loaded from an ODB file into groups associated with individual locations.
///
/// Each subclass needs to typedef `Parameters_` to the type of a subclass of
/// RowsIntoLocationsSplitterParametersBase storing its configuration options, and to provide a
/// constructor taking a reference to that class as its sole argument.
class RowsIntoLocationsSplitterBase {
public:
  virtual ~RowsIntoLocationsSplitterBase() {}

  /// \brief Identifies groups of ODB rows associated with individual locations.
  ///
  /// \param sqlData Data loaded from an ODB file.
  ///
  /// \returns A container whose ith element contains the set of indices of ODB rows associated
  /// with the ith location. The size of this container determines the number of locations created
  /// in the ObsSpace into which the ODB file is imported.
  ///
  /// \note Not all rows need to be associated with any location; those that are not will be
  /// ignored when importing data from the ODB file into an ObsSpace. In principle, rows may also
  /// be associated with more than one location, although this should rarely be needed in practice.
  virtual RowsByLocation groupRowsByLocation(const DataFromSQL &sqlData) const = 0;

  /// \brief Returns true if this splitter always assigns rows with different values in the `seqno`
  /// column to different locations, false otherwise.
  ///
  /// Subclasses in which this function returns false cannot be used when reading ODB files in
  /// parallel.
  virtual bool assignsRowsWithDifferentSeqnosToDifferentLocations() const = 0;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
