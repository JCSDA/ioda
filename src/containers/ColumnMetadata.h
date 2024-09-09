/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_COLUMNMETADATA_H_
#define CONTAINERS_COLUMNMETADATA_H_

#include <cstdint>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Functions.h"

namespace osdf {
class ColumnMetadata {
 public:
  ColumnMetadata();

  const std::int8_t exists(const std::string&) const;
  const std::int32_t add(const ColumnMetadatum);
  const std::int32_t add(const std::vector<ColumnMetadatum>);

  const std::vector<ColumnMetadatum>& get() const;
  const ColumnMetadatum& get(const std::int32_t) const;

  void resetMaxId();
  void updateMaxId(const std::int64_t);
  void updateColumnWidth(const std::int32_t, const std::int16_t);
  void remove(const std::int32_t);

  const std::string& getName(const std::int32_t) const;
  const std::int8_t getType(const std::int32_t) const;
  const std::int8_t getPermission(const std::int32_t) const;
  const std::int32_t getIndex(const std::string&) const;
  const std::int32_t getSizeCols() const;
  const std::int64_t getMaxId() const;

  void print(const Functions&, const std::int32_t) const;
  void clear();

 private:
  std::int64_t maxId_;
  std::vector<ColumnMetadatum> columnMetadata_;
};
}  // namespace osdf

#endif  // CONTAINERS_COLUMNMETADATA_H_
