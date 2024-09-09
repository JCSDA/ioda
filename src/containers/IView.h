/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_IVIEW_H_
#define CONTAINERS_IVIEW_H_

#include <cstdint>
#include <string>
#include <vector>

namespace osdf {
class IView {
 public:
  IView() {}

  IView(IView&&)                 = delete;
  IView(const IView&)            = delete;
  IView& operator=(IView&&)      = delete;
  IView& operator=(const IView&) = delete;

  virtual void getColumn(const std::string&, std::vector<std::int8_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int16_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int32_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int64_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<float>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<double>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::string>&) const = 0;

  virtual void print() const = 0;
};
}  // namespace osdf

#endif  // CONTAINERS_IVIEW_H_
