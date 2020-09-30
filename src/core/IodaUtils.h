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
#include <vector>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/missingValues.h"


namespace ioda {
  /// \brief typedef for holding dim names attached to variables
  typedef std::map<std::string, std::vector<std::string>> VarDimMap;

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
  /// \params groupName name of group
  /// \params varName name of variable
  std::string fullVarName(const std::string & groupName, const std::string & varName);

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

  /// \brief get maximum variable size along the first dimension
  Dimensions_t maxVarSize0(const Group & group);

  /// \brief get variable data type along the first dimension
  std::type_index varDtype(const Group & group, const std::string & varName);

  /// \brief true if variable is a dimension scale
  bool varIsDimScale(const Group & group, const std::string & varName);

  /// \brief convert reference, time to DateTime object
  /// \param refDtime reference date time
  /// \param timeOffets offset time values (in hours)
  std::vector<util::DateTime> convertRefOffsetToDtime(const int refIntDtime,
                                                      const std::vector<float> & timeOffsets);

  /// \brief convert datetime strings to DateTime object
  /// \param dtStrings datetime strings
  std::vector<util::DateTime> convertDtStringsToDtime(const std::vector<std::string> & dtStrings);

  /// \brief read string variable from an ObsFrameRead into a vector of strings
  /// \details The incoming variable could be a vector of strings, or a 2D array of
  /// string elements (strings of length 1). This routine will figure out which it is,
  /// read in the variable and if needed, convert to a vector of strings
  /// \param stringVar variable to be read
  /// \param feSelect selection object for stringVector (frontend)
  /// \param beSelect selection object for ObsFrame variable (backend)
  /// \param frameCount size (along first dimension) of section to read
  /// \param stringVector output string Vector
  void getFrameStringVar(const Variable & stringVar, const Selection feSelect,
                         const Selection beSelect, const Dimensions_t frameCount,
                         std::vector<std::string> & stringVector);

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

  /// \brief uniquify the output file name
  /// \details This function will tag on the MPI task number to the end of the file name
  /// to avoid collisions when running with multiple MPI tasks.
  /// \param fileName raw output file name
  /// \param rankNum MPI rank number
  std::string uniquifyFileName(const std::string & fileName, const std::size_t rankNum);

  /// \brief form a map containing lists of dimension variables that are attached to each
  /// variable
  /// \param varContainer Has_Variables object with variables to check
  /// \param varList list of regular variables
  /// \param dimVarList list of dimension scale variables
  VarDimMap genDimsAttachedToVars(const Has_Variables & varContainer,
                                  const std::vector<std::string> & varList,
                                  const std::vector<std::string> & dimVarList);

  /// \brief convert the new format varible name to the old format
  /// \param varName new format variable name
  std::string convertNewVnameToOldVname(const std::string & varName);

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
