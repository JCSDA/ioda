#pragma once
/*
 * (C) Crown copyright 2025, Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <memory>

#include "ioda/defs.h"
#include "odc/core/Table.h"

namespace ioda {
namespace Engines {
namespace ODC {

class OdbTablesSentinel {};

/// \brief Iterator over ODB tables in a data source (e.g. an ODB file or part of a file).
///
/// Client code does not normally need to use this class directly. Use OdbTablesRange (defined
/// below) instead.
class OdbTablesIterator {
 public:
  explicit OdbTablesIterator(const odc::core::ThreadSharedDataHandle &dh);

  const odc::core::Table & operator*() const {
    ASSERT(table_);
    return *table_;
  }

  const odc::core::Table * operator->() const {
    ASSERT(table_);
    return table_.get();
  }

  OdbTablesIterator &operator++() {
    readNextTableIfExists();
    return *this;
  }

  bool operator==(OdbTablesSentinel) const {
    return table_ == nullptr;
  }

  bool operator!=(OdbTablesSentinel) const {
    return !operator==(OdbTablesSentinel());
  }

 private:
  void readNextTableIfExists();

 private:
  odc::core::ThreadSharedDataHandle dh_;
  std::unique_ptr<odc::core::Table> table_;
};

/// \brief Range of ODB tables present in a data source (e.g. an ODB file or part of it).
///
/// This class is meant to be used within range-based `for` loops, as in the example below:
///
///     std::unique_ptr<eckit::DataHandle> dh = ...;
///     for (const odc::core::Table &table : OdbTablesRange(std::move(dh))) {
///         // process `table`
///     }
///
/// This is an alternative to
///
///     odc::api::Reader reader(filename, false /*aggregated?*/);
///     while (odc::api::Frame frame = reader.next()) {
///         // process `frame` (which wraps a single ODB table)
///     }
///
/// which uses more memory because a Reader object stores a vector of Table objects representing all
/// ODB tables that have been read, whereas an OdbTablesIterator created by an OdbTablesRange
/// keeps in memory only a single Table object at any given time.
class OdbTablesRange {
 public:
  /// \brief Constructor.
  ///
  /// Opens `dh` for reading.
  explicit OdbTablesRange(std::unique_ptr<eckit::DataHandle> dh);

  OdbTablesIterator begin() { return OdbTablesIterator(dh_); }
  OdbTablesSentinel end() { return OdbTablesSentinel(); }

 private:
  odc::core::ThreadSharedDataHandle dh_;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
