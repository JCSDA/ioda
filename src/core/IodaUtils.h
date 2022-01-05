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

  /// \brief typedef for holding list of variable names with associated variable object
  typedef std::vector<std::pair<std::string, Variable>> VarNameObjectList;

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
  /// \param groupName name of group
  /// \param varName name of variable
  std::string fullVarName(const std::string & groupName, const std::string & varName);

  /// \brief collect variable and dimension information from a ioda ObsGroup
  /// \details It is assumed that the input ObsGroup has been populated. For example
  ///          you open an existing hdf5 file, and then call this routing to collect
  ///          the information. The information collected is passed back through the
  ///          output parameters (last 4 parameters) of this routine.
  ///
  ///          The reason for collecting all of this information in a single routine is
  ///          to handle severe performance issues with the HDF5 library when inspecting
  ///          an ObsGroup based on an HDF5 backend.
  ///
  /// \param obsGroup ioda ObsGroup object
  /// \param varObjectList list of regular variable names with associated Variable objects
  /// \param dimVarObjectList list of dimension variable names with associated Variable objects
  /// \param dimsAttachedToVars map structure holding list of dimension scale names attached
  ///        each regular variable
  /// \param maxVarSize0 maximum var length along the first (0th) dimension
  void collectVarDimInfo(const ObsGroup & obsGroup, VarNameObjectList & varObjectList,
                         VarNameObjectList & dimVarObjectList, VarDimMap & dimsAttachedToVars,
                         Dimensions_t & maxVarSize0);

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
  /// \param newEpoch DateTime object used for the epoch if creating a new variable
  /// \param epochDtVar requested datetime variable
  /// \param destVarContainer Has_Variables object in which to open/create the variable
  void openCreateEpochDtimeVar(const std::string & groupName, const std::string & varName,
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

  /// \brief uniquify the output file name
  /// \details This function will tag on the MPI task number to the end of the file name
  /// to avoid collisions when running with multiple MPI tasks.
  /// \param fileName raw output file name
  /// \param rankNum MPI group communicator rank number
  /// \param timeRankNum MPI time communicator rank number
  std::string uniquifyFileName(const std::string & fileName, const std::size_t rankNum,
                               const int timeRankNum);

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
    const FromType FromMiss = util::missingValue(FromMiss);
    const ToType ToMiss = util::missingValue(ToMiss);

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

  /// \brief A function object that can be passed to the third parameter of
  /// forAnySupportedAttributeType() to throw an exception if the attribute
  /// is of an unsupported type.
  class ThrowIfAttributeIsOfUnsupportedType {
   public:
    explicit ThrowIfAttributeIsOfUnsupportedType(
        const std::string &attrName) : attrName_(attrName) {}

    void operator()(const ioda::source_location & codeLocation) const {
      std::string ErrorMsg = std::string("Attribute '") + attrName_ +
                             std::string("' is not of any supported type");
      throw ioda::Exception(ErrorMsg.c_str(), codeLocation);
    }

   private:
    std::string attrName_;
  };

  /// \brief Perform an action dependent on the type of an attribute \p var.
  ///
  /// \param attr
  ///   Attribute expected to be of one of the types that can be stored in an ObsSpace
  ///   variable (`int`, `int64_t`, `float`, `double`, `std::string` or `char`).
  /// \param action
  ///   A function object callable with a single argument of any type from the list above.
  ///   If the attribute `attr` is of type `int`, this function will be given a
  ///   default-initialized `int`; if it is of type `float` the function will be given a
  ///   default-initialized `float`, and so on. In practice, it is likely to be a generic
  ///   lambda expression taking a single parameter of type auto whose value is ignored,
  ///   but whose type is used in the implementation.
  /// \param errorHandler
  ///   A function object callable with a single argument of type eckit::CodeLocation,
  ///   called if `var` is not of a type that can be stored in an ObsSpace.
  ///
  /// Example of use:
  ///
  ///     Attribute sourceAttr = ...;
  ///     std::string attrName = ...;
  ///     Dimensions_t attrDims = sourceAttr.getDimensions().dimsCur;
  ///     forAnySupportedAttributeType(
  ///       sourceAttr,
  ///       [&](auto typeDiscriminator) {
  ///           typedef decltype(typeDiscriminator) T;  // type of the variable sourceVar
  ///           obs_frame_.atts.create<T>(attrName, attrDims);
  ///       },
  ///       ThrowIfAttributeIsOfUnsupportedType(attrName));
  template <typename Action, typename ErrorHandler>
  auto forAnySupportedAttributeType(const ioda::Attribute &attr, const Action &action,
                                    const ErrorHandler &typeErrorHandler) {
    if (attr.isA<int>())
      return action(int());
    if (attr.isA<long>())          // NOLINT
      return action(long());       // NOLINT
    if (attr.isA<float>())
      return action(float());
    if (attr.isA<double>())
      return action(double());
    if (attr.isA<std::string>())
      return action(std::string());
    if (attr.isA<char>())
      return action(char());
    typeErrorHandler(ioda_Here());
  }

  /// \brief Copy attributes from one object to another.
  /// \details This function will copy the attributes from the source attribute container
  /// to the destination container.
  /// \param srcAttrs source Has_Attributes container
  /// \param destAttrs destination Has_Attributes container
  void copyAttributes(const ioda::Has_Attributes & srcAttrs, ioda::Has_Attributes & destAttrs);

  /// \brief A function object that can be passed to the third parameter of
  /// forAnySupportedVariableType() or switchOnVariableType() to throw an exception
  /// if the variable is of an unsupported type.
  class ThrowIfVariableIsOfUnsupportedType {
   public:
    explicit ThrowIfVariableIsOfUnsupportedType(
        const std::string &varName) : varName_(varName) {}

    void operator()(const ioda::source_location & codeLocation) const {
      std::string ErrorMsg = std::string("Variable '") + varName_ +
                             std::string("' is not of any supported type");
      throw ioda::Exception(ErrorMsg.c_str(), codeLocation);
    }

   private:
    std::string varName_;
  };

  /// \brief Perform an action dependent on the type of an ObsSpace variable \p var.
  ///
  /// \param var
  ///   Variable expected to be of one of the types that can be stored in an ObsSpace (`int`,
  ///   `int64_t`, `float`, `std::string` or `char`).
  /// \param action
  ///   A function object callable with a single argument of any type from the list above.
  ///   If the variable `var` is of type `int`, this function will be given a default-initialized
  ///   `int`; if it is of type `float` the function will be given a default-initialized `float`,
  ///   and so on. In practice, it is likely to be a generic lambda expression taking a single
  ///   parameter of type auto whose value is ignored, but whose type is used in the implementation.
  /// \param errorHandler
  ///   A function object callable with a single argument of type eckit::CodeLocation, called if
  ///   `var` is not of a type that can be stored in an ObsSpace.
  ///
  /// Example of use:
  ///
  ///     Variable sourceVar = ...;
  ///     std::string varName = ...;
  ///     std::vector<Variable> dimVars = ...;
  ///     forAnySupportedVariableType(
  ///       sourceVar,
  ///       [&](auto typeDiscriminator) {
  ///           typedef decltype(typeDiscriminator) T;  // type of the variable sourceVar
  ///           obs_frame_.vars.createWithScales<T>(varName, dimVars, VariableCreationParameters());
  ///       },
  ///       ThrowIfVariableIsOfUnsupportedType(varName));
  template <typename Action, typename ErrorHandler>
  auto forAnySupportedVariableType(const ioda::Variable &var, const Action &action,
                                   const ErrorHandler &typeErrorHandler) {
    if (var.isA<int>())
      return action(int());
    if (var.isA<int64_t>())
      return action(int64_t());
    if (var.isA<float>())
      return action(float());
    if (var.isA<std::string>())
      return action(std::string());
    if (var.isA<char>())
      return action(char());
    typeErrorHandler(ioda_Here());
  }

  /// \brief Perform an action dependent on the type of an ObsSpace variable \p var.
  ///
  /// \param var
  ///   Variable expected to be of one of the types that can be stored in an ObsSpace (`int`,
  ///   `int64_t`, `float`, `std::string` or `char`).
  /// \param intAction
  ///   A function object taking an argument of type `int`, which will be called and given a
  ///   default-initialized `int` if the variable `var` is of type `int`.
  /// \param int64Action
  ///   A function object taking an argument of type `int64_t`, which will be called and given a
  ///   default-initialized `int64_t` if the variable `var` is of type `int64_t`.
  /// \param floatAction
  ///   A function object taking an argument of type `float`, which will be called and given a
  ///   default-initialized `float` if the variable `var` is of type `float`.
  /// \param stringAction
  ///   A function object taking an argument of type `std::string`, which will be called and given a
  ///   default-initialized `std:::string` if the variable `var` is of type `std::string`.
  /// \param charAction
  ///   A function object taking an argument of type `char`, which will be called and given a
  ///   default-initialized `char` if the variable `var` is of type `char`.
  /// \param errorHandler
  ///   A function object callable with a single argument of type eckit::CodeLocation, called if
  ///   `var` is not of a type that can be stored in an ObsSpace.
  template <typename IntAction, typename Int64Action, typename FloatAction,
            typename StringAction, typename CharAction, typename ErrorHandler>
  auto switchOnSupportedVariableType(const ioda::Variable &var,
                                     const IntAction &intAction,
                                     const Int64Action &int64Action,
                                     const FloatAction &floatAction,
                                     const StringAction &stringAction,
                                     const CharAction &charAction,
                                     const ErrorHandler &typeErrorHandler) {
    if (var.isA<int>())
      return intAction(int());
    if (var.isA<int64_t>())
      return int64Action(int64_t());
    if (var.isA<float>())
      return floatAction(float());
    if (var.isA<std::string>())
      return stringAction(std::string());
    if (var.isA<char>())
      return charAction(char());
    typeErrorHandler(ioda_Here());
  }

  /// \brief Perform a variable-type-dependent action for all types that can be stored in an
  /// ObsSpace (`int`, `int64_t`, `float`, `std::string` or `char`).
  ///
  /// \param action
  ///   A function object callable with a single argument of any type from the list above. It will
  ///   be called as many times as there are types of variables that can be stored in an ObsSpace;
  ///   each time it will received a default-initialized value of that type. In practice, it is
  ///   likely to be a generic lambda expression taking a single parameter of type auto whose value
  ///   is ignored, but whose type is used in the implementation.
  template <typename Action>
  void forEachSupportedVariableType(const Action &action) {
    action(int());
    action(int64_t());
    action(float());
    action(std::string());
    action(char());
  }
}  // namespace ioda

#endif  // CORE_IODAUTILS_H_
