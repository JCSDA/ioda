/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FUNCTIONSCOLS_H_
#define CONTAINERS_FUNCTIONSCOLS_H_

#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/ColumnMetadata.h"
#include "ioda/containers/DataBase.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/Functions.h"
#include "ioda/containers/IColsData.h"

namespace osdf {
class FrameColsData;

class FunctionsCols : public Functions {
 public:
  FunctionsCols();

  FunctionsCols(FunctionsCols&&)                 = delete;
  FunctionsCols(const FunctionsCols&)            = delete;
  FunctionsCols& operator=(FunctionsCols&&)      = delete;
  FunctionsCols& operator=(const FunctionsCols&) = delete;

  template<typename T> void addDatumValue(const std::shared_ptr<DataBase>&,
                                          const std::shared_ptr<DatumBase>&) const;

  template<typename T> void setDataValues(const std::shared_ptr<DataBase>&,
                                          const std::vector<T>&) const;

  template<typename T> void removeDatum(std::shared_ptr<DataBase>&, const std::int64_t) const;

  template<typename T> void sequenceIndices(std::vector<std::int64_t>&, const std::vector<T>&,
                                            const std::int8_t) const;

  // Takes a copy of indices by design - the input order is required for subsequent calls
  template<typename T> void reorderValues(std::vector<std::int64_t>, std::vector<T>&) const;

  template<typename T> const std::vector<T> getSlicedValues(const std::vector<T>&,
                                                            const std::vector<std::int64_t>&) const;

  template<typename T> void sliceRows(const osdf::IColsData*,
    std::vector<std::shared_ptr<DataBase>>&, ColumnMetadata&, std::vector<std::int64_t>&,
    const std::string&, const std::int8_t, const T) const;

  template<typename T> void sliceData(std::vector<std::shared_ptr<DataBase>>&,
                       const std::shared_ptr<DataBase>&, const std::vector<std::int64_t>&) const;

  template<typename T> void addValueToData(std::vector<std::shared_ptr<osdf::DataBase>>&,
                                           const std::shared_ptr<DatumBase>&, const std::int8_t,
                                           const std::int64_t, const std::int32_t) const;

  template<typename T> void clearData(std::shared_ptr<DataBase>&) const;

  template<typename T> const std::int16_t getSize(const std::shared_ptr<DataBase>&,
                                                  const std::int64_t = 0) const;
};
}  // namespace osdf

#endif  // CONTAINERS_FUNCTIONSCOLS_H_
