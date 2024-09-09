/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_IROWSDATA_H_
#define CONTAINERS_IROWSDATA_H_

#include <cstdint>
#include <string>

#include "ioda/containers/DataRow.h"

namespace osdf {
class IRowsData {
 public:
  IRowsData() {}

  IRowsData(IRowsData&&)                 = delete;
  IRowsData(const IRowsData&)            = delete;
  IRowsData& operator=(IRowsData&&)      = delete;
  IRowsData& operator=(const IRowsData&) = delete;

  virtual const std::int64_t getSizeRows() const = 0;

  virtual const std::int32_t getIndex(const std::string&) const = 0;

  virtual DataRow& getDataRow(const std::int64_t) = 0;
  virtual const DataRow& getDataRow(const std::int64_t) const = 0;
};
}  // namespace osdf

#endif  // CONTAINERS_IROWSDATA_H_
