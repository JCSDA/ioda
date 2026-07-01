/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FUNCTIONSROWS_H_
#define CONTAINERS_FUNCTIONSROWS_H_

#include <functional>
#include <memory>

#include "ioda/containers/DataRow.h"
#include "ioda/containers/DatumBase.h"
#include "ioda/containers/Functions.h"

namespace osdf {
class FunctionsRows : public Functions {
 public:
  FunctionsRows();

  FunctionsRows(FunctionsRows&&)                 = delete;
  FunctionsRows(const FunctionsRows&)            = delete;
  FunctionsRows& operator=(FunctionsRows&&)      = delete;
  FunctionsRows& operator=(const FunctionsRows&) = delete;

  template<typename T> const T getDatumValue(const std::shared_ptr<DatumBase>&) const;
  template<typename T> void setDatumValue(const std::shared_ptr<DatumBase>&, const T&) const;
};
}  // namespace osdf

#endif  // CONTAINERS_FUNCTIONSROWS_H_
