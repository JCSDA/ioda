/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_VIEWROWSDATA_H_
#define CONTAINERS_VIEWROWSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsRows.h"
#include "ioda/containers/IRowsData.h"

namespace osdf {
class ViewRowsData : public IRowsData {
 public:
  explicit ViewRowsData(const Functions&, const ColumnMetadata&,
                        const std::vector<std::shared_ptr<DataRow>>&);

  ViewRowsData()                               = delete;
  ViewRowsData(ViewRowsData&&)                 = delete;
  ViewRowsData(const ViewRowsData&)            = delete;
  ViewRowsData& operator=(ViewRowsData&&)      = delete;
  ViewRowsData& operator=(const ViewRowsData&) = delete;

  const std::int32_t getSizeCols() const;
  const std::int64_t getSizeRows() const override;
  const std::int64_t getMaxId() const;

  const std::int32_t getIndex(const std::string&) const override;
  const std::string& getName(const std::int32_t) const;
  const std::int8_t getType(const std::int32_t) const;

  const std::int8_t columnExists(const std::string&) const;

  DataRow& getDataRow(const std::int64_t) override;
  const DataRow& getDataRow(const std::int64_t) const override;

  const ColumnMetadata& getColumnMetadata() const;
  const std::vector<std::shared_ptr<DataRow>>& getDataRows() const;

  void print() const;

 private:
  const Functions& funcs_;

  ColumnMetadata columnMetadata_;
  std::vector<std::shared_ptr<DataRow>> dataRows_;
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWROWSDATA_H_
