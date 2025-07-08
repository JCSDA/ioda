#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

/// \file RowsIntoLocationsSplitter.h
/// Concrete implementations of the RowsIntoLocationsSplitterBase interface.

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "RowsIntoLocationsSplitterBase.h"

namespace ioda {
namespace Engines {
namespace ODC {

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of
/// RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno.
class RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarnoParameters :
    public RowsIntoLocationsSplitterParametersBase {
  OOPS_CONCRETE_PARAMETERS(
      RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarnoParameters,
      RowsIntoLocationsSplitterParametersBase)
public:
  /// \brief If set to true and the number of locations associated with a given `seqno` exceeds the
  /// number of levels loaded from the `numlev` column in the first row with that `seqno`, only the
  /// first `numlev` locations will be kept, and the rest will be discarded.
  oops::Parameter<bool> keepOnlyReportedLevels{"keep only reported levels", false, this};
};

/// \brief Splits rows by `seqno`, then assigns each row in which any `varno` occurs for the
/// nth time in the set of rows with that `seqno` to the nth location associated with that `seqno`.
///
/// For example, given the following data loaded from an ODB file,
///
/// row | seqno | varno
/// --- | ----- | -----
///   0 |     1 |     2
///   1 |     1 |     6
///   2 |     1 |     2
///   3 |     1 |     6
///   4 |     2 |     6
///   5 |     2 |     6
///   6 |     2 |     6
///   7 |     2 |     2
///   8 |     2 |     2
///   9 |     2 |     2
///
/// the rows will be assigned to the following locations:
///
/// row | location
/// --- | --------
///   0 |        0
///   1 |        0
///   2 |        1
///   3 |        1
///   4 |        2
///   5 |        3
///   6 |        4
///   7 |        2
///   8 |        3
///   9 |        4
///
/// If the `keep only reported levels` option is selected and the number of locations associated
/// with a given `seqno` exceeds the number of levels loaded from the `numlev` column in the first
/// row with that `seqno`, only the first `numlev` locations will be kept, and the rest will be
/// discarded.
///
/// \note The current implementation expects rows associated with the same `seqno` to have
/// consecutive indices. Non-consecutive ranges of rows with the same `seqno` are treated as if they
/// had different `seqno`s.
class RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno :
    public RowsIntoLocationsSplitterBase {
public:
  typedef RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarnoParameters Parameters_;

  explicit RowsIntoLocationsSplitterBySeqnoThenByCounterOfRowsWithVarno(const Parameters_ &);

  RowsByLocation groupRowsByLocation(const DataFromSQL &sqlData) const override;

  std::vector<std::string> defaultRecordIdColumns() const override;

private:
  Parameters_ parameters_;
};

// -----------------------------------------------------------------------------

/// \brief Parameters controlling the behavior of RowsIntoLocationsSplitterBySeqno.
class RowsIntoLocationsSplitterBySeqnoParameters :
    public RowsIntoLocationsSplitterParametersBase {
  OOPS_CONCRETE_PARAMETERS(RowsIntoLocationsSplitterBySeqnoParameters,
                           RowsIntoLocationsSplitterParametersBase)
public:
  /// The maximum number of rows with the same `seqno` and `varno` that may be assigned to the same
  /// location.
  oops::Parameter<size_t> maxNumChannels{"maximum number of channels",
                                         std::numeric_limits<size_t>::max(), this};
};

/// \brief Splits rows by `seqno`.
///
/// If the `maximum number of channels` option is set, sets of rows with the same `seqno` are split
/// further until none of them contains more than `maximum number of channels` rows with the same
/// `varno`.
///
/// For example, if the following data are loaded from an ODB file,
///
/// row | seqno | varno
/// --- | ----- | -----
///   0 |     1 |     2
///   1 |     1 |     6
///   2 |     1 |     2
///   3 |     1 |     6
///   4 |     2 |     6
///   5 |     2 |     6
///   6 |     2 |     6
///   7 |     2 |     2
///   8 |     2 |     2
///   9 |     2 |     2
///
/// and the `maximum number of channels` option is not set, the rows will be assigned to the
/// following locations:
///
/// row | location
/// --- | --------
///   0 |        0
///   1 |        0
///   2 |        0
///   3 |        0
///   4 |        1
///   5 |        1
///   6 |        1
///   7 |        1
///   8 |        1
///   9 |        1
///
/// If the `maximum number of channels` option is set to 2, the assignment will be different:
///
/// row | location
/// --- | --------
///   0 |        0
///   1 |        0
///   2 |        0
///   3 |        0
///   4 |        1
///   5 |        1
///   6 |        2
///   7 |        1
///   8 |        1
///   9 |        2
class RowsIntoLocationsSplitterBySeqno : public RowsIntoLocationsSplitterBase {
public:
  typedef RowsIntoLocationsSplitterBySeqnoParameters Parameters_;

  explicit RowsIntoLocationsSplitterBySeqno(const Parameters_ &parameters);

  RowsByLocation groupRowsByLocation(const DataFromSQL &sqlData) const override;

  std::vector<std::string> defaultRecordIdColumns() const override;

private:
  /// Faster and simpler version used when there's no constraint on the maximum number of channels.
  RowsByLocation groupRowsByLocationWithoutConstraints(const DataFromSQL &sqlData) const;
  RowsByLocation groupRowsByLocationWithMaxNumChannelsConstraint(const DataFromSQL &sqlData) const;

private:
  Parameters_ parameters_;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
