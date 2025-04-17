/*
 * (C) Crown copyright 2025, Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/OdbTablesRange.h"

namespace ioda {
namespace Engines {
namespace ODC {

OdbTablesIterator::OdbTablesIterator(const odc::core::ThreadSharedDataHandle &dh) : dh_(dh) {
  readNextTableIfExists();  // make the iterator point to the first table if one exists
}

void OdbTablesIterator::readNextTableIfExists() {
  // Inspired by odc::core::TablesReader::ensureTable.

  const eckit::Offset nextPosition = table_ == nullptr ? eckit::Offset(0) : table_->nextPosition();
  const eckit::Length estimatedLength = dh_.estimate();
  // Some DataHandles don't implement estimate() --> accept "0"
  ASSERT(nextPosition <= estimatedLength || estimatedLength == eckit::Length(0));

  // If the table has been truncated, this is an error, and we cannot read on.
  const eckit::Offset pos = dh_.seek(nextPosition);
  if (pos < nextPosition) {
      throw odc::core::ODBIncomplete(dh_.title(), Here());
  }

  // Will return a null pointer if the data source contains no more tables
  table_ = odc::core::Table::readTable(dh_);
}

OdbTablesRange::OdbTablesRange(std::unique_ptr<eckit::DataHandle> dh) : dh_(dh.release()) {}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
