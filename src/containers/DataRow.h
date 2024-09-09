/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_DATAROW_H_
#define CONTAINERS_DATAROW_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "ioda/containers/DatumBase.h"

namespace osdf {
class Functions;
class ColumnMetadata;

class DataRow {
 public:
  explicit DataRow(std::int64_t);

  DataRow() = delete;

  const std::int64_t getId() const;
  const std::int32_t getSize() const;

  const std::shared_ptr<DatumBase>& getColumn(const std::int32_t) const;
  std::shared_ptr<DatumBase>& getColumn(const std::int32_t);

  void insert(const std::shared_ptr<DatumBase>&);
  void remove(const std::int32_t);

  void print(const Functions&, const ColumnMetadata&, const std::int32_t) const;
  void clear();

 private:
  std::vector<std::shared_ptr<DatumBase>> dataColumns_;
  std::int64_t id_;
};
}  // namespace osdf

#endif  // CONTAINERS_DATAROW_H_
