/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_VIEWCOLSDATA_H_
#define CONTAINERS_VIEWCOLSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/IColsData.h"

namespace osdf {
class ViewColsData : public IColsData {
 public:
  explicit ViewColsData(const Functions&, const ColumnMetadata&, const std::vector<std::int64_t>&,
                        const std::vector<std::shared_ptr<DataBase>>&);

  ViewColsData()                               = delete;
  ViewColsData(ViewColsData&&)                 = delete;
  ViewColsData(const ViewColsData&)            = delete;
  ViewColsData& operator=(ViewColsData&&)      = delete;
  ViewColsData& operator=(const ViewColsData&) = delete;

  const std::int32_t getSizeCols() const override;
  const std::int64_t getSizeRows() const override;
  const std::int64_t getMaxId() const;

  const std::int32_t getIndex(const std::string&) const override;
  const std::string& getName(const std::int32_t) const;
  const std::int8_t getType(const std::int32_t) const;

  const std::int8_t columnExists(const std::string&) const;

  const std::shared_ptr<DataBase>& getDataColumn(const std::int32_t) const override;

  const ColumnMetadata& getColumnMetadata() const override;

  const std::vector<std::int64_t>& getIds() const override;

  const std::vector<std::shared_ptr<DataBase>>& getDataCols() const override;

  void print() const;

 private:
  const Functions& funcs_;

  ColumnMetadata columnMetadata_;
  std::vector<std::int64_t> ids_;
  std::vector<std::shared_ptr<DataBase>> dataColumns_;
};
}  // namespace osdf

#endif  // CONTAINERS_VIEWCOLSDATA_H_
