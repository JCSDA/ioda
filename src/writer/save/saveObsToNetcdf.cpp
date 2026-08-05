/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/writer/save/saveObsToNetcdf.hpp"

#include <netcdf>
#include <algorithm>
#include <utility>

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/ObsDataIoParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace writer {

//--------------------------------------------------------------------------------
// Function declarations for "private" functions
//--------------------------------------------------------------------------------

struct DimCreationParameters {
  int size;
  bool unlimited;

  DimCreationParameters(const int dim_size, bool unlim) :
      size(dim_size), unlimited(unlim) {}
};

struct VarCreationParameters {
  std::string assocColumn;
  std::vector<std::string> dimNames;
  netCDF::NcType dataType;
  std::string units;

  VarCreationParameters(const std::string & assoc_col,
                        const std::vector<std::string> & dim_names,
                        const netCDF::NcType data_type,
                        const std::string & units) :
      assocColumn(assoc_col), dimNames(dim_names), dataType(data_type), units(units) {}
};

typedef std::vector<std::pair<std::string, DimCreationParameters>> DimCreationList;
typedef std::vector<std::pair<std::string, VarCreationParameters>> VarCreationList;

/// \brief convert osdf data type to netcdf data type
/// \param osdfDataType osdf data type as an int8_t
/// \return corresponding netCDF::NcType
static netCDF::NcType convertOsdfDataTypeToNcType(const std::int8_t osdfDataType);

/// \brief Find an OSDF column associated with the variable name without
/// a channel suffix.
/// \details This needs to be done to look up the column metadata
/// associated with the variable name without a channel suffix.
/// \param varName variable name without channel suffix
/// \param srcOsdf source OSDF container from which to get the column names
/// \param osdfMetadata source OSDF metadata containing information about the variables
/// \return column name with the first slice suffix if the variable has a slice dimension,
/// otherwise return varName
static std::string findColNameWithSliceSuffix(const std::string & varName,
                                             const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                             const osdf::FrameMetadata & osdfMetadata);

/// \brief Create a data structure containing the output (ioda) dimension names with their
/// corresponding creation paramters.
/// \details For now we should only see these dimensions
///  1. Location
///  2. Channel
/// \param srcOsdf source OSDF container from which to get the column names
/// \param osdfMetadata source OSDF metadata containing information about the dimensions
/// \return VarCreationList output vector of variable names with their creation parameters
static DimCreationList makeDimCreationList(const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                           const osdf::FrameMetadata & osdfMetadata);

/// \brief Create a data structure containing the output (ioda) dimension variable
/// names with their corresponding creation paramters.
/// \details For now we should only see these dimensions
///  1. The variables that will be dimensioned by Location
///  2. The variables that will be dimensioned by Channel
/// \param srcOsdf source OSDF container from which to get the column names
/// \param osdfMetadata source OSDF metadata containing information about the variables
/// \return VarCreationList output vector of variable names with their creation parameters
static VarCreationList makeDimVarCreationList(const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                              const osdf::FrameMetadata & osdfMetadata);

/// \brief create netcdf dimension
/// \param group netcdf group containing the dimension
/// \param dimName name of dimension
/// \param createParams dimension creation parameters
static void createNcDim(netCDF::NcGroup & group, const std::string & dimName,
                        const DimCreationParameters & createParams);

/// \brief set values in a netcdf dimension variable
/// \details Note that we are only supporting Location and Channel dimensions
/// at this time.
/// \param dimVar netcdf dimenion variable object
/// \param srcOsdf source OSDF container holding Location values
/// \param osdfMetadata source OSDF metadata holding Channel values
/// \param nlocsStart row offset for the "Location" dimension variable (0 unless writing
/// this rank's slice of a single shared file); ignored for every other dimension variable
static void setNcDimVar(netCDF::NcVar & dimVar,
                       const std::unique_ptr<osdf::IFrame> & srcOsdf,
                       const osdf::FrameMetadata & osdfMetadata,
                       const std::size_t nlocsStart = 0);

/// \brief Create a data structure containing the output (ioda) variable names with their
/// corresponding creation paramters.
/// \details For now we should only see these three types of variables
///  1. The variables that will be dimensioned by Location
///  2. The variables that will be dimensioned by Channel
///  3. The variables that will be dimensioned by Location and Channel
/// \param srcOsdf source OSDF container from which to get the column names
/// \param osdfMetadata source OSDF metadata containing information about the variables
/// \return VarCreationList output vector of variable names with their creation parameters
static VarCreationList makeVarCreationList(const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                           const osdf::FrameMetadata & osdfMetadata);

/// \brief create the variable (specified as a hierarchical path) from the netCDF file
/// \details This function allows variable names with hierarchical paths to be
/// created in the netCDF file.  The variable name is specified as a vector
/// of strings, where the last entry is the variable and any leading entries are
/// the group names leading to the variable.
/// \param topGroup top level group object corresponding to varNameParts entries
/// \param ncVarName hierarchical variable name (ie, group/variable)
/// \param createParams creation parameters for the netcdf variable
static netCDF::NcVar createHierNcVar(netCDF::NcGroup & topGroup,
                                     const std::string & ncVarName,
                                     const VarCreationParameters & createParams);

/// \brief set the fill value for a newly created netcdf variable
/// \param var netcdf variable
/// \param type netcdf data type
static void setNcVarFillValue(netCDF::NcVar & var, const netCDF::NcType & type);

/// \brief set all of the necessary netcdf variable attributes
/// \param var netcdf variable
/// \param createParams variable creation parameters
static void setNcVarAttributes(netCDF::NcVar & var, const VarCreationParameters & createParams);

/// \brief helper function for calling the appropriate netcdf putVar function
/// \param var netcdf variable object
/// \param starts hyperslab start indices for each dimension
/// \param counts hyperslab count indices for each dimension
/// \param varData vector of data values to write to the variable
// Note: we want to treat the varData parameter as a const, be we cannot
// do that because we need to copy the string data pointers from the
// std::string specialization. See the comments in the definition of this
// function below.
template <typename VarType>
void setNcVarData(netCDF::NcVar & var, const std::vector<std::size_t> & starts,
                  const std::vector<std::size_t> & counts,
                  std::vector<VarType> & varData);

/// \brief set values in a netcdf variable
/// \param var netcdf dimension variable object
/// \param assocColumn column name in srcOsdf corresponding to the netcdf variable
/// \param srcOsdf source OSDF container holding all columns
/// \param osdfMetadata source OSDF metadata holding channel description
/// \param nlocsStart row offset for the "Location" axis (0 unless writing this rank's
/// slice of a single shared file); has no effect on any slice (eg, Channel) axis, which
/// is always written in full since that data is identical on every pool rank
static void setNcVar(netCDF::NcVar & var,
                     const std::string & assocColumn,
                     const std::unique_ptr<osdf::IFrame> & srcOsdf,
                     const osdf::FrameMetadata & osdfMetadata,
                     const std::size_t nlocsStart = 0);

//--------------------------------------------------------------------------------
// Function definitions for "private" functions
//--------------------------------------------------------------------------------

//---------------------------------------------------------------------
netCDF::NcType convertOsdfDataTypeToNcType(const std::int8_t osdfDataType) {
  switch (osdfDataType) {
    case osdf::consts::eInt:
      return netCDF::NcType::nc_INT;
      break;
    case osdf::consts::eInt64:
      return netCDF::NcType::nc_INT64;
      break;
    case osdf::consts::eFloat:
      return netCDF::NcType::nc_FLOAT;
      break;
    case osdf::consts::eChar:
      return netCDF::NcType::nc_CHAR;
      break;
    case osdf::consts::eString:
      return netCDF::NcType::nc_STRING;
      break;
    default:
      throw eckit::BadValue("ioda::writer::saveOsdfToNetcdf: Unrecognized osdf data type: "
                              + std::to_string(osdfDataType),
                            Here());
  }
}

//---------------------------------------------------------------------
std::string findColNameWithSliceSuffix(const std::string & varName,
                                       const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                       const osdf::FrameMetadata & osdfMetadata) {
  // If the variable has a slice dimension, then use the first column name with
  // a slice suffix. Otherwise, just return the variable name.
  std::string colNameWithSliceSuffix = varName;
  const std::string dimName = osdfMetadata.varSliceDimName(varName);
  if (!dimName.empty()) {
    const std::vector<int> & sliceNums = osdfMetadata.getDimNums(dimName);
    if (!sliceNums.empty()) {
      colNameWithSliceSuffix += "_" + std::to_string(sliceNums.front());
    }
  }
  return colNameWithSliceSuffix;
}

//---------------------------------------------------------------------
void setNcVarFillValue(netCDF::NcVar & var, const netCDF::NcType & type) {
  switch (type.getId()) {
    case netCDF::NcType::ncType::nc_INT: {
      const int fillVal = util::missingValue<int>();
      netCDF::NcVarAtt att = var.putAtt("_FillValue", type, fillVal);
      checkNcObj(att,
        "ioda::writer::setNcVarFillValue: Failed to create int attribute: _FillValue");
      break;
      }
    case netCDF::NcType::ncType::nc_INT64: {
      const int64_t fillVal = util::missingValue<int64_t>();
      netCDF::NcVarAtt att = var.putAtt("_FillValue", type, fillVal);
      checkNcObj(att,
        "ioda::writer::setNcVarFillValue: Failed to create int64 attribute: _FillValue");
      break;
      }
    case netCDF::NcType::ncType::nc_FLOAT: {
      const float fillVal = util::missingValue<float>();
      netCDF::NcVarAtt att = var.putAtt("_FillValue", type, fillVal);
      checkNcObj(att,
        "ioda::writer::setNcVarFillValue: Failed to create float attribute: _FillValue");
      break;
      }
    case netCDF::NcType::ncType::nc_CHAR: {
      const char fillVal = util::missingValue<char>();
      netCDF::NcVarAtt att = var.putAtt("_FillValue", type, 1, &fillVal);
      checkNcObj(att,
        "ioda::writer::setNcVarFillValue: Failed to create char attribute: _FillValue");
      break;
      }
    case netCDF::NcType::ncType::nc_STRING: {
      const std::string fillVal = util::missingValue<std::string>();
      const char * values[] = { fillVal.data() };
      netCDF::NcVarAtt att = var.putAtt("_FillValue", 1, values);
      checkNcObj(att,
        "ioda::writer::setNcVarFillValue: Failed to create string attribute: _FillValue");
      break;
      }
    default:
      throw eckit::BadValue(
        "ioda::writer::setNcVarFillValue: Unsupported data type: " + type.getName(),
        Here());
  }
}

//---------------------------------------------------------------------
void setNcVarAttributes(netCDF::NcVar & var, const VarCreationParameters & createParams) {
  // For now always want to set two attributes:
  //  _FillValue  - for specifying the missing value
  //  units       - conventional units name
  setNcVarFillValue(var, createParams.dataType);
  var.putAtt("units", createParams.units);
}

//---------------------------------------------------------------------
DimCreationList makeDimCreationList(const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                    const osdf::FrameMetadata & osdfMetadata) {
  // We always have the Location dimension (unlimited size). In addition, we create a dimension
  // for every slice dimension registered in the frame metadata
  // (Channel, Level, nfactors, etc.). The registered dimension names are sorted so the output
  // file layout is deterministic.
  DimCreationList dimCreationList;
  dimCreationList.emplace_back(
    std::make_pair(std::string("Location"), DimCreationParameters(srcOsdf->numRows(), true)));

  std::vector<std::string> sliceDimNames = osdfMetadata.getDimNames();
  std::sort(sliceDimNames.begin(), sliceDimNames.end());
  for (const auto & dimName : sliceDimNames) {
    if (dimName == "Location") {
      continue;  // Location is handled above; never expected in the registry, but guard anyway
    }
    const int dimSize = osdfMetadata.getDimNums(dimName).size();
    dimCreationList.emplace_back(std::make_pair(dimName, DimCreationParameters(dimSize, false)));
  }
  return dimCreationList;
}

//---------------------------------------------------------------------
VarCreationList makeDimVarCreationList(const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                       const osdf::FrameMetadata & osdfMetadata) {
  // We always have the Location dimension, and we can check with the osdfMetata
  // to see if we need the Channel dimension.
  // For the creation parameters:
  //   - There are no conventional units for dimensions, so use a blank string.
  //   - The dimension list is always one dimension where the dimension and
  //     variable names match.
  //   - Make the associate column name match the dimension and variable name
  VarCreationList varCreationList;
  const std::string varUnits("");
  {
    const std::string locDimName("Location");
    VarCreationParameters locVarCreateParams(locDimName, {locDimName},
                                             netCDF::NcType::nc_INT64, varUnits);
    varCreationList.emplace_back(std::make_pair(locDimName, locVarCreateParams));
  }

  // Create a coordinate variable for every registered slice dimension (Channel, Level,
  // nfactors, etc.). Sorted for deterministic output file layout.
  std::vector<std::string> sliceDimNames = osdfMetadata.getDimNames();
  std::sort(sliceDimNames.begin(), sliceDimNames.end());
  for (const auto & dimName : sliceDimNames) {
    if (dimName == "Location") {
      continue;
    }
    VarCreationParameters dimVarCreateParams(dimName, {dimName},
                                             netCDF::NcType::nc_INT, varUnits);
    varCreationList.emplace_back(std::make_pair(dimName, dimVarCreateParams));
  }
  return varCreationList;
}

//---------------------------------------------------------------------
void createNcDim(netCDF::NcGroup & group, const std::string & dimName,
                 const DimCreationParameters & createParams) {
  // When unlimited, create the dimension without the size parameter.
  netCDF::NcDim dim;
  if (createParams.unlimited) {
    dim = group.addDim(dimName);
  } else {
    dim = group.addDim(dimName, createParams.size);
  }
  checkNcObj(dim, "ioda::writer::saveOsdfToNetcdf: Failed to create dimension: " + dimName);
}

//---------------------------------------------------------------------
void setNcDimVar(netCDF::NcVar & dimVar,
                 const std::unique_ptr<osdf::IFrame> & srcOsdf,
                 const osdf::FrameMetadata & osdfMetadata,
                 const std::size_t nlocsStart) {
  // The Location values are in the osdf column "sourceLocationIndices". The values for every
  // other (second) dimension are held in the frame metadata dimension registry.
  std::string dimVarName = dimVar.getName();
  if (dimVarName == "Location") {
    std::vector<int> locVals;
    srcOsdf->getColumn("sourceLocationIndices", locVals);
    dimVar.putVar({nlocsStart}, {locVals.size()}, locVals.data());
  } else if (osdfMetadata.hasDim(dimVarName)) {
    const std::vector<int> & dimVals = osdfMetadata.getDimNums(dimVarName);
    dimVar.putVar(dimVals.data());
  } else {
    throw eckit::BadValue(
      "ioda::write::setNcDimVar: unknown dimension name: " + dimVarName, Here());
  }
}

//---------------------------------------------------------------------
VarCreationList makeVarCreationList(const std::unique_ptr<osdf::IFrame> & srcOsdf,
                                    const osdf::FrameMetadata & osdfMetadata) {
  // Get the list of column names from srcOsdf. Use this list and the
  // information in osdfMetadata to create the list of variable
  // names with their corresponding dimension names.
  VarCreationList varCreationList;
  const std::vector<std::string> columnNames =
    ioda::osdfColNamesWithoutChanSuffixes(*srcOsdf, osdfMetadata);
  for (const std::string & columnName : columnNames) {
    // Skip special variables that are not intended for the output file.
    // For now, the sourceLocationIndices variable (in the top level group)
    // is used to create the Location dimension and so we are not writing
    // it to the output file as a variable.
    if (columnName == "sourceLocationIndices") {
      continue;
    }

    // Determine whether the variable has a slice dimension. The column names
    // here have had their numeric slice suffixes stripped, so columnName is the collapsed name.
    std::vector<std::string> varDimNames;
    std::string associatedColumn;
    if (!osdfMetadata.varSliceDimName(columnName).empty()) {
      // This variable has a slice dimension, so it will be dimensioned by either
      // [<dim>] or [Location, <dim>]. Either way the osdfMetadata holds the proper
      // dimension names.
      varDimNames = osdfMetadata.getVarDimNames(columnName);
      associatedColumn = findColNameWithSliceSuffix(columnName, srcOsdf, osdfMetadata);
    } else {
      // This variable has no slice dimension, so it is dimensioned by Location only.
      varDimNames = std::vector<std::string>{"Location"};
      associatedColumn = columnName;
    }

    // Convert the OSDF column type to the corresponding netCDF type. Then
    // emplace the ojbect (pair) that goes into the list.
    const netCDF::NcType varNcType =
      convertOsdfDataTypeToNcType(srcOsdf->getColumnType(associatedColumn));
    const std::string varUnits = srcOsdf->getColumnUnits(associatedColumn);
    const VarCreationParameters varCreationParams(associatedColumn, varDimNames,
                                                  varNcType, varUnits);
    varCreationList.emplace_back(std::make_pair(columnName, varCreationParams));
  }
  return varCreationList;
}

//---------------------------------------------------------------------
netCDF::NcVar createHierNcVar(netCDF::NcGroup & topGroup,
                              const std::string & ncVarName,
                              const VarCreationParameters & createParams) {
  // Split the column to get the group structure for the variable.
  //    "A/B/C/myVar" -> "A", "B", "C", "myVar"
  //    Group structure is in the first n-1 entries (A/B/C) and the variable name
  //    is in the last entry (myVar).
  // Then walk through the group structure and create any groups that don't already exist.
  // Then create the variable.
  std::vector<std::string> groupVarList = ioda::splitString(ncVarName, '/');

  // Make sure we have at least one element. The last element is the variable
  // and the first n-1 elements are the group structure. Strip off the
  // variable name, then use the remaining elements to create and open the
  // desired group for the variable.
  ASSERT(groupVarList.size() > 0);
  const std::string varName = groupVarList.back();
  groupVarList.pop_back();

  // Walk the group structure creating groups as needed.
  netCDF::NcGroup group = topGroup;
  for (const auto & childGroupName : groupVarList) {
    if (group.getGroup(childGroupName).isNull()) {
      // Group doesn't exist -> create it
      group.addGroup(childGroupName);
    }
    // Move to the child group
    group = group.getGroup(childGroupName);
  }

  // group now is set to the desired group for the variable
  // convert the vector of dimension names to a vector of
  // netCDF::NcDim objects for the addVar call.
  std::vector<netCDF::NcDim> varDims;
  for (auto & dimName : createParams.dimNames) {
    netCDF::NcDim dim = topGroup.getDim(dimName);
    checkNcObj(dim, "ioda::writer::saveOsdfToNetcdf: Failed to open dimension: " + dimName);
    varDims.push_back(dim);
  }
  netCDF::NcVar var = group.addVar(varName, createParams.dataType, varDims);
  checkNcObj(var, "ioda::writer::saveOsdsToNetcdf: Failed to create variable: " + varName);
  setNcVarAttributes(var, createParams);
  return var;
}

//---------------------------------------------------------------------
template <typename VarType>
void setNcVarData(netCDF::NcVar & var, const std::vector<std::size_t> & starts,
                  const std::vector<std::size_t> & counts,
                  std::vector<VarType> & varData) {
  var.putVar(starts, counts, varData.data());
}

template <>
void setNcVarData(netCDF::NcVar & var, const std::vector<std::size_t> & starts,
                  const std::vector<std::size_t> & counts,
                  std::vector<std::string> & varData) {
  // The vector of strings contains a series of string objects in contiguous
  // memory. Each one of the string objects contains a char * pointing to the
  // string value in the heap. We need to pass to the putVar function
  // an array of char * pointers, so we first need to set up such an array.
  // This can be done by walking through the varData vector and copying
  // the pointer values from the string objects into a char * array.
  // One detail is that the varData needs to be non-const to be able to
  // copy the char * pointers.
  char * values[varData.size()];
  for (std::size_t i = 0; i < varData.size(); ++i) {
    values[i] = varData[i].data();
  }
  // The first two arguments are the start and count values that
  // describe how to extract a block of data from the values array.
  // In this case we want all the values so we use a start value
  // of 0, and a count equal to the size of the varData vector.
  var.putVar(starts, counts, values);
}

//--------------------------------------------------------------------------------
void setNcVar(netCDF::NcVar & var, const std::string & assocColumn,
              const std::unique_ptr<osdf::IFrame> & srcOsdf,
              const osdf::FrameMetadata & osdfMetadata,
              const std::size_t nlocsStart) {
  // First determine if this variable has channels. If so, get the list of channels
  // from the osdfMetadata, and use those to read the osdf columns and form those
  // values into data for the 2D variable (or 1D channel metadata variable)
  // in the output netcdf file.
  //
  // In any case, use the associated column name to select the proper column
  // out of the osdf that holds the values for the variable.
  const std::string varName = var.getName();

  // Need to strip off any numerical suffix on the associated column name
  // to look up the variable's slice dimension.
  const std::string colWithSlices = ioda::removeStringNumericSuffix(assocColumn);
  const std::string sliceDimName = osdfMetadata.varSliceDimName(colWithSlices);
  const std::size_t numLocs = srcOsdf->numRows();
  if (!sliceDimName.empty()) {
    // Use colWithSlices for the desired column name. Code will attach all of the slice
    // suffixes (the slice dimension's index values) to pull data from the srcOsdf.
    // Channel/slice-dimensioned data is identical on every pool rank, so it is always
    // written in full starting at 0; nlocsStart only ever applies to the Location axis.
    // For now assume we are always writing the entire variable data in one putVar call,
    // so the start value is always 0 and the count value is always the size of the varData vector.
    // We can generalize this in the future when we want to write the variable data in chunks.
    const std::vector<int> & sliceNums = osdfMetadata.getDimNums(sliceDimName);
    const std::size_t numSlices = sliceNums.size();
    if (osdfMetadata.getVarDimNames(colWithSlices).size() == 1) {
      // dimensions: [ <sliceDim> ]
      // The data in srcOsdf is stored in one column per slice. Each of these columns has the
      // data repeated for each location, so we only need to read from the first row of each
      // column to re-pack the original 1D array. If there are no locations (e.g. all
      // observations were filtered out), the per-slice value is unrecoverable, so we write the
      // dimension out with missing data.
      osdf::FrameUtils::callWithSupportedType(
        srcOsdf->getColumnType(assocColumn),
        [&](auto typeDiscriminator) {
          using T = decltype(typeDiscriminator);
          std::vector<T> varVals(numSlices);
          for (std::size_t islice = 0; islice < numSlices; ++islice) {
            if (numLocs > 0) {
              std::vector<T> sliceVals(numLocs);
              srcOsdf->getColumn(colWithSlices + "_" + std::to_string(sliceNums[islice]),
                                 sliceVals);
              varVals[islice] = sliceVals[0];
            } else {
              varVals[islice] = util::missingValue<T>();
            }
          }
          setNcVarData<T>(var, {0}, {numSlices}, varVals);
          });
    } else {
      // dimension: [ Location, <sliceDim> ]
      osdf::FrameUtils::callWithSupportedType(
        srcOsdf->getColumnType(assocColumn),
        [&](auto typeDiscriminator) {
          using T = decltype(typeDiscriminator);
          std::vector<T> varVals(numLocs * numSlices);
          for (std::size_t islice = 0; islice < numSlices; ++islice) {
            std::vector<T> sliceVals(numLocs);
            srcOsdf->getColumn(colWithSlices + "_" + std::to_string(sliceNums[islice]), sliceVals);
            for (std::size_t iloc = 0; iloc < numLocs; ++iloc) {
              const std::size_t ival =  (iloc * numSlices) + islice;
              varVals[ival] = sliceVals[iloc];
            }
          }
          setNcVarData<T>(var, {nlocsStart, 0}, {numLocs, numSlices}, varVals);
          });
    }
  } else {
    // dimensions: [ Location ]
    // Use assocColumn for the desired column name. Ie, leave numeric suffix
    // attached if there is one.
    osdf::FrameUtils::callWithSupportedType(
      srcOsdf->getColumnType(assocColumn),
      [&](auto typeDiscriminator) {
        using T = decltype(typeDiscriminator);
        std::vector<T> varVals(numLocs);
        srcOsdf->getColumn(assocColumn, varVals);
        setNcVarData<T>(var, {nlocsStart}, {numLocs}, varVals);
        });
  }
}

//--------------------------------------------------------------------------------
// Single-file write path: one pool rank writes at a time, in rank order
//--------------------------------------------------------------------------------
//
// Avoids parallel HDF5 entirely. Rank 0 creates the file and its full schema (sized for
// the pool-wide total row count) using the same plain netCDF::NcFile calls as the
// per-rank multi-file path above, writes its own slice of data, closes the file, and
// signals rank 1 via a trivial point-to-point token. Rank 1 reopens that same file,
// writes its own slice at its own row offset, closes, and signals rank 2 -- and so on
// through the last pool rank. Every write is an ordinary serial netCDF::NcFile/NcVar
// call, so real variable-length (NC_STRING) data is written directly -- no fixed-width
// workaround, no separate restore pass, no parallel-HDF5 API surface at all.

/// \brief compute this rank's row offset and the pool-wide total row count
/// \details Scoped to only the io pool ranks in ioPoolComm. No "assigned rank" information
/// needs to be used here since collectObs()/distributeObs() have already
/// physically relocated every row onto its destination pool rank before this function
/// is called, and ensured the distribution is non-overlapping.
static void collectSingleFileNlocsInfo(const eckit::mpi::Comm & ioPoolComm,
                                       const std::size_t localNlocs,
                                       std::size_t & totalNlocs,
                                       std::size_t & nlocsStart) {
  const std::size_t root = 0;
  std::vector<std::size_t> allNlocs(ioPoolComm.size());
  std::vector<std::size_t> allStarts(ioPoolComm.size());
  ioPoolComm.gather(localNlocs, allNlocs, root);
  if (ioPoolComm.rank() == root) {
    totalNlocs = 0;
    for (std::size_t i = 0; i < allNlocs.size(); ++i) {
      allStarts[i] = totalNlocs;
      totalNlocs += allNlocs[i];
    }
  }
  ioPoolComm.broadcast(totalNlocs, root);
  ioPoolComm.scatter(allStarts, nlocsStart, root);
}

/// \brief look up an already-created variable (specified as a hierarchical path)
/// \details Read-only counterpart to createHierNcVar: walks the same group path via
/// getGroup() (never creating groups) and returns the existing variable via getVar().
/// Used by every pool rank after the first, which look up the variables the first rank
/// already created in order to write their own slice of data into them.
static netCDF::NcVar getHierNcVar(netCDF::NcGroup & topGroup, const std::string & ncVarName) {
  std::vector<std::string> groupVarList = ioda::splitString(ncVarName, '/');
  ASSERT(groupVarList.size() > 0);
  const std::string varName = groupVarList.back();
  groupVarList.pop_back();

  netCDF::NcGroup group = topGroup;
  for (const auto & childGroupName : groupVarList) {
    group = group.getGroup(childGroupName);
    checkNcObj(group,
      "ioda::writer::saveOsdfToNetcdf: Failed to open group: " + childGroupName);
  }

  netCDF::NcVar var = group.getVar(varName);
  checkNcObj(var, "ioda::writer::saveOsdfToNetcdf: Failed to open variable: " + varName);
  return var;
}

/// \brief write the shared output file one pool rank at a time, in rank order
static void saveOsdfToNetcdfSingleFile(const eckit::mpi::Comm & ioPoolComm,
                                       std::unique_ptr<osdf::IFrame> & srcOsdf,
                                       osdf::FrameMetadata & osdfMetadata,
                                       const std::string & outputFileName) {
  std::size_t totalNlocs = 0;
  std::size_t nlocsStart = 0;
  collectSingleFileNlocsInfo(ioPoolComm, srcOsdf->numRows(), totalNlocs, nlocsStart);

  const std::size_t myRank = ioPoolComm.rank();
  const std::size_t poolSize = ioPoolComm.size();
  const int tokenTag = 0;

  // Wait for the previous rank to finish with the file before touching it. Rank 0 has
  // no predecessor, so it starts immediately.
  if (myRank > 0) {
    int token = 0;
    ioPoolComm.receive(token, myRank - 1, tokenTag);
  }

  if (srcOsdf->numCols() > 0) {
    if (myRank == 0) {
      // First rank: create the file and its full schema, sized for the pool-wide total,
      // then write this rank's own slice.
      netCDF::NcFile outFile(outputFileName, netCDF::NcFile::replace);
      checkNcObj(outFile,
        "ioda::writer::saveOsdfToNetcdf: Failed to create file: " + outputFileName);

      const DimCreationList dimCreationList = makeDimCreationList(srcOsdf, osdfMetadata);
      for (auto & dimCreationInfo : dimCreationList) {
        if (dimCreationInfo.first == "Location") {
          // Location is sized to the pool-wide total (unlike the multi-file path, which
          // sizes it per rank) since every rank shares this one file.
          createNcDim(outFile, "Location", DimCreationParameters(totalNlocs, true));
        } else {
          createNcDim(outFile, dimCreationInfo.first, dimCreationInfo.second);
        }
      }

      const VarCreationList dimVarCreationList = makeDimVarCreationList(srcOsdf, osdfMetadata);
      for (auto & dimVarCreate : dimVarCreationList) {
        netCDF::NcVar dimVar = createHierNcVar(outFile, dimVarCreate.first, dimVarCreate.second);
        setNcDimVar(dimVar, srcOsdf, osdfMetadata, nlocsStart);
      }

      const VarCreationList varCreationList = makeVarCreationList(srcOsdf, osdfMetadata);
      for (auto & varCreate : varCreationList) {
        netCDF::NcVar var = createHierNcVar(outFile, varCreate.first, varCreate.second);
        setNcVar(var, varCreate.second.assocColumn, srcOsdf, osdfMetadata, nlocsStart);
      }
      outFile.close();
    } else {
      // Every other rank: reopen the file the previous rank already created/populated,
      // look up the already-existing variables, and write only this rank's own slice.
      netCDF::NcFile outFile(outputFileName, netCDF::NcFile::write);
      checkNcObj(outFile,
        "ioda::writer::saveOsdfToNetcdf: Failed to open file: " + outputFileName);

      const VarCreationList dimVarCreationList = makeDimVarCreationList(srcOsdf, osdfMetadata);
      for (auto & dimVarCreate : dimVarCreationList) {
        netCDF::NcVar dimVar = getHierNcVar(outFile, dimVarCreate.first);
        setNcDimVar(dimVar, srcOsdf, osdfMetadata, nlocsStart);
      }

      const VarCreationList varCreationList = makeVarCreationList(srcOsdf, osdfMetadata);
      for (auto & varCreate : varCreationList) {
        netCDF::NcVar var = getHierNcVar(outFile, varCreate.first);
        setNcVar(var, varCreate.second.assocColumn, srcOsdf, osdfMetadata, nlocsStart);
      }
      outFile.close();
    }
  } else if (myRank == 0) {
    // No schema (no columns) defined on this osdf. Every pool rank is expected to agree
    // on this, since the schema is shared/replicated across the pool, so only rank 0
    // needs to write the special empty-file marker that ioda recognizes (Location
    // dimension of size 0, only the Location variable).
    netCDF::NcFile outFile(outputFileName, netCDF::NcFile::replace);
    checkNcObj(outFile,
      "ioda::writer::saveOsdfToNetcdf: Failed to create file: " + outputFileName);
    outFile.addDim("Location");
    const VarCreationParameters locVarCreateParams("Location", {"Location"},
                                                   netCDF::NcType::nc_INT64, "");
    createHierNcVar(outFile, "Location", locVarCreateParams);
    outFile.close();
  }

  // Signal the next rank that the file is now safe to reopen.
  if (myRank + 1 < poolSize) {
    int token = 0;
    ioPoolComm.send(token, myRank + 1, tokenTag);
  }

  ioPoolComm.barrier();
}

//--------------------------------------------------------------------------------
// Function definitions for public functions
//--------------------------------------------------------------------------------

void saveOsdfToNetcdf(const ObsDataOutParameters & dataOutParams,
                      const eckit::mpi::Comm & ioPoolComm,
                      std::unique_ptr<osdf::IFrame> & srcOsdf,
                      osdf::FrameMetadata & osdfMetadata) {
  // saveObs (caller of this function) has already verified that the
  // output file type is H5File (ie, netcdf).

  // Retrieve the file name, open the output file, and transfer the
  // data from srcOsdf to the output file. Print out the file name
  // before appending the io pool rank number. It seems a bit
  // cleaner to do it this way.
  std::string outputFileName =
    dataOutParams.engine.value().engineParameters.value().fileName.value();
  oops::Log::info() << "Saving to file: " << outputFileName << std::endl;

  // If we are writing multiple files, then we need to append
  // the io pool rank to the file name to get the unique file name for
  // this rank. If we are writing a single file, then all ranks will
  // write to the same file name.
  const bool writeMultipleFiles = dataOutParams.writeMultipleFiles.value();

  if (!writeMultipleFiles && ioPoolComm.size() > 1) {
    // Single shared file, written one pool rank at a time in rank order (see the
    // "Single-file write path" section above).
    saveOsdfToNetcdfSingleFile(ioPoolComm, srcOsdf, osdfMetadata, outputFileName);
    return;
  }

  // todo(SRH): For now we are ignoring the time communicator rank number.
  // We eventually need to support this for when we output files on
  // successive time steps. Setting timeCommRank to -1 will cause the output
  // file name to not include the time communicator rank number. Only append
  // the rank number if there is more than one rank in the io pool.
  if (ioPoolComm.size() > 1) {
    const int timeCommRank = -1;
    outputFileName = ioda::Engines::uniquifyFileName(outputFileName,
        writeMultipleFiles, ioPoolComm.rank(), timeCommRank);
  }

  // Open the output file, walk through the columns in srcOsdf, and
  // write, variable by variable, to the output file. Use the osdfMetadata
  // information to help sort out the channel dimensions on those variables
  // that use them.
  netCDF::NcFile outFile(outputFileName, netCDF::NcFile::replace);
  checkNcObj(outFile,
    "ioda::writer::saveOsdfToNetcdf: Failed to open/create file: " + outputFileName);

  if (srcOsdf->numCols() > 0) {
    // The osdf is not empty so we transfer the data to the output file.
    //
    // Create dimensions. Dimensions always go in the top level.
    // For now we are handling:
    //  1. Location - values are in the sourceLocationIndices column
    //                create the Location dimension with unlimited size
    //  2. Channel - values are in the osdfMetadata.chanNums vector
    const DimCreationList dimCreationList = makeDimCreationList(srcOsdf, osdfMetadata);
    for (auto & dimCreationInfo : dimCreationList) {
      createNcDim(outFile, dimCreationInfo.first, dimCreationInfo.second);
    }
    const VarCreationList dimVarCreationList = makeDimVarCreationList(srcOsdf, osdfMetadata);
    for (auto & dimVarCreate : dimVarCreationList) {
      netCDF::NcVar dimVar =
        createHierNcVar(outFile, dimVarCreate.first, dimVarCreate.second);
      setNcDimVar(dimVar, srcOsdf, osdfMetadata);
    }

    // Form a list of variable names with their associated dimension names.
    // Loop through the list and create the variables in the output file.
    // We are expecting three types of variables for now:
    //  1. The variables that will be dimensioned by Location
    //  2. The variables that will be dimensioned by Channel
    //  3. The variables that will be dimensioned by Location and Channel
    const VarCreationList varCreationList = makeVarCreationList(srcOsdf, osdfMetadata);
    for (auto & varCreate : varCreationList) {
      netCDF::NcVar var =
        createHierNcVar(outFile, varCreate.first, varCreate.second);
      setNcVar(var, varCreate.second.assocColumn, srcOsdf, osdfMetadata);
    }
  } else {
    // The osdf is empty, so we write an empty file that has only the Location
    // dimension (with size 0) and only the Location variable. This is a special
    // format that ioda recognizes which will create an empty ObsSpace when
    // read in.
    std::string locName("Location");
    outFile.addDim(locName);

    VarCreationParameters locVarCreateParams(locName, {locName}, netCDF::NcType::nc_INT64, "");
    createHierNcVar(outFile, locName, locVarCreateParams);
  }

  outFile.close();
}

}  // namespace writer
}  // namespace ioda
