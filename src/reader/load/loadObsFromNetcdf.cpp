/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/load/loadObsFromNetcdf.hpp"

#include <netcdf>

#include <cmath>
#include <map>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/ObsDataIoParameters.h"

#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace reader {

//--------------------------------------------------------------------------------
// Function declarations for "private" functions
//--------------------------------------------------------------------------------

/// \brief check if a netCDF variable is a dimension
/// \param group netCDF group
/// \param varName name of the variable to check
static bool isNetcdfVarADimension(const netCDF::NcGroup & group, const std::string & varName);

/// \brief check if we can keep a variable for loading into the OSDF container
/// \details The OSDF container accepts any 1D variable (dimensioned by Location or by any
/// slice dimension), and any 2D variable whose first dimension is Location (the second
/// dimension may have any name). This function checks for this and returns true if the
/// variable can be loaded into the OSDF.
/// \param varName hierarchical variable name
/// \param var netCDF variable to check
/// \param varDimNames list in order of dimension names for the variable
static bool keepNetcdfVarForOSDF(const std::string & varName,
                                 const netCDF::NcVar & var,
                                 std::vector<std::string> & varDimNames);

/// \brief list out variable names in a recursive fashion
/// \details This function will traverse the group hierarchy and return a vector
/// of strings containing all of the variable names under the given group.
/// The variable names will have the group path prepended to them in order
/// to keep all the variable names unique, and to denote where they live in
/// the group hierarchy.
///
/// The listDimensions flag indicates if the function should list the
/// dimensions instead of the variables. If false list variables, if true
/// list dimensions.
/// \param group netCDF group
/// \param groupPathPrefix leading group path for this level of the heirarchy
/// \param listDimensions flag to indicate listing dimensions or variables
static std::vector<std::string> listAllNetcdfVars(const netCDF::NcGroup& group,
                                    const std::string & groupPathPrefix,
                                    const bool listDimensions);

/// \brief load the variable (specified as a hierarchical path) from the netCDF file
/// \details This function allows variable names with hierarchical paths to be
/// loaded from the netCDF file.  The variable name is specified as a vector
/// of strings, where the last entry is the variable and any leading entries are
/// the group names leading to the variable.
/// \param topGroup top level group object corresponding to varNameParts entreis
/// \param varNameParts vector of strings containing the variable name and group names
static netCDF::NcVar openHierarchialNetcdfVar(netCDF::NcGroup & topGroup,
                                              const std::vector<std::string> & varNameParts);

/// \brief transfer a block of data from a netCDF variable to a vector
/// \details This is the "block of locations" signature for getting the netcdf
/// variable data. The startLoc and locCount parameters are used to specify the
/// block of data to read. The first dimension of the variable is assumed to be
/// Location.
/// \tparam VarType data type of the variable
/// \param startLoc staring location (Location is the first dimension)
/// \param locCount count of locations to read
/// \param var netcdf variable
template <typename VarType>
static std::vector<VarType> getNcVarData(const std::size_t startLoc,
                                         const std::size_t locCount,
                                         const netCDF::NcVar & var);

/// \brief helper function for the getNcVarData function
/// \details This function is used by the getNcVarData to make the call to
/// the netCDF API to get the variable data when that variable has
/// Location for the first dimenstion. In this case, we want to select
/// a subset block of locations from the input file. The function is
/// specialized for std::string to handle the special case of string
/// data in netCDF.
/// \tparam VarType data type of the variable
/// \param var netcdf variable
/// \param start starting location for the data
/// \param count count of locations to read
/// \param varData vector to hold the variable data
template <typename VarType>
static void getSelectNcVarData(const netCDF::NcVar & var,
                               const std::vector<std::size_t> & start,
                               const std::vector<std::size_t> & count,
                               std::vector<VarType> & varData);

/// \brief helper function for the getNcVarData function
/// \details This function is used by the getNcVarData to make the call to
/// the netCDF API to get the variable data when that variable does not
/// have Location for the first dimenstion. In this case, we want to
/// read the entire variable from the file. The function is specialized
/// for std::string to handle the special case of string data in netCDF.
/// \tparam VarType data type of the variable
/// \param var netcdf variable
/// \param varData vector to hold the variable data
template <typename VarType>
static void getAllNcVarData(const netCDF::NcVar & var, std::vector<VarType> & varData);

/// \brief replace fill values with JEDI missing values
/// \param var netCDF variable
/// \param varData vector of variable data
template <typename VarType>
static void replaceFillValuesWithMissing(const netCDF::NcVar & var,
                                         std::vector<VarType> & varData);

/// \brief get the fill value for a netCDF variable
/// \tparam VarType data type for variable
/// \param var netcdf variable
template <typename VarType>
static VarType getNcVarFillValue(const netCDF::NcVar & var);

/// \brief helper functions to get the default fill value for a netCDF variable
/// \param fillValue
static int getNcVarDefaultFillValue(const int & fillValue);
static int64_t getNcVarDefaultFillValue(const int64_t & fillValue);
static float getNcVarDefaultFillValue(const float & fillValue);
static std::string getNcVarDefaultFillValue(const std::string & fillValue);

/// \brief get the units string from the given netcdf variable
/// \param ncVar netcdf variable
std::string getNcVarUnits(const netCDF::NcVar & var);

/// \brief  transfer variable data to the destination OSDF container
/// \tparam VarType
/// \param varName hierarchical name of variable
/// \param varUnit unit of the variable
/// \param varData variable data
/// \param varDimNames variable dimension names
/// \param destOSDF destination OSDF container
/// \param osdfMetadata frame metadata for dest OSDF
template <typename VarType>
static void transferVarDataToOSDF(const std::string& varName,
                                  const std::string& varUnit,
                                  const std::vector<VarType> & varData,
                                  const std::size_t numlocs,
                                  const std::vector<std::string> & varDimNames,
                                  std::unique_ptr<osdf::IFrame> & destOSDF,
                                  osdf::FrameMetadata & osdfMetadata);

/// \brief load specified range of locations from the input file to the OSDF container
/// \details This function is not MPI aware, rather it is the low level function that handles
/// loading data from an input hdf5 file on a single process. It takes a range of locations
/// which will allow it to be run on multiple processes where the range of locations point
/// to different sections of the data for different ranks.
/// Note that the specification of the location range is given as a start location number
/// and a count. This lines up with the manner in which netcdf/hdf5 hyperslab block
/// specification is made. The idea is to give the first N locations to rank 0, then the
/// second N locations to rank 1, and so forth in a load balanced manner.
/// \param inFile netcdf file object (opened in read mode)
/// \param startLoc beginning of location range
/// \param locCount number of locations in range
/// \param destOSDF destination OSDF container object
/// \param osdfMetadata frame metadata for dest OSDF
int loadObsBlockFromNetcdf(netCDF::NcFile & inFile,
                           const std::size_t startLoc,
                           const std::size_t locCount,
                           std::unique_ptr<osdf::IFrame> & destOSDF,
                           osdf::FrameMetadata & osdfMetadata);

//--------------------------------------------------------------------------------
// Function definitions for "private" functions
//--------------------------------------------------------------------------------

//---------------------------------------------------------------------
bool isNetcdfVarADimension(const netCDF::NcGroup & group, const std::string & varName) {
  // Get a map of dimensions from the group and check if varName exists as a key in the map
  std::multimap<std::string, netCDF::NcDim> dimMap = group.getDims();
  return (dimMap.find(varName) != dimMap.end());
}

//---------------------------------------------------------------------
// TODO(srh): We eventually need to eliminate the variable limitations on storage
// in the OSDF container. For now, we can make a lot of progress before these limitations
// are removed.
bool keepNetcdfVarForOSDF(const std::string & varName,
                          const netCDF::NcVar & var,
                          std::vector<std::string> & varDimNames) {
  // Get a list of the dimensions attached to this variable. We want to keep
  // variables that are dimensioned by:
  //    1D: any single dimension
  //    2D: Location, <any slice dimension>
  bool keepVar = true;
  const std::vector<netCDF::NcDim> varDims = var.getDims();
  const std::size_t numDims = varDims.size();
  varDimNames.clear();
  if ((numDims == 0) || (numDims > 2)) {
    oops::Log::info() << "WARNING: ioda::reader::keepNetcdfVarForOSDF: Variable: " << varName
              << " has no dimensions or more than 2 dimensions. Skipping." << std::endl;

    keepVar = false;
  } else if (numDims == 1) {
    // 1D variable: accept it regardless of the dimension name.
    varDimNames.push_back(varDims[0].getName());
  } else {
    // 2D variable: accept it only if the first dimension is Location and the second dimension
    // is some *other* slice dimension. A [Location, Location] variable has no slice
    // dimension for the OSDF to expand into slice columns (only slice dims are registered
    // in FrameMetadata), so it cannot round-trip -- skip it rather than silently mangle it.
    const std::string firstDimName = varDims[0].getName();
    const std::string sliceDimName = varDims[1].getName();
    if (firstDimName == "Location" && sliceDimName != "Location") {
      varDimNames.push_back(firstDimName);
      varDimNames.push_back(sliceDimName);
    } else {
      oops::Log::info() << "WARNING: ioda::reader::keepNetcdfVarForOSDF: 2D Variable: " << varName
                << " is not dimensioned [Location, <slice-dim>]. Skipping." << std::endl;
      keepVar = false;
    }
  }
  return keepVar;
}

//---------------------------------------------------------------------
std::vector<std::string> listAllNetcdfVars(const netCDF::NcGroup& group,
                                           const std::string & groupPathPrefix,
                                           const bool listDimensions) {
  // List out this group's variables
  std::string varName;
  std::vector<std::string> varNames;
  for (const auto & varInfo : group.getVars()) {
    // Determine if the variable is a dimension or not.
    const bool varIsDim = isNetcdfVarADimension(group, varInfo.first);

    // Add the hierarchical variable name to the list according to the listDimensions flag.
    if (groupPathPrefix.empty()) {
      varName = varInfo.first;
    } else {
      varName = groupPathPrefix + std::string("/") + varInfo.first;
    }
    if (listDimensions) {
      if (varIsDim) {
        varNames.push_back(varName);
      }
    } else {
      if (!varIsDim) {
        varNames.push_back(varName);
      }
    }
  }

  // Traverse to all child groups and repeat.
  for (const auto & groupInfo : group.getGroups()) {
    std::string newGroupPath;
    if (groupPathPrefix.empty()) {
      // If the group path prefix is empty, just use the group name
      // as the new group path prefix
      newGroupPath = groupInfo.first;
    } else {
      // Otherwise, prepend the group path to the group name
      newGroupPath = groupPathPrefix + std::string("/") + groupInfo.first;
    }
    const netCDF::NcGroup childGroup = group.getGroup(groupInfo.first);
    const std::vector<std::string> childVarNames =
        listAllNetcdfVars(childGroup, newGroupPath, listDimensions);
    varNames.insert(varNames.end(), childVarNames.begin(), childVarNames.end());
  }

  // Return the list of variable names
  return varNames;
}

//---------------------------------------------------------------------
netCDF::NcVar openHierarchialNetcdfVar(netCDF::NcGroup & topGroup,
                                       const std::vector<std::string> & varNameParts) {
  // Use varNameParts to walk down the group hierarchy to get to the variable.
  netCDF::NcVar var;
  if (varNameParts.size() == 1) {
    // If there is only one part, it's the variabe at the top level
    var = topGroup.getVar(varNameParts[0]);
  } else {
    // Otherwise, walk through the groups and subgroups then get the variable
    netCDF::NcGroup group = topGroup.getGroup(varNameParts[0]);
    for (std::size_t i = 1; i < varNameParts.size() - 1; ++i) {
      group = group.getGroup(varNameParts[i]);
    }
    var = group.getVar(varNameParts.back());
  }
  return var;
}

//--------------------------------------------------------------------------------
template <typename VarType>
std::vector<VarType> getNcVarData(const std::size_t startLoc, const std::size_t locCount,
                                  const netCDF::NcVar & var) {
  std::vector<VarType> varData;
  if (var.getDim(0).getName() == "Location") {
    // Location is the first dimension in this form of reading. We want to use the hyperslab
    // selection for the block of locations (first dimension) and select the entirety of any
    // remaining dimesions.
    std::vector<size_t> count(0);
    const std::vector<netCDF::NcDim> varDims = var.getDims();
    for (auto & dim : varDims) {
      count.push_back(dim.getSize());
    }
    std::vector<size_t> start(count.size(), 0);
    start[0] = startLoc;
    count[0] = locCount;

    std::size_t numElements = 1;
    for (auto & dimSize : count) {
      numElements *= dimSize;
    }
    varData.resize(numElements);
    getSelectNcVarData<VarType>(var, start, count, varData);
  } else {
    // Location is not the first dimension, so we read the entire variable.
    varData.resize(var.getDim(0).getSize());
    getAllNcVarData<VarType>(var, varData);
  }
  return varData;
}

//--------------------------------------------------------------------------------
template <typename VarType>
void getSelectNcVarData(const netCDF::NcVar & var,
                        const std::vector<std::size_t> & start,
                        const std::vector<std::size_t> & count,
                        std::vector<VarType> & varData) {
  var.getVar(start, count, varData.data());
}

// Explicit specialization for char
template <>
void getSelectNcVarData<char>(const netCDF::NcVar & var,
                             const std::vector<std::size_t> & start,
                             const std::vector<std::size_t> & count,
                             std::vector<char> & varData) {
  // The char type is ambiguous about the signedness of the data, so this specialization
  // is needed to assume a signed char type for the underlying data in order to call
  // the correct netCDF API function (nc_get_vara_schar vs nc_get_vara_uchar).
  var.getVar(start, count, reinterpret_cast<signed char *>(varData.data()));
}

// Explicit specialization for std::string
template <>
void getSelectNcVarData<std::string>(const netCDF::NcVar & var,
                                     const std::vector<std::size_t> & start,
                                     const std::vector<std::size_t> & count,
                                     std::vector<std::string> & varData) {
  // The string type is a special case where the data in the netCDF variable is stored in
  // allocated (heap) memory. The means for accessing this data a vector of char *
  // pointers must be used. Then the data is transferred to a vector of std::string.
  std::vector<char *> tmpVarData(varData.size());
  var.getVar(start, count, tmpVarData.data());
  for (std::size_t i = 0; i < varData.size(); ++i) {
    varData[i] = std::string(tmpVarData[i]);
  }
}

//--------------------------------------------------------------------------------
template <typename VarType>
void getAllNcVarData(const netCDF::NcVar & var, std::vector<VarType> & varData) {
  var.getVar(varData.data());
}

template <>
void getAllNcVarData(const netCDF::NcVar & var, std::vector<std::string> & varData) {
  // The string type is a special case where the data in the netCDF variable is stored in
  // allocated (heap) memory. The means for accessing this data a vector of char *
  // pointers must be used. Then the data is transferred to a vector of std::string.
  std::vector<char *> tmpVarData(varData.size());
  var.getVar(tmpVarData.data());
  for (std::size_t i = 0; i < varData.size(); ++i) {
    varData[i] = std::string(tmpVarData[i]);
  }
}

//--------------------------------------------------------------------------------
template <typename VarType>
void replaceFillValuesWithMissing(const netCDF::NcVar & var, std::vector<VarType> & varData) {
  // Replace the fill values with the JEDI missing value. The fill value is
  // determined by the type of the variable.
  const VarType jediMissingValue = util::missingValue<VarType>();
  const VarType fillValue = getNcVarFillValue<VarType>(var);
  if constexpr (std::is_floating_point_v<VarType>) {
    // Float path: always scan. A file may use IEEE NaN/inf as a "no-data"
    // marker even when _FillValue == the JEDI missing value, so the
    // early-return that applies to other types cannot be taken here.
    for (auto & varVal : varData) {
      if (varVal == fillValue || std::isnan(varVal) || std::isinf(varVal)) {
        varVal = jediMissingValue;
      }
    }
  } else {
    // Non-float path: isnan/isinf are not meaningful, and if the fill value is
    // the same as the JEDI missing value there is nothing to do.
    if (fillValue == jediMissingValue) {
      return;
    }
    for (auto & varVal : varData) {
      if (varVal == fillValue) {
        varVal = jediMissingValue;
      }
    }
  }
}

//--------------------------------------------------------------------------------
template <typename VarType>
VarType getNcVarFillValue(const netCDF::NcVar & var) {
  // Give precedence to an explicit _FillValue attribute; otherwise use the netCDF
  // default. (Avoids NcVar::getFillModeParameters(), which is unreliable here.)
  VarType fillValue;
  const auto atts = var.getAtts();
  const auto attIt = atts.find("_FillValue");
  if (attIt == atts.end()) {
    // Just need to call getNcVarDefaultFillValue with a variable of the desired
    // data type to get the right overload function.
    return getNcVarDefaultFillValue(fillValue);
  }
  const netCDF::NcVarAtt & fillAtt = attIt->second;
  if constexpr (std::is_same<VarType, char>::value) {
    // NcAtt::getValues(char *) always calls nc_get_att_text(), regardless of
    // the attribute's actual netCDF type. That throws for the numeric
    // (NC_BYTE) _FillValue attributes used on byte/bool variables, so read
    // via signed char instead, which correctly uses nc_get_att_schar().
    signed char attFillValue;
    fillAtt.getValues(&attFillValue);
    return static_cast<VarType>(attFillValue);
  }
  if constexpr (std::is_same<VarType, std::string>::value) {
    // _FillValue on a string variable may be stored as either a variable-length
    // NC_STRING or a fixed-length NC_CHAR attribute, and (as with getNcVarUnits()
    // above) the getValues() overload needed differs for each: the std::string&
    // overload always calls nc_get_att_text(), which only understands NC_CHAR.
    if (fillAtt.getType().getName() == "string") {
      char * attFillValue;
      fillAtt.getValues(&attFillValue);
      const std::string result(attFillValue);
      nc_free_string(1, &attFillValue);
      return result;
    }
    fillAtt.getValues(fillValue);
    return fillValue;
  }
  fillAtt.getValues(&fillValue);
  return fillValue;
}

//--------------------------------------------------------------------------------
int getNcVarDefaultFillValue(const int & fillValue) {
  return NC_FILL_INT;
}
int64_t getNcVarDefaultFillValue(const int64_t & fillValue) {
  return NC_FILL_INT64;
}
float getNcVarDefaultFillValue(const float & fillValue) {
  return NC_FILL_FLOAT;
}
std::string getNcVarDefaultFillValue(const std::string & fillValue) {
  return NC_FILL_STRING;
}
//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------------
std::string getNcVarUnits(const netCDF::NcVar & var) {
  // Assume that var is a variable with an attribute name "units".
  // Here's an example of the units value:
  //    "seconds since 1970-01-01T00:00:00Z"
  netCDF::NcVarAtt unitsAttr;

  try {
    unitsAttr = var.getAtt("units");
  } catch (netCDF::exceptions::NcException&) {
    oops::Log::warning() << "Variable: " << var.getName()
                         << " does not have attribute units. Populating with missing units."
                         << std::endl;
    return util::missingValue<std::string>();
  }
  checkNcObj(
    unitsAttr,
    "ioda::reader::getNcVarUnits: Failed to open attribute 'units' on variable: " + var.getName());

  // We need to detect which string type (fixed length vs variable length) we have
  // in the "units" attribute before attempting to read it because the underlying
  // calls need to be different for these two cases.
  //     Variable length string -> use the char** version of getValues which
  //                               calls the underlying function: nc_get_attr_string
  //     Fixed length string -> use the std::string version of getValues which
  //                            calls the underlying function: nc_get_attr_text
  //
  // The unitsAttr.getType().getName() function calls will return "string" for
  // a variable length string, and return "char" for a fixed length string.
  const std::string unitsTypeName = unitsAttr.getType().getName();
  std::string unitsString;
  if (unitsTypeName == "string") {
    // variable length string
    char * tempString;
    unitsAttr.getValues(&tempString);
    unitsString = std::string(tempString);
    nc_free_string(1, &tempString);
  } else if (unitsTypeName == "char") {
    // fixed length string
    unitsAttr.getValues(unitsString);
  } else {
    // unrecognized type name for string type
    const std::string errMsg = std::string("ioda::reader::getNcVarUnits: Unrecognized ") +
      std::string("string type name for the 'units' attribute: ") + unitsTypeName;
    throw std::runtime_error(errMsg);
  }
  return unitsString;
}

//--------------------------------------------------------------------------------
template <typename VarType>
void transferVarDataToOSDF(const std::string& varName,
                           const std::string& varUnit,
                           const std::vector<VarType> & varData,
                           const std::size_t numLocs,
                           const std::vector<std::string> & varDimNames,
                           std::unique_ptr<osdf::IFrame> & destOSDF,
                           osdf::FrameMetadata & osdfMetadata) {
  // varData should be the proper size. It is either 1D (Location), 1D (some slice
  // dimension) or 2D (Location X some slice dimension). We've already verified the
  // dimensioning when we decided to keep the variable for loading into the OSDF container.
  // The slice index values (real coordinate values or synthetic 0..n-1 indices) come from
  // the FrameMetadata dimension registry, and serve as the "_<index>" column suffixes.
  if (varDimNames.size() == 1) {
    if (varDimNames[0] == "Location") {
      // 1D variable dimensioned by Location
      if (varData.size() == numLocs) {
        // Ready to append to the OSDF container
        destOSDF->appendNewColumn(varName, varData, varUnit);
      } else {
        const std::string errMsg = std::string("ioda::reader::transferVarDataToOSDF: ") +
            std::string("1D Variable (Location) size != numLocs: ") + varName;
        throw std::runtime_error(errMsg);
      }
    } else {
      // 1D variable dimensioned by some slice dimension (e.g. Channel, Level): broadcast
      // each slice value across all locations, one column per slice.
      const std::string & dimName = varDimNames[0];
      const std::vector<int> & dimNums = osdfMetadata.getDimNums(dimName);
      if (varData.size() == dimNums.size()) {
        // TODO(srh): This is inefficient in terms of memory usage. We can address
        // this later if it turns out to be problematic.
        for (std::size_t islice = 0; islice < dimNums.size(); ++islice) {
          std::vector<VarType> dataSlice(numLocs, varData[islice]);
          destOSDF->appendNewColumn(
            varName + std::string("_") + std::to_string(dimNums[islice]),
            dataSlice, varUnit);
        }
        osdfMetadata.addVarDimNames(varName, varDimNames);
      } else {
        const std::string errMsg = std::string("ioda::reader::transferVarDataToOSDF: ") +
            std::string("1D Variable (") + dimName + std::string(") size != dim size: ") + varName;
        throw std::runtime_error(errMsg);
      }
    }
  } else if (varDimNames.size() == 2) {
    // 2D variable dimensioned by Location and some slice dimension. Expand into one
    // column per slice.
    const std::string & dimName = varDimNames[1];
    const std::vector<int> & dimNums = osdfMetadata.getDimNums(dimName);
    const std::size_t numSlices = dimNums.size();
    if (varData.size() == (numLocs * numSlices)) {
      for (std::size_t islice = 0; islice < numSlices; ++islice) {
        std::vector<VarType> dataSlice(numLocs);
        for (std::size_t iloc = 0; iloc < numLocs; ++iloc) {
          const std::size_t idata = (numSlices * iloc) + islice;
          dataSlice[iloc] = varData[idata];
        }
        destOSDF->appendNewColumn(
          varName + std::string("_") + std::to_string(dimNums[islice]),
          dataSlice, varUnit);
      }
      osdfMetadata.addVarDimNames(varName, varDimNames);
    } else {
      const std::string errMsg = std::string("ioda::reader::transferVarDataToOSDF: ") +
        std::string("2D Variable (Location, ") + dimName +
        std::string(") size != (numLocs * dim size): ") + varName;
      throw std::runtime_error(errMsg);
    }
  }

  // Count variable (in numVars) if it is in the ObsValue group
  if (varName.find("ObsValue/") != std::string::npos) {
    osdfMetadata.incrNumVars();
  }
}

//---------------------------------------------------------------------
int loadObsBlockFromNetcdf(netCDF::NcFile & inFile,
                           const std::size_t startLoc,
                           const std::size_t locCount,
                           std::unique_ptr<osdf::IFrame> & destOSDF,
                           osdf::FrameMetadata & osdfMetadata) {
  // Get a list of all the dimensions in the file. We won't store dimensions directly in
  // the destOSDF container, but we need to register their coordinate values so the per-slice
  // columns can be expanded. The number of locations that will be read from the file is given
  // by the locCount parameter. But we want to check that locCount is not greater than the
  // total number of locations in the file.
  const std::vector<std::string> allDims = listAllNetcdfVars(inFile, std::string(""), true);
  for (const auto & dimName : allDims) {
    if (dimName == "Location") {
      // For Location, just bounds-check the requested location count.
      netCDF::NcDim dim = inFile.getDim(dimName);
      checkNcObj(dim, "ioda::reader::loadObsBlockFromNetcdf: Failed to get dimension: " + dimName);
      const std::size_t numLocations = dim.getSize();
      if (locCount > numLocations) {
        throw std::runtime_error("ioda::reader::loadObsBlockFromNetcdf: locCount is greater than "
                                 "number of locations in the file.");
      }
    } else {
      // Register every slice dimension. Store the coordinate's real values as the slice
      // index values when they form a set of unique integers -- either stored as an integer
      // type (e.g. Channel: {1, 3, 5, ..., 22}) or stored as a floating type whose values are
      // all whole numbers (e.g. float Channel: {1, 2, 3, 4}). Floating coordinates are read
      // and cast to int only when every value is integral, so a genuinely fractional
      // coordinate does not silently collide after truncation. In every other case (non-numeric
      // type, non-integral floats, or non-unique integers) fall back to synthetic 0-based
      // indices {0, 1, ..., n-1} so that the "_<index>" column suffixes are always unique.
      netCDF::NcVar var = inFile.getVar(dimName);
      if (var.isNull()) {
        continue;
      }
      const std::size_t dimSize = var.getDim(0).getSize();
      bool useRealValues = false;
      std::vector<int> dimNums(dimSize);
      const netCDF::NcType::ncType coordType = var.getType().getTypeClass();
      if (coordType == netCDF::NcType::ncType::nc_INT) {
        var.getVar(dimNums.data());
        const std::unordered_set<int> uniqueVals(dimNums.begin(), dimNums.end());
        useRealValues = (uniqueVals.size() == dimSize);
      } else if (coordType == netCDF::NcType::ncType::nc_FLOAT ||
                 coordType == netCDF::NcType::ncType::nc_DOUBLE) {
        // Read the raw floating values and accept them as slice indices only if every value
        // is a whole number (so the int cast is exact) and the resulting set is unique.
        std::vector<double> realVals(dimSize);
        var.getVar(realVals.data());
        bool allIntegral = true;
        for (std::size_t i = 0; i < dimSize; ++i) {
          if (realVals[i] != std::floor(realVals[i])) {
            allIntegral = false;
            break;
          }
          dimNums[i] = static_cast<int>(realVals[i]);
        }
        if (allIntegral) {
          const std::unordered_set<int> uniqueVals(dimNums.begin(), dimNums.end());
          useRealValues = (uniqueVals.size() == dimSize);
        }
      } else {
        // Coordinate values are not int, float nor double. Throw an exception to
        // discourage the use of non-numeric dimension coordinates.
        const std::string errMsg =
                "ioda::reader::loadObsBlockFromNetcdf: Unsupported coordinate "
                "type for dimension: " + dimName + std::string("\n") +
                "Must use one of: int, float or double";
        throw eckit::BadParameter(errMsg, Here());
      }
      if (!useRealValues) {
        std::iota(dimNums.begin(), dimNums.end(), 0);
      }
      osdfMetadata.setDimNums(dimName, dimNums);
    }
  }

  // Get a list of all variables in the file expressed as hierarchical paths. The
  // herierchy is due to the netcdf group structure. Walk through all the variables
  // and transfer the location block given by the startLoc and locCount parameters into
  // the destination OSDF container.
  const std::vector<std::string> allVars = listAllNetcdfVars(inFile, std::string(""), false);
  for (const auto & varName : allVars) {
    const std::vector<std::string> varNameParts = ioda::splitString(varName, '/');
    netCDF::NcVar var = openHierarchialNetcdfVar(inFile, varNameParts);
    checkNcObj(var, "ioda::reader::loadObsBlockFromNetcdf: Failed to open variable: " + varName);

    std::vector<std::string> varDimNames;
    if (keepNetcdfVarForOSDF(varName, var, varDimNames)) {
      // Get the variable type for transferring its data to the OSDF
      const netCDF::NcType varType = var.getType();
      std::string varUnit = getNcVarUnits(var);

      if (varType.getName() == "int") {
        std::vector<int> varData = getNcVarData<int>(startLoc, locCount, var);
        replaceFillValuesWithMissing<int>(var, varData);
        transferVarDataToOSDF<int>(
          varName, varUnit, varData, locCount, varDimNames, destOSDF, osdfMetadata);
      } else if (varType.getName() == "int64") {
        std::vector<int64_t> varData = getNcVarData<int64_t>(startLoc, locCount, var);
        replaceFillValuesWithMissing<int64_t>(var, varData);
        transferVarDataToOSDF<int64_t>(
          varName, varUnit, varData, locCount, varDimNames, destOSDF, osdfMetadata);
      } else if (varType.getName() == "float") {
        std::vector<float> varData = getNcVarData<float>(startLoc, locCount, var);
        replaceFillValuesWithMissing<float>(var, varData);
        transferVarDataToOSDF<float>(
          varName, varUnit, varData, locCount, varDimNames, destOSDF, osdfMetadata);
      } else if (varType.getName() == "byte") {
        std::vector<char> varData = getNcVarData<char>(startLoc, locCount, var);
        replaceFillValuesWithMissing<char>(var, varData);
        transferVarDataToOSDF<char>(
          varName, varUnit, varData, locCount, varDimNames, destOSDF, osdfMetadata);
      } else if (varType.getName() == "string") {
        std::vector<std::string> varData =
                                 getNcVarData<std::string>(startLoc, locCount, var);
        replaceFillValuesWithMissing<std::string>(var, varData);
        transferVarDataToOSDF<std::string>(
          varName, varUnit, varData, locCount, varDimNames, destOSDF, osdfMetadata);
      } else {
        oops::Log::info() << "WARNING: ioda::reader::loadObsBlockFromNetcdf: Variable: "
                          << varName << " is not int, int64, float, char or string. Skipping."
                          << std::endl;
      }
    }
  }

  // Create a special column in the destOSDF container that holds the location
  // indices from the file for this block of locations. This is needed by UFO
  // testing to synchronize the resulting ObsSpace locations with the geovals
  // test file. Note: the input ioda obs file and the test geovals file are required
  // to have the same locations in the same order.
  //
  // Since we are not removing any locations during the load process, the startLoc
  // and locCount parameters can be used to create this location index column.
  std::vector<int> locationIndices(locCount);
  std::iota(locationIndices.begin(), locationIndices.end(), startLoc);
  destOSDF->appendNewColumn("sourceLocationIndices", locationIndices);

  return 0;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Function definitions for public functions
//--------------------------------------------------------------------------------

void loadOsdfFromNetcdf(const ObsDataInParameters & dataInParams,
                        const eckit::mpi::Comm & ioPoolComm,
                        std::unique_ptr<osdf::IFrame> & destOSDF,
                        osdf::FrameMetadata & osdfMetadata) {
  const int myMpiRank = ioPoolComm.rank();
  const int myMpiSize = ioPoolComm.size();

  // Check the parameters
  const std::string engineType = dataInParams.engine.value().engineParameters.value().type.value();
  if (engineType != "H5File") {
    const std::string errMsg = "ioda::reader::loadOsdfFromNetcdf: Unsupported engine type: "
                               + engineType + " Must use H5File engine type with this function.";
    throw eckit::BadParameter(errMsg, Here());
  }
  const bool readMultipleFiles = (myMpiSize == 1) ? false : dataInParams.readMultipleFiles.value();
  const std::string fileName = Engines::uniquifyFileName(
                             dataInParams.engine.value().engineParameters.value().getFileName(),
                             readMultipleFiles,
                             myMpiRank,
                             -1);  // timeRankNum not relevant

  oops::Log::info() << "INFO: ioda::reader::loadOsdfFromNetcdf: reading file: "
                    << fileName << std::endl;
  netCDF::NcFile inFile(fileName, netCDF::NcFile::read);
  checkNcObj(inFile, "ioda::reader::loadOsdfFromNetcdf: Failed to open file: " + fileName);

  // Determine numLocations and emptyFile flag.
  int emptyFile = 0;  // 0 --> non-empty file, 1 --> empty file
  std::size_t numLocations = 0;
  if (readMultipleFiles || (myMpiRank == 0)) {
    netCDF::NcDim dim = inFile.getDim("Location");
    checkNcObj(dim, "ioda::reader::loadOsdfFromNetcdf: Failed to get dimension: Location");
    numLocations = dim.getSize();
    emptyFile = (numLocations == 0) ? 1 : 0;
  }

  // Determine start and count values to pass to the loadObsBlockFromNetcdf function
  int start;
  int count;
  if (readMultipleFiles) {  // reading split files, so each IO pool rank reads its whole file
    start = 0;
    count = numLocations;
  } else {  // rank 0 figures out which chunks of the full file that each IO pool rank will read
    std::vector<int> starts(myMpiSize, 0);
    std::vector<int> counts(myMpiSize, 0);
    if (myMpiRank == 0 && !emptyFile) {
      // Divide the locations evenly among the MPI ranks. Do an integer divide (nlocs / mpi size)
      // to get the base size for all ranks. Then spread out any remainder among the first n ranks.
      // Express this distribution in start and count values which are appropriate for calling
      // the loadObsBlockFromNetcdf function. Use mpi scatter to distribute the start and
      // count values to each rank.
      const std::size_t locationsPerRank = numLocations / myMpiSize;
      const std::size_t remainder = numLocations % myMpiSize;
      counts.assign(myMpiSize, locationsPerRank);
      for (std::size_t i = 0; i < remainder; ++i) {
        counts[i]++;
      }
      int seed = 0;
      for (size_t i = 1; i < counts.size(); i++) {
          seed += counts[i-1];
          starts[i] = seed;
      }
    }
    ioPoolComm.broadcast(emptyFile, 0);
    if (!emptyFile) {
      ioPoolComm.scatter(starts, start, 0);
      ioPoolComm.scatter(counts, count, 0);
    }
  }

  // Split file or not, each IO pool rank now reads its assigned data into an OSDF container.
  if (!emptyFile) {
    const int rc = loadObsBlockFromNetcdf(inFile, start, count, destOSDF, osdfMetadata);
    if (rc != 0) {
      const std::string errMsg = "loadOsdfFromNetcdf: Failed to load block: "
                                 " start: " + std::to_string(start) +
                                 " count: " + std::to_string(count);
      throw std::runtime_error(errMsg);
    }
  }
  inFile.close();
}

}  // namespace reader
}  // namespace ioda
