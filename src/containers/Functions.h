/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FUNCTIONS_H_
#define CONTAINERS_FUNCTIONS_H_

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ioda/containers/DataBase.h"
#include "ioda/containers/DataRow.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/IFrameData.h"

namespace osdf {
class Functions {
 public:
  Functions();

  Functions(Functions&&)                 = delete;
  Functions(const Functions&)            = delete;
  Functions& operator=(Functions&&)      = delete;
  Functions& operator=(const Functions&) = delete;

  template<typename T> void addColumnToRow(IFrameData* data, DataRow&, std::int8_t&,
                                           std::int32_t&, const T) const;

  template<typename T> const std::shared_ptr<DataBase> createData(const std::vector<T>&) const;

  template<typename T> const std::shared_ptr<DatumBase> createDatum(const T) const;

  const std::int8_t compareDatums(const std::shared_ptr<DatumBase>&,
                                  const std::shared_ptr<DatumBase>&) const;

  template<typename T> const std::int8_t compareToThreshold(const std::int8_t,
                                                            const T, const T) const;

  template<typename T> const std::vector<T>& getDataValues(const std::shared_ptr<DataBase>&) const;
  template<typename T> std::vector<T>& getDataValues(std::shared_ptr<DataBase>&);

  const std::string padString(std::string, const std::int32_t) const;
};
}  // namespace osdf

#endif  // CONTAINERS_FUNCTIONS_H_
