/*
 * (C) Copyright 2018-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_IODAUTILS_H_
#define CORE_IODAUTILS_H_

#include <map>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Exception.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/missingValues.h"


namespace ioda {
  class ObsSpaceParameters;

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

  /// \brief form full variable name given individual group and variable names
  /// \details This routine will form the consistent frontend variable name according
  //  to the layout policy. This is defined as "groupName/varName".
  /// \param groupName name of group
  /// \param varName name of variable
  std::string fullVarName(const std::string & groupName, const std::string & varName);

  /// \brief get variable data type
  std::type_index varDtype(const Group & group, const std::string & varName);

  /// \brief true if variable is a dimension scale
  bool varIsDimScale(const Group & group, const std::string & varName);

  /// \brief transform variable's units epoch string to an epoch DateTime object
  /// \param dtVar input epoch style datetime variable
  util::DateTime getEpochAsDtime(const Variable & dtVar);

  /// \brief open or create an epoch style datetime variable
  /// \param groupName name of group in which to open or create variable
  /// \param varName name of variable to open or create
  /// \param globalNlocs total number of locations across all MPI tasks
  /// \param newEpoch DateTime object used for the epoch if creating a new variable
  /// \param epochDtVar requested datetime variable
  /// \param destVarContainer Has_Variables object in which to open/create the variable
  void openCreateEpochDtimeVar(const std::string & groupName, const std::string & varName,
                               const std::size_t globalNlocs,
                               const util::DateTime & newEpoch, Variable & epochDtVar,
                               Has_Variables & destVarContainer);

  /// \brief convert datetime strings to DateTime objects
  /// \param dtStrings datetime strings
  std::vector<util::DateTime> convertDtStringsToDtime(const std::vector<std::string> & dtStrings);

  /// \brief convert epoch datetimes to DateTime objects
  /// \param epochDtime datetime object holding the epoch datetime value
  /// \param timeOffsets int64_t vector holding the time offsets in seconds from epochDtime
  std::vector<util::DateTime> convertEpochDtToDtime(const util::DateTime epochDtime,
                                                    const std::vector<int64_t> & timeOffsets);

  /// \brief convert DateTime objects to epoch time offsets
  /// \param epochDtime datetime object holding the epoch datetime value
  /// \param dtimes vector of DateTime objects
  std::vector<int64_t> convertDtimeToTimeOffsets(const util::DateTime epochDtime,
                                                 const std::vector<util::DateTime> & dtimes);

  /// \brief convert datetime strings to epoch time offsets
  /// \param epochDtime datetime object holding the epoch datetime value
  /// \param dtStrings vector of datetime strings
  std::vector<int64_t> convertDtStringsToTimeOffsets(const util::DateTime epochDtime,
                                                 const std::vector<std::string> & dtStrings);

  /// \brief convert 2D string array to a vector of strings
  /// \details The incoming 2D strings array is passed in through the arrayData argument
  /// as a flattened vector. The arrayShape arugument shows how to un-flatten the arrayData.
  /// \param arrayData Flattened data for input 2D string array
  /// \param arrayShape Shape of input arrayData
  std::vector<std::string> StringArrayToStringVector(
                              const std::vector<std::string> & arrayData,
                              const std::vector<Dimensions_t> & arrayShape);

  /// \brief set params for output file construction from test YAML configuration
  /// \details This routine is intended for use by the ObsIo and ObsFrame tests. It
  /// relies on test confiruation "test data.write dimensions" and
  /// "test data.write variables" accompanying an "obsdataout.obsfile" spec in the
  /// YAML configuration.
  /// \param obsConfig test YAML configuration for and ObsSpace
  /// \param obsParams output params
  void setOfileParamsFromTestConfig(const eckit::LocalConfiguration & obsConfig,
                                    ioda::ObsSpaceParameters & obsParams);

  /// \brief convert the new format varible name to the old format
  /// \param varName new format variable name
  std::string convertNewVnameToOldVname(const std::string & varName);

  // -----------------------------------------------------------------------------
  /*!
   * \details This method will perform numeric data type conversions. This method is aware
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
    ToVar.resize(FromVar.size());

    std::string FromTypeName = TypeIdName(typeid(FromType));
    std::string ToTypeName = TypeIdName(typeid(ToType));
    const FromType FromMiss = util::missingValue<FromType>();
    const ToType ToMiss = util::missingValue<ToType>();

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
