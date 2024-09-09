/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMEROWSDATA_H_
#define CONTAINERS_FRAMEROWSDATA_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/FunctionsRows.h"
#include "ioda/containers/IFrameData.h"
#include "ioda/containers/IRowsData.h"

namespace osdf {
class FrameRowsData : public IFrameData, public IRowsData  {
 public:
  explicit FrameRowsData(const FunctionsRows&);
  explicit FrameRowsData(const FunctionsRows&, const ColumnMetadata&, const std::vector<DataRow>&);

  FrameRowsData()                                = delete;
  FrameRowsData(FrameRowsData&&)                 = delete;
  FrameRowsData(const FrameRowsData&)            = delete;
  FrameRowsData& operator=(FrameRowsData&&)      = delete;
  FrameRowsData& operator=(const FrameRowsData&) = delete;

  void configColumns(const std::initializer_list<ColumnMetadatum>);
  void configColumns(const std::vector<ColumnMetadatum>);

  void appendNewRow(const DataRow&);  // Equivalent function currently removed from template.
  // Adding column does not add any data as no assumption is made about row creation in this
  // row-priority data structure
  void appendNewColumn(const std::string&, const std::int8_t,
                       const std::int8_t = consts::eReadWrite);

  void removeColumn(const std::int32_t);
  void removeRow(const std::int64_t);

  void updateColumnWidth(const std::int32_t, const std::int16_t);

  const std::int32_t getSizeCols() const;
  const std::int64_t getSizeRows() const override;
  const std::int64_t getMaxId() const;

  const std::int32_t getIndex(const std::string&) const override;

  const std::string& getName(const std::int32_t) const override;
  const std::int8_t getType(const std::int32_t) const override;
  const std::int8_t getPermission(const std::int32_t) const override;

  const std::int8_t columnExists(const std::string&) const;

  DataRow& getDataRow(const std::int64_t) override;
  const DataRow& getDataRow(const std::int64_t) const override;

  const ColumnMetadata& getColumnMetadata() const;
  const std::vector<DataRow>& getDataRows() const;

  void initialise(const std::int64_t);

  void print() const;
  void clear();

 private:
  const FunctionsRows& funcs_;

  ColumnMetadata columnMetadata_;
  std::vector<DataRow> dataRows_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMEROWSDATA_H_
