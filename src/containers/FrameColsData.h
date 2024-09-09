/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMECOLSDATA_H_
#define CONTAINERS_FRAMECOLSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsCols.h"
#include "ioda/containers/IColsData.h"
#include "ioda/containers/IFrameData.h"

namespace osdf {
class FrameColsData : public IFrameData, public IColsData {
 public:
  explicit FrameColsData(const FunctionsCols&, const ColumnMetadata&,
           const std::vector<std::int64_t>&, const std::vector<std::shared_ptr<DataBase>>&);
  explicit FrameColsData(const FunctionsCols&);

  FrameColsData()                                = delete;
  FrameColsData(FrameColsData&&)                 = delete;
  FrameColsData(const FrameColsData&)            = delete;
  FrameColsData& operator=(FrameColsData&&)      = delete;
  FrameColsData& operator=(const FrameColsData&) = delete;

  void configColumns(const std::vector<ColumnMetadatum>);
  void configColumns(const std::initializer_list<ColumnMetadatum>);

  void appendNewRow(const DataRow&);
  void appendNewColumn(const std::shared_ptr<DataBase>&, const std::string&, const std::int8_t,
                       const std::int8_t = consts::eReadWrite);

  void removeColumn(const std::int32_t);
  void removeRow(const std::int64_t);

  void updateMaxId(const std::int64_t);
  void updateColumnWidth(const std::int32_t, const std::int16_t);

  const std::int32_t getSizeCols() const override;
  const std::int64_t getSizeRows() const override;
  const std::int64_t getMaxId() const;

  const std::int32_t getIndex(const std::string&) const override;

  const std::string& getName(const std::int32_t) const override;
  const std::int8_t getType(const std::int32_t) const override;
  const std::int8_t getPermission(const std::int32_t) const override;

  const std::int8_t columnExists(const std::string&) const;

  std::vector<std::int64_t>& getIds();
  const std::vector<std::int64_t>& getIds() const;

  std::shared_ptr<DataBase>& getDataColumn(const std::int32_t);
  const std::shared_ptr<DataBase>& getDataColumn(const std::int32_t) const override;

  ColumnMetadata& getColumnMetadata();
  const ColumnMetadata& getColumnMetadata() const override;

  std::vector<std::shared_ptr<DataBase>>& getDataCols();
  const std::vector<std::shared_ptr<DataBase>>& getDataCols() const;

  void initialise(const std::int64_t);

  void print() const;
  void clear();

 private:
  const FunctionsCols& funcs_;

  ColumnMetadata columnMetadata_;
  std::vector<std::int64_t> ids_;
  std::vector<std::shared_ptr<DataBase>> dataColumns_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMECOLSDATA_H_
