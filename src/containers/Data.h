/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>

#include "ioda/containers/DataBase.h"

namespace osdf {
template<class T>
class Data : public DataBase {
 public:
  explicit Data(const std::vector<T>&);

  virtual ~Data() = default;

  Data()                        = delete;  //!< Deleted default constructor
  Data(Data&&)                  = delete;  //!< Deleted move constructor
  Data(const Data&)             = delete;  //!< Deleted copy constructor
  Data& operator=(Data&&)       = delete;  //!< Deleted move assignment
  Data& operator=(const Data&)  = delete;  //!< Deleted copy assignment

  const std::vector<T>& getData() const {
    return values_;
  }

  std::vector<T>& getData() {
    return values_;
  }

  const std::string getDatumStr(const std::int64_t) const;

  void setDatum(const std::int64_t rowIndex, const T value) {
    values_.at(rowIndex) = value;
  }

  void addDatum(const T value) {
    values_.push_back(value);
  }

  void setData(const std::vector<T> values) {
    values_ = values;
    size_ = values_.size();
  }

  void removeDatum(const std::int64_t rowIndex) {
    values_.erase(std::next(values_.begin(), rowIndex));
    size_--;
  }

 private:
  std::vector<T> values_;
};
}  // namespace osdf

#endif  // DATA_H
