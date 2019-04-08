/*
 * (C) Copyright 2018-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef UTILS_IODAUTILS_H_
#define UTILS_IODAUTILS_H_

#include <string>
#include <typeinfo>
#include <vector>

namespace ioda {

  // Utilities for converting back and forth between vector of strings and
  // a 2D character array.
  std::vector<std::size_t> CharShapeFromStringVector(
                              const std::vector<std::string> & StringVector);

  std::vector<std::string> CharArrayToStringVector(const char * CharData,
                              const std::vector<std::size_t> & CharShape);

  void StringVectorToCharArray(const std::vector<std::string> & StringVector,
                               const std::vector<std::size_t> & CharShape, char * CharData);

  std::string TypeIdName(const std::type_info & TypeId);

}  // namespace ioda

#endif  // UTILS_IODAUTILS_H_
