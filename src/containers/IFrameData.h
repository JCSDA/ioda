/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_IFRAMEDATA_H_
#define CONTAINERS_IFRAMEDATA_H_

#include <cstdint>
#include <string>
#include <vector>
#include "DataRow.h"

namespace osdf {

class ColumnMetadata;

/// \class IFrameData
/// This pure abstract class is currently used to enable common operations on data in containers to
/// be handled by a single function. This approach reduces code repetition and enhances
/// maintainability. By passing pointers to an IFrameData class into functions, it doesn't care
/// about the type or underlying structure of the data and is able to determine what it needs from
/// the functions this interface implements.
/// \see Functions::addColumnToRow

class IFrameData {
 public:
  IFrameData() {}

  IFrameData(IFrameData&&)                 = delete;
  IFrameData(const IFrameData&)            = delete;
  IFrameData& operator=(IFrameData&&)      = delete;
  IFrameData& operator=(const IFrameData&) = delete;

  virtual const std::string& getName(const std::int32_t) const = 0;
  virtual const std::string& getUnits(const std::int32_t) const = 0;
  virtual const std::int8_t getType(const std::int32_t) const = 0;
  virtual const std::int8_t getPermission(const std::int32_t) const = 0;

  virtual const ColumnMetadata& getColumnMetadata() const = 0;
  virtual const std::int64_t getSizeRows() const          = 0;
  virtual void getDataRows(std::vector<DataRow>& dataRowsContainer) const = 0;
  virtual const std::int64_t getMaxId() const = 0;
};
}  // namespace osdf

#endif  // CONTAINERS_IFRAMEDATA_H_
