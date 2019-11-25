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

#include "oops/util/abor1_cpp.h"
#include "oops/util/missingValues.h"

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

  // -----------------------------------------------------------------------------
  /*!
   * \details This method will perform numeric data type conversions. The caller needs
   *          to allocate memory for the converted data (ToVar). This method is aware
   *          of the IODA missing values and will convert these appropriately. For
   *          example when converting double to float, all double missing values will
   *          be replaced with float missing values during the conversion.
   *
   * \param[in]  FromVar Vector of variable we are converting from
   * \param[out] ToVar   Vector of variable we are converting to
   * \param[in]  VarSize Total number of elements in FromVar and ToVar.
   */
  template<typename FromType, typename ToType>
  void ConvertVarType(const std::vector<FromType> & FromVar, std::vector<ToType> & ToVar) {
    std::string FromTypeName = TypeIdName(typeid(FromType));
    std::string ToTypeName = TypeIdName(typeid(ToType));
    const FromType FromMiss = util::missingValue(FromMiss);
    const ToType ToMiss = util::missingValue(ToMiss);

    // It is assumed that the caller has allocated memory for both input and output
    // variables.
    //
    // In any type change, the missing values need to be switched.
    //
    // Allow type changes between numeric types (int, float, double). These can
    // be handled with the standard conversions.
    bool FromTypeOkay = ((typeid(FromType) == typeid(int)) ||
                         (typeid(FromType) == typeid(float)) ||
                         (typeid(FromType) == typeid(double)));

    bool ToTypeOkay = ((typeid(ToType) == typeid(int)) ||
                       (typeid(ToType) == typeid(float)) ||
                       (typeid(ToType) == typeid(double)));

    if (FromTypeOkay && ToTypeOkay) {
      for (std::size_t i = 0; i < FromVar.size(); i++) {
        if (FromVar[i] == FromMiss) {
          ToVar[i] = ToMiss;
        } else {
          ToVar[i] = FromVar[i];
        }
      }
    } else {
      std::string ErrorMsg = "Unsupported variable data type conversion: " +
         FromTypeName + " to " + ToTypeName;
      ABORT(ErrorMsg);
    }
  }
}  // namespace ioda

#endif  // UTILS_IODAUTILS_H_
