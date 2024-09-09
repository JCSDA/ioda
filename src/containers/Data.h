/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_DATA_H_
#define CONTAINERS_DATA_H_

#include <string>
#include <vector>

#include "ioda/containers/DataBase.h"

namespace osdf {
template<class T>
class Data : public DataBase {
 public:
  explicit Data(const std::vector<T>&);

  virtual ~Data() = default;

  Data()                       = delete;
  Data(Data&&)                 = delete;
  Data(const Data&)            = delete;
  Data& operator=(Data&&)      = delete;
  Data& operator=(const Data&) = delete;

  const std::vector<T>& getValues() const {
    return values_;
  }

  std::vector<T>& getValues() {
    return values_;
  }

  const std::string getValueStr(const std::int64_t) const;

  void setValue(const std::int64_t rowIndex, const T value) {
    values_.at(static_cast<std::size_t>(rowIndex)) = value;
  }

  void addValue(const T value) {
    values_.push_back(value);
  }

  void setValues(const std::vector<T> values) {
    values_ = values;
  }

  void removeValue(const std::int64_t rowIndex) {
    values_.erase(std::next(values_.begin(), rowIndex));
  }

  void reserve(std::int64_t size) {
    values_.reserve(static_cast<std::size_t>(size));
  }

  void clear() {
    values_.clear();
  }

 private:
  std::vector<T> values_;
};
}  // namespace osdf

#endif  // CONTAINERS_DATA_H_
