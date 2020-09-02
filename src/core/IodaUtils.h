/*
 * (C) Copyright 2018-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_IODAUTILS_H_
#define CORE_IODAUTILS_H_

#include <string>
#include <typeinfo>
#include <vector>

#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"

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

  std::size_t FindMaxStringLength(const std::vector<std::string> & StringVector);

  std::string TypeIdName(const std::type_info & TypeId);

  /// \brief find all variables in a Group and list their hierarchical names
  /// \param group Group for which all variables underneath this group will be listed
  /// \param varPath Hierarchical path to group (use "" for top level)
  std::vector<std::string> listAllVars(const Group & group, std::string varPath);

  /// \brief find the dimension scale variables in a Group and list their hierarchical names
  /// \param group Group for which variables underneath this group will be listed
  std::vector<std::string> listDimVars(const Group & group);

  /// \brief find the regular variables in a Group and list their hierarchical names
  /// \param group Group for which variables underneath this group will be listed
  std::vector<std::string> listVars(const Group & group);

  /// \brief get variable size along the first dimension
  Dimensions_t varSize0(const Group & group, const std::string & varName);

  /// \brief get variable data type along the first dimension
  std::type_index varDtype(const Group & group, const std::string & varName);

  /// \brief true if first dimension is nlocs dimension
  bool varIsDist(const Group & group, const std::string & varName);

  /// \brief true if variable is a dimension scale
  bool varIsDimScale(const Group & group, const std::string & varName);

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
          ToVar[i] = static_cast<ToType>(FromVar[i]);
        }
      }
    } else {
      std::string ErrorMsg = "Unsupported variable data type conversion: " +
         FromTypeName + " to " + ToTypeName;
      ABORT(ErrorMsg);
    }
  }
}  // namespace ioda

#endif  // CORE_IODAUTILS_H_
