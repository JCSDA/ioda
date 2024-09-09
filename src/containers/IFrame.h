/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_IFRAME_H_
#define CONTAINERS_IFRAME_H_

#include <cstdint>
#include <string>
#include <vector>

namespace osdf {
class ColumnMetadatum;
class DataBase;
class DataRow;
class DatumBase;

class IFrame {
 public:
  IFrame() {}

  IFrame(IFrame&&)                 = delete;
  IFrame(const IFrame&)            = delete;
  IFrame& operator=(IFrame&&)      = delete;
  IFrame& operator=(const IFrame&) = delete;

  virtual void configColumns(const std::vector<ColumnMetadatum>) = 0;
  virtual void configColumns(const std::initializer_list<ColumnMetadatum>) = 0;

  virtual void appendNewColumn(const std::string&, const std::vector<std::int8_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::int16_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::int32_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::int64_t>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<float>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<double>&) = 0;
  virtual void appendNewColumn(const std::string&, const std::vector<std::string>&) = 0;

  virtual void getColumn(const std::string&, std::vector<std::int8_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int16_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int32_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int64_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<float>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<double>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::string>&) const = 0;

  virtual void setColumn(const std::string&, const std::vector<std::int8_t>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<std::int16_t>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<std::int32_t>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<std::int64_t>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<float>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<double>&) const = 0;
  virtual void setColumn(const std::string&, const std::vector<std::string>&) const = 0;

  virtual void removeColumn(const std::string&) = 0;
  virtual void removeColumn(const std::int32_t) = 0;
  virtual void removeRow(const std::int64_t) = 0;

  virtual void sortRows(const std::string&, const std::int8_t) = 0;
  virtual void print() const = 0;
  virtual void clear() = 0;
};

}  // namespace osdf

#endif  // CONTAINERS_IFRAME_H_
