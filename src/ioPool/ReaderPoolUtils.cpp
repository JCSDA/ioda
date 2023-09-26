/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file ReaderUtils.cpp
/// \brief Utilities for a ioda io reader backend

#include "ioda/ioPool/ReaderPoolUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <set>
#include <sstream>

#include "gsl/gsl-lite.hpp"

#include "eckit/geometry/Point2.h"
#include "eckit/mpi/Comm.h"
#include "eckit/config/YAMLConfiguration.h"

#include "ioda/Attributes/AttrUtils.h"
#include "ioda/defs.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/Copying.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/WriterFactory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/Variables/Fill.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace IoPool {

namespace detail {
  using std::to_string;

  /// An overload of to_string() taking a string and returning the same string.
  std::string to_string(std::string s) {
    return s;
  }
}  // namespace detail

//--------------------------------------------------------------------------------
// Tag values need to be non-negative, so reserve the values from zero to n for
// sending/receiving auxiliary data besides variable data. Then start the variable
// numbering from n+1 to generate unique tag values for the variable data transfers.
static const int msgIsSize = 1;
static const int msgIsData = 2;

static const int msgIsVariableSize = 0;
static const int mpiVariableNumberStart = 1;

//--------------------------------------------------------------------------------
// Function declarations for "private" functions
//--------------------------------------------------------------------------------

/// @brief Transfer source variable data into source buffer while replacing fill with missing
/// @param ioPool reader io pool object
/// @param srcVar source variable object
/// @param srcVarName source variable name
/// @param srcBuffer source memory buffer to hold srcVar data
static void readerLoadSourceVarReplaceFill(
            const ReaderPoolBase & ioPool, const Variable & srcVar,
            const std::string & srcVarName, std::vector<char> & srcBuffer);

/// @brief Transfer data from the source buffer to the destination variable
/// @param varName variable name
/// @param srcBuffer source memory buffer holding the source variable data
/// @param index vector of location indices used for data selection
/// @param destNlocs number of locations in destination group
/// @param doLocSelection when true apply location index selection
/// @param destBuffer destination memory buffer, holds values after index selection
/// @param destVar destitination variable
static void readerSaveDestVarLocal(
        const std::string & varName, const std::vector<char> & srcBuffer,
        const std::vector<std::size_t> & index, const ioda::Dimensions_t destNlocs,
        const bool doLocSelection, std::vector<char> & destBuffer, ioda::Variable & destVar);

/// @brief determine if location selection is required for the given variable
/// @param srcNlocs number of locations in source group
/// @param destNlocs number of locations in destination group
/// @param varName variable name
/// @param firstDimName name of first dimension of variable
static bool setDoLocSelection(
        const ioda::Dimensions_t & srcNlocs, const ioda::Dimensions_t & destNlocs,
        const std::string & varName, const std::string & firstDimName);

/// @brief apply location index selection going from srcBuffer to destBuffer
/// @param srcBuffer source memory buffer holding the source variable data
/// @param locIndices location index values
/// @param dataTypeSize size (in bytes) of variable data type
/// @param varShape variable shape (number of elements along each dimension)
/// @param destBuffer destination memory buffer, holds values after index selection
template <class DataType>
void selectVarValues(const std::vector<DataType> & srcBuffer,
                     const std::vector<std::size_t> & locIndices,
                     const ioda::Dimensions_t dataTypeSize,
                     const std::vector<ioda::Dimensions_t> & varShape,
                     std::vector<DataType> & destBuffer);

/// @brief get the size (in bytes) of DataType
template <class DataType>
ioda::Dimensions_t getDataTypeSize();

/// @brief template specialization for strings
template <>
ioda::Dimensions_t getDataTypeSize<std::string>();

/// @brief calculate the maximum data type size from the set of all supported data types
ioda::Dimensions_t getMaxDataTypeSize();

//--------------------------------------------------------------------------------
// Definitions of private functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
template<typename DataType>
DataType getMissingValue() {
    DataType missVal = util::missingValue<DataType>();
    return missVal;
}

//--------------------------------------------------------------------------------
// These are special cases to get a char * pointer for the string missing value
// to persist through the lifetime of the reader pool.
template<typename DataType>
DataType getMissingValue(const ReaderPoolBase & ioPool) {
    DataType missVal = util::missingValue<DataType>();
    return missVal;
}

template<>
std::shared_ptr<std::string> getMissingValue(const ReaderPoolBase & ioPool) {
    std::shared_ptr<std::string> missVal = ioPool.stringMissingValue();
    return missVal;
}

//--------------------------------------------------------------------------------
// Special case for broadcasting a DateTimeFormat enum type via eckit broadcast.
void broadcastDateTimeFormat(const eckit::mpi::Comm & comm, DateTimeFormat & enumVar,
                             const int root) {
    int tempInt;
    if (comm.rank() == root) {
        // Send enum as int since eckit MPI broadcast doesn't accept enum types
        tempInt = static_cast<int>(enumVar);
        comm.broadcast(tempInt, root);
    } else {
        comm.broadcast(tempInt, root);
        enumVar = static_cast<DateTimeFormat>(tempInt);
    }
}

//--------------------------------------------------------------------------------
void convertEpochStringToDtime(const std::string & epochString, util::DateTime & epochDtime) {
     // expected format is: "seconds since YYYY-MM-DDThh:mm:ssZ"
     std::size_t pos = epochString.find_last_of(" ");
     std::string epochUnits = epochString.substr(0, pos);
     std::string epochDtimeString = epochString.substr(pos + 1);
     if (epochUnits != "seconds since") {
         throw Exception("Date time epoch style units must start with 'seconds since'",
                         ioda_Here());
     }
     epochDtime = util::DateTime(epochDtimeString);
}

//--------------------------------------------------------------------------------
void checkForRequiredVars(const ioda::Group & srcGroup, const eckit::mpi::Comm & commAll,
                          std::string & sourceName, DateTimeFormat & dtimeFormat,
                          bool & emptyFile) {
    if (commAll.rank() == 0) {
        // Get number of locations from obs source
        std::size_t sourceNlocs = srcGroup.vars.open("Location").getDimensions().dimsCur[0];
        emptyFile = false;
        if (sourceNlocs == 0) {
            emptyFile = true;
            oops::Log::warning() << "WARNING: Input file " << sourceName
                                 << " contains zero observations" << std::endl;
        }

        // Check to see which format the source data time is in. There are two old formats
        // that need to be obsoleted soon, plus the conventional format.
        //
        // Old formats
        //    offset:
        //        datetime refrenece is in global attribute "date_time"
        //        variable values are float offset from reference in hours
        //
        //    string:
        //        variable values are ISO 8601 formatted strings
        //
        // Conventional format
        //    epoch:
        //        datetime reference (epoch) is stored in variable attribute "units"
        //            value is "seconds since <dtime>" where <dtime> is an
        //            ISO 8601 formatted string
        //        variable values are int64_t holding offset in seconds from the epoch
        //
        // TODO(srh) For now the old formats will be automatically converted to the epoch
        // format before storing in the obs space container. Warnings will be issued if
        // an old format is being used. Eventually, we need to turn the warnings into
        // errors and only allow the epoch format moving forward.

        // Check for datetime formats with lowest precedence first. That way subsequent
        // (higher precedence) can override in case several formats exist in the file.
        dtimeFormat = DateTimeFormat::None;
        if (srcGroup.vars.exists("MetaData/time")) { dtimeFormat = DateTimeFormat::Offset; }
        if (srcGroup.vars.exists("MetaData/datetime")) { dtimeFormat = DateTimeFormat::String; }
        if (srcGroup.vars.exists("MetaData/dateTime")) { dtimeFormat = DateTimeFormat::Epoch; }

        // Check to see if required metadata variables exist
        bool haveRequiredMetadata = dtimeFormat != DateTimeFormat::None;
        haveRequiredMetadata = haveRequiredMetadata && srcGroup.vars.exists("MetaData/latitude");
        haveRequiredMetadata = haveRequiredMetadata && srcGroup.vars.exists("MetaData/longitude");

        // Only do this check if there are more than zero obs in the file (sourceNlocs > 0)
        // When a file does contain zero obs, we want to allow for an "empty" file with
        // no variables. This makes it easier for r2d2 to provide a valid "empty" file when there
        // are no obs available.
        if ((sourceNlocs > 0) && (!haveRequiredMetadata)) {
          std::string errorMsg =
              std::string("\nOne or more of the following metadata variables are missing ") +
              std::string("from the input obs data source:\n") +
              std::string("    MetaData/dateTime (preferred) or MetaData/datetime ") +
              std::string("or MetaData/time\n") +
              std::string("    MetaData/latitude\n") +
              std::string("    MetaData/longitude\n");
          throw Exception(errorMsg.c_str(), ioda_Here());
        }

        if (dtimeFormat == DateTimeFormat::String) {
          oops::Log::info() << "WARNING: string style datetime will cause performance degredation "
                            << "and will eventually be deprecated." << std::endl
                            << "WARNING: Please update your datetime data to the epoch style "
                            << "representation using the new variable: MetaData/dateTime."
                            << std::endl;
        }

        if (dtimeFormat == DateTimeFormat::Offset) {
          oops::Log::info() << "WARNING: the reference/offset style datetime will "
                            << "be deprecated soon."
                            << std::endl
                            << "WARNING: Please update your datetime data to the epoch style "
                            << "representation using the new variable: MetaData/dateTime."
                            << std::endl;
        }

        oops::mpi::broadcastString(commAll, sourceName, 0);
        broadcastDateTimeFormat(commAll, dtimeFormat, 0);
        oops::mpi::broadcastBool(commAll, emptyFile, 0);
    } else {
        oops::mpi::broadcastString(commAll, sourceName, 0);
        broadcastDateTimeFormat(commAll, dtimeFormat, 0);
        oops::mpi::broadcastBool(commAll, emptyFile, 0);
    }
}

//--------------------------------------------------------------------------------
void readSourceDtimeVar(const ioda::Group & srcGroup, const eckit::mpi::Comm & commAll,
                        const bool emptyFile, const DateTimeFormat dtimeFormat,
                        std::vector<int64_t> & dtimeVals, std::string & dtimeEpoch) {
    // Initialize the output variables to values corresponding to an empty file. That way
    // if we have an empty file, then we can skip the file read and broadcast steps.
    dtimeVals.resize(0);
    dtimeEpoch = "seconds since 1970-01-01T00:00:00Z";

    if (!emptyFile) {
        if (commAll.rank() == 0) {
            // Read in variable data (converting if necessary) and determine epoch value
            ioda::Variable dtimeVar;
            if (dtimeFormat == DateTimeFormat::Epoch) {
                // Simply read in var values and copy the units attribute
                dtimeVar = srcGroup.vars.open("MetaData/dateTime");
                dtimeVar.atts.open("units").read<std::string>(dtimeEpoch);
                dtimeVar.read<int64_t>(dtimeVals);
            } else if (dtimeFormat == DateTimeFormat::String) {
                // Set the epoch to the linux standard epoch
                std::string epochDtimeString = std::string("1970-01-01T00:00:00Z");
                dtimeEpoch = std::string("seconds since ") + epochDtimeString;

                std::vector<std::string> dtStrings;
                dtimeVar = srcGroup.vars.open("MetaData/datetime");
                dtimeVar.read<std::string>(dtStrings);

                util::DateTime epochDtime(epochDtimeString);
                dtimeVals =  convertDtStringsToTimeOffsets(epochDtime, dtStrings);
            } else if (dtimeFormat == DateTimeFormat::Offset) {
                // Set the epoch to the "date_time" global attribute
                int refDtimeInt;
                srcGroup.atts.open("date_time").read<int>(refDtimeInt);

                int year = refDtimeInt / 1000000;     // refDtimeInt contains YYYYMMDDhh
                int tempInt = refDtimeInt % 1000000;
                int month = tempInt / 10000;       // tempInt contains MMDDhh
                tempInt = tempInt % 10000;
                int day = tempInt / 100;           // tempInt contains DDhh
                int hour = tempInt % 100;
                util::DateTime refDtime(year, month, day, hour, 0, 0);

                dtimeEpoch = std::string("seconds since ") + refDtime.toString();

                std::vector<float> dtTimeOffsets;
                dtimeVar = srcGroup.vars.open("MetaData/time");
                dtimeVar.read<float>(dtTimeOffsets);
                dtimeVals.resize(dtTimeOffsets.size());
                for (std::size_t i = 0; i < dtTimeOffsets.size(); ++i) {
                    dtimeVals[i] = static_cast<int64_t>(lround(dtTimeOffsets[i] * 3600.0));
                }
            }

            oops::mpi::broadcastVector<int64_t>(commAll, dtimeVals, 0);
            oops::mpi::broadcastString(commAll, dtimeEpoch, 0);
        } else {
            oops::mpi::broadcastVector<int64_t>(commAll, dtimeVals, 0);
            oops::mpi::broadcastString(commAll, dtimeEpoch, 0);
        }
    }
}

//--------------------------------------------------------------------------------
void initSourceIndices(const ioda::Group & srcGroup, const eckit::mpi::Comm & commAll,
        const bool emptyFile, const std::vector<int64_t> & dtimeValues,
        const int64_t windowStart, const int64_t windowEnd, const bool applyLocCheck,
        std::vector<float> & lonValues, std::vector<float> & latValues,
        std::vector<std::size_t> & sourceLocIndices,
        std::size_t & srcNlocs, std::size_t & srcNlocsInsideTimeWindow,
        std::size_t & srcNlocsOutsideTimeWindow, std::size_t & srcNlocsRejectQc,
        std::size_t & globalNlocs) {
    // Initialize the output variables to values corresponding to an empty file. That way
    // if we have an empty file, then we can skip the file read and broadcast steps.
    lonValues.resize(0);
    latValues.resize(0);
    sourceLocIndices.resize(0);
    srcNlocs = 0;
    srcNlocsInsideTimeWindow = 0;
    srcNlocsOutsideTimeWindow = 0;
    srcNlocsRejectQc = 0;
    globalNlocs = 0;

    if (!emptyFile) {
        if (commAll.rank() == 0) {
            // The existence of the datetime, longitude and latitude variables has been
            // verified at this point. Also, the datetime data has been converted to
            // the epoch format if that was necessary. Need to read in the lon and lat
            // values and save them for downstream processing.
            Variable lonVar = srcGroup.vars.open("MetaData/longitude");
            lonVar.read<float>(lonValues);
            Variable latVar = srcGroup.vars.open("MetaData/latitude");
            latVar.read<float>(latValues);

            // Note that the time window filter hasn't been applied yet so sourceLocIndices
            // can possibly be larger here than it should be after the time window filter
            // is applied. Make sure all of the MPI ranks have sourceLocIndices set to the
            // same size for the broadcast below. Also make sure that the resulting
            // sourceLocIndices is sized appropriately on all ranks.
            srcNlocs = dtimeValues.size();
            sourceLocIndices.resize(srcNlocs);
            srcNlocsInsideTimeWindow = 0;
            srcNlocsOutsideTimeWindow = 0;
            srcNlocsRejectQc = 0;
            globalNlocs = 0;

            if (applyLocCheck) {
                // Currently have two filters:
                //    1. Remove locations outside the timing window
                //    2. Remove locations that have mising values in either of lon or lat

                // Need the fill values for lon and lat to do the second check
                ioda::detail::FillValueData_t lonFvData = lonVar.getFillValue();
                float lonFillValue = ioda::detail::getFillValue<float>(lonFvData);
                ioda::detail::FillValueData_t latFvData = latVar.getFillValue();
                float latFillValue = ioda::detail::getFillValue<float>(latFvData);

                // Keep all locations that fall inside the timing window. Note numLocsSelecte
                // will be set to the number of locations stored in the output vectors after
                // exiting the following for loop.
                for (std::size_t i = 0; i < dtimeValues.size(); ++i) {
                    // Check the timing window first since having a location outside the timing
                    // window likely occurs more than having issues with the lat and lon values.
                    // Note that a datetime matching the window start will be rejects. This is
                    // done to prevent such a datetime appearing in two adjecnt windows.
                    bool keepThisLocation =
                        ((dtimeValues[i] > windowStart) && (dtimeValues[i] <= windowEnd));
                    if (keepThisLocation) {
                        // Keep count of how many obs fall inside the time window
                        srcNlocsInsideTimeWindow++;
                        if ((lonValues[i] == lonFillValue) || (latValues[i] == latFillValue)) {
                            // Keep count of how many obs get rejected by QC checks
                            srcNlocsRejectQc++;
                            keepThisLocation = false;
                        }
                    } else {
                        // Keep a count of how many obs were rejected due to being outside
                        // the timing window
                       srcNlocsOutsideTimeWindow++;
                    }

                    // Obs has passed all of the quality checks so add it to the list of records
                    if (keepThisLocation) {
                        sourceLocIndices[globalNlocs] = i;
                        globalNlocs++;
                    }
                }
            } else {
                // Skipping QC checks so set sourceLocIndices to all of the locations.
                std::iota(sourceLocIndices.begin(), sourceLocIndices.end(), 0);
                globalNlocs = sourceLocIndices.size();
                srcNlocsInsideTimeWindow = globalNlocs;
            }
            // At this point:
            //   srcNlocs == the original total number of locations in the obs source.
            //   srcNlocsInsideTimeWindow == the number of locations in the obs source that
            //                               fall inside the time window.
            //   srcNlocsOutsideTimeWindow == the number of locations in the obs source that
            //                                fall outside the time window.
            //   srcNlocsRejectQc == the number of locations in the obs source that
            //                       got rejected by the QC checks
            //   globalNlocs == the number of locations that made it through the time window
            //                  filter and the check on lat, lon for missing values
            //   sourceLocIndices is sized with the original total number of locations in the
            //                    obs source
            //
            // We need to resize sourceLocIndices to globalNlocs since this vector's size
            // is used to set the local number of nlocs for the obs space on this MPI task.
            sourceLocIndices.resize(globalNlocs);

            oops::mpi::broadcastVector<float>(commAll, lonValues, 0);
            oops::mpi::broadcastVector<float>(commAll, latValues, 0);
            oops::mpi::broadcastVector<std::size_t>(commAll, sourceLocIndices, 0);
            commAll.broadcast(srcNlocs, 0);
            commAll.broadcast(srcNlocsInsideTimeWindow, 0);
            commAll.broadcast(srcNlocsOutsideTimeWindow, 0);
            commAll.broadcast(srcNlocsRejectQc, 0);
            commAll.broadcast(globalNlocs, 0);
        } else {
            oops::mpi::broadcastVector<float>(commAll, lonValues, 0);
            oops::mpi::broadcastVector<float>(commAll, latValues, 0);
            oops::mpi::broadcastVector<std::size_t>(commAll, sourceLocIndices, 0);
            commAll.broadcast(srcNlocs, 0);
            commAll.broadcast(srcNlocsInsideTimeWindow, 0);
            commAll.broadcast(srcNlocsOutsideTimeWindow, 0);
            commAll.broadcast(srcNlocsRejectQc, 0);
            commAll.broadcast(globalNlocs, 0);
        }
    }
}

//--------------------------------------------------------------------------------
void buildObsGroupingKeys(const ioda::Group & srcGroup,
                          const std::vector<int64_t> & dtimeValues,
                          const std::vector<float> & lonValues,
                          const std::vector<float> & latValues,
                          const std::vector<std::string> & obsGroupVarList,
                          const std::vector<std::size_t> & sourceLocIndices,
                          std::vector<std::string> & groupingKeys) {
    // Get the number of locations in srcGroup for a simple (but fast) check below.
    // This is being done to avoid checking if the first dimension of a grouping
    // variable is Location through the HDF5 API. This check is a known performance
    // bottleneck.
    ioda::Dimensions_t numLocations = srcGroup.vars.open("Location").getDimensions().dimsCur[0];

    // Walk though each variable and construct the segments of the key values (strings)
    // Append the segments as each variable is encountered.
    for (std::size_t i = 0; i < obsGroupVarList.size(); ++i) {
        // Retrieve the variable values from the obs source and convert
        // those values to strings. Then append those "value" strings from each
        // variable to form the grouping keys.
        std::string obsGroupVarName = obsGroupVarList[i];
        if ((obsGroupVarName == "dateTime") || (obsGroupVarName == "longitude") ||
            (obsGroupVarName == "latitude")) {
            // Already have read in dateTime, lon and lat so go directly to their
            // values.
            std::string keySegment;
            for (std::size_t j = 0; j < sourceLocIndices.size(); ++j) {
                std::string keySegment;
                if (obsGroupVarName == "dateTime") {
                    keySegment = detail::to_string(dtimeValues[sourceLocIndices[j]]);
                } else if (obsGroupVarName == "longitude") {
                    keySegment = detail::to_string(lonValues[sourceLocIndices[j]]);
                } else {
                    keySegment = detail::to_string(latValues[sourceLocIndices[j]]);
                }
                if (i == 0) {
                    groupingKeys[j] = keySegment;
                } else {
                    groupingKeys[j] += ":";
                              groupingKeys[j] += keySegment;
                }
            }
        } else {
            std::string varName = std::string("MetaData/") + obsGroupVarName;
            Variable groupVar = srcGroup.vars.open(varName);
            if (groupVar.getDimensions().dimsCur[0] != numLocations) {
                std::string ErrMsg =
                    std::string("ERROR: buildObsGroupingKeys: ") +
                    std::string("obs grouping variable (") + obsGroupVarName +
                    std::string(") must have 'Location' as first dimension");
                Exception(ErrMsg.c_str(), ioda_Here());
            }

            VarUtils::forAnySupportedVariableType(
                  groupVar,
                  [&](auto typeDiscriminator) {
                      typedef decltype(typeDiscriminator) T;
                      std::vector<T> groupVarValues;
                      groupVar.read<T>(groupVarValues);
                      for (std::size_t j = 0; j < sourceLocIndices.size(); ++j) {
                          std::string keySegment =
                              detail::to_string(groupVarValues[sourceLocIndices[j]]);
                          if (i == 0) {
                              groupingKeys[j] = keySegment;
                          } else {
                              groupingKeys[j] += ":";
                              groupingKeys[j] += keySegment;
                          }
                      }
                  },
                  VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
        }
    }
}

//--------------------------------------------------------------------------------
void assignRecordNumbers(const ioda::Group & srcGroup, const eckit::mpi::Comm & commAll,
                         const bool emptyFile, const std::vector<int64_t> & dtimeValues,
                         const std::vector<float> & lonValues,
                         const std::vector<float> & latValues,
                         const std::vector<std::size_t> & sourceLocIndices,
                         const std::vector<std::string> & obsGroupVarList,
                         std::vector<std::size_t> & sourceRecNums) {
    // Initialize the output variables to values corresponding to an empty file. That way
    // if we have an empty file, then we can skip the file read and broadcast steps.
    sourceRecNums.resize(0);

    if (!emptyFile) {
        if (commAll.rank() == 0) {
            // If the obsGroupVarList is empty, then the obs grouping feature is not being
            // used and the record number assignment can simply be sequential numbering
            // starting with zero. Otherwise, assign unique record numbers to each unique
            // combination of the values in the obsGroupVarList.
            std::size_t locSize = sourceLocIndices.size();
            sourceRecNums.resize(locSize);

            if (obsGroupVarList.size() == 0) {
                // Do not apply obs grouping. Simply assign sequential numbering.
                std::iota(sourceRecNums.begin(), sourceRecNums.end(), 0);
            } else {
                // Apply obs grouping. First convert all of the group variable data values for this
                // frame into string key values. This is done in one call to minimize accessing the
                // frame data for the grouping variables.
                std::vector<std::string> obsGroupingKeys(locSize);
                buildObsGroupingKeys(srcGroup, dtimeValues, lonValues, latValues,
                                     obsGroupVarList, sourceLocIndices, obsGroupingKeys);

                std::size_t recnum = 0;
                std::map<std::string, std::size_t> obsGroupingMap;
                for (std::size_t i = 0; i < locSize; ++i) {
                  if (obsGroupingMap.find(obsGroupingKeys[i]) == obsGroupingMap.end()) {
                    // key is not present in the map -> assign current record number to
                    // the current key and move to the next record number
                    obsGroupingMap.insert(
                        std::pair<std::string, std::size_t>(obsGroupingKeys[i], recnum));
                    recnum += 1;
                  }
                  sourceRecNums[i] = obsGroupingMap.at(obsGroupingKeys[i]);
                }
            }
            oops::mpi::broadcastVector<std::size_t>(commAll, sourceRecNums, 0);
        } else {
            oops::mpi::broadcastVector<std::size_t>(commAll, sourceRecNums, 0);
        }
    }
}

//------------------------------------------------------------------------------------
void applyMpiDistribution(const std::shared_ptr<Distribution> & dist, const bool emptyFile,
                          const std::vector<float> & lonValues,
                          const std::vector<float> & latValues,
                          const std::vector<std::size_t> & sourceLocIndices,
                          const std::vector<std::size_t> & sourceRecNums,
                          std::vector<std::size_t> & localLocIndices,
                          std::vector<std::size_t> & localRecNums,
                          std::size_t & localNlocs, std::size_t & localNrecs) {
    // Initialize the output variables to values corresponding to an empty file. That way
    // if we have an empty file, then we can skip the file read and broadcast steps.
    localLocIndices.resize(0);
    localRecNums.resize(0);
    localNlocs = 0;
    localNrecs = 0;

    if (!emptyFile) {
        // Walk through each location and record the index and record number that
        // the distribution object determines to keep.
        std::size_t rowNum = 0;
        std::size_t recNum = 0;
        std::set<std::size_t> uniqueRecNums;
        for (std::size_t i = 0; i < sourceLocIndices.size(); ++i) {
            rowNum = sourceLocIndices[i];
            recNum = sourceRecNums[i];

            eckit::geometry::Point2 point(lonValues[rowNum], latValues[rowNum]);

            dist->assignRecord(recNum, rowNum, point);

            if (dist->isMyRecord(recNum)) {
                localLocIndices.push_back(rowNum);
                localRecNums.push_back(recNum);
                uniqueRecNums.insert(recNum);
            }
        }
        localNlocs = localLocIndices.size();
        localNrecs = uniqueRecNums.size();
    }
}

//--------------------------------------------------------------------------------
void setIndexAndRecordNums(const ioda::Group & srcGroup, const eckit::mpi::Comm & commAll,
        const bool emptyFile, const std::shared_ptr<ioda::Distribution> & distribution,
        const std::vector<int64_t> & dtimeValues,
        const int64_t windowStart, const int64_t windowEnd, const bool applyLocCheck,
        const std::vector<std::string> & obsGroupVarList,
        std::vector<float> & lonValues, std::vector<float> & latValues,
        std::size_t & srcNlocs, std::size_t & srcNlocsInsideTimeWindow,
        std::size_t & srcNlocsOutsideTimeWindow, std::size_t & srcNlocsRejectQc,
        std::vector<std::size_t> & localLocIndices,
        std::vector<std::size_t> & localRecNums,
        std::size_t & globalNlocs, std::size_t & localNlocs, std::size_t & localNrecs) {
    // The initSourceIndices function will skip QC checks if applyLocCheck is false.
    // in this case the sourceLocIndices vector is initialized to the entire set from
    // the obs source. The initSourceIndices uses the lon and lat values so it
    // also will read in those values from the obs source.
    std::vector<std::size_t> sourceLocIndices;
    initSourceIndices(srcGroup, commAll, emptyFile, dtimeValues, windowStart, windowEnd,
                      applyLocCheck, lonValues, latValues, sourceLocIndices, srcNlocs,
                      srcNlocsInsideTimeWindow, srcNlocsOutsideTimeWindow,
                      srcNlocsRejectQc, globalNlocs);

    // Assign a record number for each entry in sourceLocIndices. Use the obs grouping
    // feature if obsGroupVarList is not empty. Otherwise assign sequential
    // nubmers starting with zero.
    std::vector<std::size_t> sourceRecNums;
    assignRecordNumbers(srcGroup, commAll, emptyFile, dtimeValues, lonValues, latValues,
                        sourceLocIndices, obsGroupVarList, sourceRecNums);

    // Apply the MPI distribution which will result in the setting of the local location
    // indices and their corresponding record numbers.
    applyMpiDistribution(distribution, emptyFile, lonValues, latValues, sourceLocIndices,
                         sourceRecNums, localLocIndices, localRecNums,
                         localNlocs, localNrecs);
}

//--------------------------------------------------------------------------------
void setDistributionMap(const ReaderPoolBase & ioPool,
                        const std::vector<std::size_t> & localLocIndices,
                        const std::vector<std::pair<int, int>> & rankAssignment,
                        ReaderDistributionMap & distributionMap) {
    // Note that all of the exchange will be done using the "All" communicator, and
    // we are simply using the "Pool" communicator to identify if this rank is a
    // member of the io pool.
    int dataSize;
    distributionMap.empty();
    if (ioPool.commPool() != nullptr) {
        // On an io pool member, need to collect the local source indices from
        // all of the associated non io pool members.
        for (auto & rankAssign : rankAssignment) {
            int fromRankNum = rankAssign.first;
            ioPool.commAll().receive(&dataSize, 1, fromRankNum, msgIsSize);
            distributionMap[fromRankNum].resize(dataSize);
            ioPool.commAll().receive(distributionMap[fromRankNum].data(), dataSize,
                                     fromRankNum, msgIsData);
        }
    } else {
        // On a non io pool member, need to send local source indices to the
        // associated io pool member. The rankAssignment structure should contain
        // only one rank.
        for (auto & rankAssign : rankAssignment) {
            int toRankNum = rankAssign.first;
            dataSize = localLocIndices.size();
            ioPool.commAll().send(&dataSize, 1, toRankNum, msgIsSize);
            ioPool.commAll().send(localLocIndices.data(), dataSize, toRankNum, msgIsData);
        }
    }
}

//--------------------------------------------------------------------------------
void readerCreateWorkDirectory(const std::string & workDirBase,
                               const std::string & fileName, std::string & workDir) {
    // workDirBase can be either empty or a valid path to a directory that will contain
    // the input file set.
    //
    // fileName can be from a generator or an input file. The generator backend will
    // supply a representative file name that will be used for the input file set.
    //
    // Set workDir to workDirBase if workDirBase is not empty, otherwise use the dirname
    // of the input file, or /tmp in the generator case. In both case, file or generator,
    // create a subdirectory based on the file name or generator type.
    const auto lastSlash = fileName.find_last_of("/");
    if (workDirBase == "") {
        // Use the directory portion of the file name
        if (lastSlash == std::string::npos) {
            // fileName has no directory path specified --> use current directory
            workDir = std::string(".");
        } else {
            workDir = fileName.substr(0, lastSlash);
        }
    } else {
        workDir = workDirBase;
    }

    // Make sure workDir at this point exists. Note we still need to create a subdirectory
    // of workDir, but we will use mkdtemp for that which does the directory creation after
    // generating the unique name.
    std::string sysCommand = std::string("mkdir -p ") + workDir;
    if (system(sysCommand.c_str()) != 0) {
        throw Exception(std::string("Could not execute: ") + sysCommand, ioda_Here());
    }

    // Generate the subdirectory name, based on the input file name,
    // and create the subdirectory using mkdtemp.
    std::string workDirTemplate = workDir;
    if (lastSlash == std::string::npos) {
        workDirTemplate += std::string("/") + fileName;
    } else {
        workDirTemplate += std::string("/") + fileName.substr(lastSlash + 1);
    }
    workDirTemplate += ".filesetXXXXXX";
    std::string workSubDir(mkdtemp(workDirTemplate.data()));
    if (workSubDir.empty()) {
        throw Exception(std::string("Could not build work subdirectory"), ioda_Here());
    }

    // mkdtemp creates the directory using mode 700, we want 755 so members in our group
    // and other can see the directory contents
    sysCommand = std::string("chmod 755 ") + workSubDir;
    if (system(sysCommand.c_str()) != 0) {
        throw Exception(std::string("Could not execute: ") + sysCommand, ioda_Here());
    }

    // return the full path to the work directory
    workDir = workSubDir;
}

//--------------------------------------------------------------------------------
void readerGatherAssociatedRanks(const ReaderPoolBase & ioPool,
                                 std::vector<int> & assocAllRanks,
                                 std::vector<int> & ioPoolRanks) {
    // First get the local associated rank. For the purposes of this function,
    // if you are on a rank that is in the io pool, your own rank number is the
    // associated rank. Otherwise, the associated rank is in the first (and only)
    // entry of your rankAssignment.
    int assocRank;
    int ioPoolRank;
    if (ioPool.commPool() != nullptr) {
        assocRank = ioPool.commAll().rank();
        ioPoolRank = ioPool.commPool()->rank();
    } else {
        assocRank = ioPool.rankAssignment()[0].first;
        ioPoolRank = -1;
    }
    assocAllRanks.resize(ioPool.commAll().size());
    ioPoolRanks.resize(ioPool.commAll().size());
    ioPool.commAll().allGather(assocRank, assocAllRanks.begin(), assocAllRanks.end());
    ioPool.commAll().allGather(ioPoolRank, ioPoolRanks.begin(), ioPoolRanks.end());
}

//--------------------------------------------------------------------------------
void readerGatherLocationIndices(const ReaderPoolBase & ioPool,
                                 std::vector<std::size_t> & locIndicesAllRanks,
                                 std::vector<int> & locIndicesStarts,
                                 std::vector<int> & locIndicesCounts) {
    // Gather the list of indices. This requires a variable length gather where the
    // lists of starts and counts are given to the MPI gatherv command.
    //
    // Gather up each rank's count of location indices
    const int numTasks = ioPool.commAll().size();
    const int myLocIndicesCount = ioPool.index().size();
    locIndicesCounts.resize(numTasks);
    ioPool.commAll().allGather(myLocIndicesCount,
                               locIndicesCounts.begin(), locIndicesCounts.end());

    // Calculate the starting point (in a variable gather of the location indices) of
    // each rank's vector of location indices. Then do a variable all gather of the
    // location indices.
    int recvSize = locIndicesCounts[0];
    locIndicesStarts.resize(numTasks);
    locIndicesStarts[0] = 0;
    for (std::size_t i = 1; i < numTasks; ++i) {
        locIndicesStarts[i] = locIndicesStarts[i - 1] + locIndicesCounts[i - 1];
        recvSize += locIndicesCounts[i];
    }
    locIndicesAllRanks.resize(recvSize);
    ioPool.commAll().allGatherv(ioPool.index().begin(), ioPool.index().end(),
                                locIndicesAllRanks.begin(), locIndicesCounts.data(),
                                locIndicesStarts.data());
}

//--------------------------------------------------------------------------------
void readerSetFileSelection(const int allRank,
                            const std::vector<int> & assocAllRanks,
                            const std::vector<std::size_t> & locIndicesAllRanks,
                            const std::vector<int> & locIndicesStarts,
                            const std::vector<int> & locIndicesCounts,
                            std::vector<std::size_t> & indices,
                            std::vector<int> & destAllRanks,
                            std::vector<int> & starts,
                            std::vector<int> & counts) {
    // Walk though the assocAllRanks and use the entries matching allRank to pull
    // out the indices associated with allRank. Generate the start, count values as
    // you go. Note the algorithm below is not very runtime efficient (eg. vector
    // push_back calls) and it is relying on the io pool not getting very
    // big (ie, 10s of ranks at the most).
    int start = 0;
    for (int i = 0; i < assocAllRanks.size(); ++i) {
        if (assocAllRanks[i] == allRank) {
            // Attach the entries in locIndices* to the output vectors.
            const int inputStart = locIndicesStarts[i];
            const int inputCount = locIndicesCounts[i];
            indices.insert(indices.end(), locIndicesAllRanks.begin() + inputStart,
                           locIndicesAllRanks.begin() + inputStart + inputCount);
            std::vector<int> dest(inputCount, i);
            destAllRanks.insert(destAllRanks.end(), dest.begin(), dest.end());
            starts.push_back(start);
            counts.push_back(inputCount);
            start += inputCount;
        }
    }
}

//--------------------------------------------------------------------------------
void readerWriteInputFileMpiMapping(const ReaderPoolBase & ioPool,
                                    const Group & srcGroup,
                                    const std::vector<std::size_t> & indices,
                                    const std::vector<int> & destAllRanks,
                                    Group & destGroup) {
    ASSERT(indices.size() == destAllRanks.size());
    const int nlocs = indices.size();

    // Create the Location dimension
    VariableCreationParameters int64Params = VariableCreationParameters::defaults<int64_t>();
    int64Params.setFillValue<int64_t>(util::missingValue<int64_t>());
    int64Params.noCompress();
    Variable destLocVar = destGroup.vars.create<int64_t>(
                          "Location", { nlocs }, { ioda::Unlimited }, int64Params);
    destLocVar.setIsDimensionScale("Location");
    Variable srcLocVar = srcGroup.vars.open("Location");

    // Create and the destAllRanks variable in the top level group.
    VariableCreationParameters intParams = VariableCreationParameters::defaults<int>();
    intParams.setFillValue<int>(util::missingValue<int>());
    intParams.noCompress();
    Variable destRankVar = destGroup.vars.createWithScales<int>(
                           "destinationRank", { destLocVar }, intParams);

    // Only write out the values if nlocs > zero
    if (nlocs > 0) {
        // Location
        // Conversion from size_t to int64_t should be safe. Only unsafe for values greater
        // than ~1/2 the max size_t value.
        std::vector<int64_t> destInt64Values(indices.begin(), indices.end());
        destLocVar.write<int64_t>(destInt64Values);

        // destinationRank
        destRankVar.write<int>(destAllRanks);
    }
}

//--------------------------------------------------------------------------------
void readerWriteInputFilePreparedVars(const ReaderPoolBase & ioPool, const Group & srcGroup,
                                      const std::vector<std::size_t> & indices,
                                      const std::vector<int64_t> & dtimeValues,
                                      const std::string & dtimeEpoch,
                                      const std::vector<float> & lonValues,
                                      const std::vector<float> & latValues,
                                      Group & destGroup) {
    // Open the Location dimension variable. All of these variables are 1D MetaData variables
    // that are dimensioned by Location.
    Variable destLocVar = destGroup.vars.open("Location");

    const int destNlocs = indices.size();

    // dateTime
    VariableCreationParameters int64Params = VariableCreationParameters::defaults<int64_t>();
    int64Params.setFillValue<int64_t>(util::missingValue<int64_t>());
    int64Params.noCompress();
    Variable dateTimeVar = destGroup.vars.createWithScales<int64_t>(
                           "MetaData/dateTime", { destLocVar }, int64Params);
    dateTimeVar.atts.add<std::string>("units", dtimeEpoch);

    // longitude
    VariableCreationParameters floatParams = VariableCreationParameters::defaults<float>();
    floatParams.setFillValue<float>(util::missingValue<float>());
    floatParams.noCompress();
    Variable destLonVar = destGroup.vars.createWithScales<float>(
                          "MetaData/longitude", { destLocVar }, floatParams);
    if (srcGroup.vars.exists("MetaData/longitude")) {
        Variable srcLonVar = srcGroup.vars.open("MetaData/longitude");
        copyAttributes(srcLonVar.atts, destLonVar.atts);
    }

    // latitude
    Variable destLatVar = destGroup.vars.createWithScales<float>(
                          "MetaData/latitude", { destLocVar }, floatParams);
    if (srcGroup.vars.exists("MetaData/latitude")) {
        Variable srcLatVar = srcGroup.vars.open("MetaData/latitude");
        copyAttributes(srcLatVar.atts, destLatVar.atts);
    }

    // Only write out the data when destNlocs > zero
    if (destNlocs > 0) {
        // Allocate buffers for variable data selection
        std::vector<int64_t> destInt64Values(destNlocs);
        std::vector<float> destFloatValues(destNlocs);

        // dateTime
        selectVarValues<int64_t>(dtimeValues, indices, 1, { destNlocs }, destInt64Values);
        dateTimeVar.write<int64_t>(destInt64Values);

        // longitude
        selectVarValues<float>(lonValues, indices, 1, { destNlocs }, destFloatValues);
        destLonVar.write<float>(destFloatValues);

        // latitude
        selectVarValues<float>(latValues, indices, 1, { destNlocs }, destFloatValues);
        destLatVar.write<float>(destFloatValues);
    }
}

//--------------------------------------------------------------------------------
void readerCreateInputFileVariables(const VarUtils::Vec_Named_Variable & dimVarList,
                                    const VarUtils::VarDimMap & dimsAttachedToVars,
                                    Group & destGroup) {
    // Create dimensions according to those from the original input file.
    // Note that the Location variable has already been created.
    for (const auto & dimNamedVar : dimVarList) {
        std::string dimName = dimNamedVar.name;
        if (dimName == "Location") {
            continue;
        }
        oops::Log::trace() << "readerCreateInputFileVariables: creating: "
                           << dimName << std::endl;

        // Create the varible, mark as dimension, and copy the attributes
        Variable srcDimVar = dimNamedVar.var;
        Dimensions_t dimSize = srcDimVar.getDimensions().dimsCur[0];
        Variable destDimVar;
        VarUtils::forAnySupportedVariableType(
            srcDimVar,
            [&](auto typeDiscriminator) {
                typedef decltype(typeDiscriminator) T;
                VariableCreationParameters params = VariableCreationParameters::defaults<T>();
                params.setFillValue<T>(getMissingValue<T>());
                // Don't want compression in the memory image.
                params.noCompress();
                destDimVar = destGroup.vars
                    .create<T>(dimName, { dimSize }, { dimSize }, params)
                    .setIsDimensionScale(dimName);
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(dimName));
        copyAttributes(srcDimVar.atts, destDimVar.atts);
    }

    // Create the regular variables according to those from the original input file.
    // Note that dateTime, latitude, longitude have already been created.
    // The old date time names can show up and we want to ignore those too.
    for (const auto & varObject : dimsAttachedToVars) {
        std::string varName = varObject.first.name;
        if ((varName == "MetaData/dateTime") || (varName == "MetaData/datetime") ||
            (varName == "MetaData/time") || (varName == "MetaData/longitude") ||
            (varName == "MetaData/latitude")) {
            continue;
        }
        oops::Log::trace() << "readerCreateInputFileVariables: creating: "
                           << varName << std::endl;

        // Create a list of the dimensions to attach to the variable.
        std::vector<Variable> dimVars;
        for (const auto & dimNamedVar : varObject.second) {
            std::string dimName = dimNamedVar.name;
            dimVars.push_back(destGroup.vars.open(dimName));
        }

        // Create the variable and copy the attributes
        Variable srcVar = varObject.first.var;
        Variable destVar;
        VarUtils::forAnySupportedVariableType(
            srcVar,
            [&](auto typeDiscriminator) {
                typedef decltype(typeDiscriminator) T;
                VariableCreationParameters params = VariableCreationParameters::defaults<T>();
                params.setFillValue<T>(getMissingValue<T>());
                // Don't want compression in the memory image.
                params.noCompress();
                destVar = destGroup.vars.createWithScales<T>(varName, dimVars, params);
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
        copyAttributes(srcVar.atts, destVar.atts);
    }
}

//--------------------------------------------------------------------------------
void getMaxNumElementsFromVarLists(const VarUtils::Vec_Named_Variable & dimVarList,
                                   const VarUtils::VarDimMap & dimsAttachedToVars,
                                   Dimensions_t & maxNumElements, Dimensions_t & nlocs) {
    maxNumElements = 0;
    nlocs = 0;
    // check dimension sizes
    for (const auto & dimNamedVar : dimVarList) {
        Dimensions_t numElements = dimNamedVar.var.getDimensions().numElements;
        if (numElements > maxNumElements) {
            maxNumElements = numElements;
        }
        if (dimNamedVar.name == "Location") {
            nlocs = numElements;
        }
    }
    // check var sizes
    for (const auto & varObject : dimsAttachedToVars) {
        Dimensions_t numElements = varObject.first.var.getDimensions().numElements;
        if (numElements > maxNumElements) {
            maxNumElements = numElements;
        }
    }
}

//--------------------------------------------------------------------------------
void getMaxNumElementsFromGroup(const Group & group,
                                Dimensions_t & maxNumElements, Dimensions_t & nlocs) {
    maxNumElements = 0;
    nlocs = 0;
    for (const auto & varName : group.listObjects<ObjectType::Variable>(true)) {
        Dimensions_t numElements = group.vars.open(varName).getDimensions().numElements;
        if (numElements > maxNumElements) {
            maxNumElements = numElements;
        }
        if (varName == "Location") {
            nlocs = numElements;
        }
    }
}

//--------------------------------------------------------------------------------
void readerInputFileTransferVarData(const ReaderPoolBase & ioPool,
                                    const VarUtils::Vec_Named_Variable & dimVarList,
                                    const VarUtils::VarDimMap & dimsAttachedToVars,
                                    const std::vector<std::size_t> & indices,
                                    Group & destGroup) {
    // Allocate two generic (vector of char) memory buffers, one for holding the source
    // variable data, the other for holding the destination variable data. We need to
    // avoid repeated allocations and deallocations to mitigate memory fragmentation so
    // we want to make sure we allocate enough space. For that we need to know the maximum
    // number of elements in all of the variables and the maximum data type size (in bytes).
    // Note that the getMaxNumElements* functions are also used to set srcNlocs and destNlocs.
    Dimensions_t srcNlocs;
    Dimensions_t destNlocs;
    Dimensions_t srcMaxNumElements;
    Dimensions_t destMaxNumElements;
    getMaxNumElementsFromVarLists(dimVarList, dimsAttachedToVars, srcMaxNumElements, srcNlocs);
    getMaxNumElementsFromGroup(destGroup, destMaxNumElements, destNlocs);
    Dimensions_t maxDataTypeSize = getMaxDataTypeSize();
    std::vector<char> srcBuffer(srcMaxNumElements * maxDataTypeSize);
    std::vector<char> destBuffer(destMaxNumElements * maxDataTypeSize);

    // Note that the Location variable has been taken care of already.
    for (const auto & dimNamedVar : dimVarList) {
        std::string dimName = dimNamedVar.name;
        if (dimName == "Location") {
            continue;
        }

        // All dimensions besides Location (which we are skipping) need to have their
        // entire contents transferred to the destination group (ie, no selection).
        // No need to transfer any data if numElements is zero. This also helps with
        // supporting the case of an empty source file.
        // Note that dimNamedVar.var is the source variable.
        // Fifth argument to readerSaveDestVarLocal is doLocSelection which is always
        // false in this case.
        Variable destDimVar = destGroup.vars.open(dimName);
        if (destDimVar.getDimensions().numElements > 0) {
            readerLoadSourceVarReplaceFill(ioPool, dimNamedVar.var, dimName, srcBuffer);
            readerSaveDestVarLocal(dimName, srcBuffer, indices, destNlocs, false,
                                   destBuffer, destDimVar);
        }
    }

    // Note that dateTime, latitude, longitude have already had their data transferred.
    // The old date time names can show up and we want to ignore those too.
    for (const auto & varObject : dimsAttachedToVars) {
        std::string varName = varObject.first.name;
        if ((varName == "MetaData/dateTime") || (varName == "MetaData/datetime") ||
            (varName == "MetaData/time") || (varName == "MetaData/longitude") ||
            (varName == "MetaData/latitude")) {
            continue;
        }

        // A variable that potentially needs location selection will have its
        // first dimension be Location. That is why the firstDimName is passed into
        // the setDoLocSelection function.
        const std::string firstDimName = varObject.second[0].name;
        const bool doLocSelection =
            setDoLocSelection(srcNlocs, destNlocs, varName, firstDimName);

        // Transfer the variable data. Skip the transfer if the destination variable
        // has zero elements. Saves some needless function calls, and helps support
        // the case of an empty source file.
        Variable destVar = destGroup.vars.open(varName);
        if (destVar.getDimensions().numElements > 0) {
            readerLoadSourceVarReplaceFill(ioPool, varObject.first.var, varName, srcBuffer);
            readerSaveDestVarLocal(varName, srcBuffer, indices, destNlocs, doLocSelection,
                                   destBuffer, destVar);
        }
    }
}

//--------------------------------------------------------------------------------
void readerBuildAssocInputFile(const ReaderPoolBase & ioPool, const Group & srcGroup,
                               const int allRank, const int poolRank,
                               const std::vector<std::size_t> & indices,
                               const std::vector<int> & destAllRanks,
                               const std::vector<int> & starts,
                               const std::vector<int> & counts,
                               const std::vector<int64_t> & dtimeValues,
                               const std::string & dtimeEpoch,
                               const std::vector<float> & lonValues,
                               const std::vector<float> & latValues) {
    // Construct the path, name to the new input file. Strip off the trailing suffix
    // (.nc4, .odb, .nc, etc) and replace with "_<ioPoolRank>.nc4". For now, we will
    // always use the hdf5 backend for these files.

    // Get "basename" of the path in fileName
    std::string inputFileName;
    auto pos = ioPool.fileName().find_last_of('/');
    if (pos == std::string::npos) {
        inputFileName = ioPool.fileName();
    } else {
        // fileName contains path information, strip off every character leading
        // up to the last '/'.
        inputFileName = ioPool.fileName().substr(pos + 1);
    }

    // Generate a file name using the poolRank number, and the time communicator
    // rank number. The second argument to uniquifyFileName is "write multiple files"
    // which needs to be true to allow uniquifyFileName to tag on the poolRank number.
    int timeRankNum = -1;
    if (ioPool.commTime().size() > 1) {
        timeRankNum = ioPool.commTime().rank();
    }
    inputFileName = ioPool.workDir() + std::string("/") + inputFileName;
    inputFileName = Engines::uniquifyFileName(inputFileName, true, poolRank, timeRankNum);
    oops::Log::trace() << "readerBuildAssocInputFile: inputFileName: "
                       << inputFileName << std::endl;

    // Open up an hdf5 writer backend and transfer the selected data into the
    // output file. We need to create a new eckit configuration for the writer
    // engine factory.
    // Third and fourth arguments to constructFileWriterFromConfig are
    // "write multiple files" and "is parallel" respectively. We want
    // "write multiple files" to be false since we are tagging on the io pool
    // rank above.
    eckit::LocalConfiguration engineConfig =
        Engines::constructFileBackendConfig("hdf5", inputFileName);
    std::unique_ptr<Engines::WriterBase> writerEngine =
        Engines::constructFileWriterFromConfig(ioPool.commAll(), ioPool.commTime(),
                                               false, false, engineConfig);
    Group destGroup = writerEngine->getObsGroup();

    // Copy the source group hierarchical structure (all subgroups and group attributes)
    copyGroupStructure(srcGroup, destGroup);

    // Write out the mpi mapping data held in indices and destAllRanks. Place
    // these variables in a special group in the output file.
    // Then write out the prepared variables (dateTime, latitude, longitude)
    readerWriteInputFileMpiMapping(ioPool, srcGroup, indices, destAllRanks, destGroup);
    readerWriteInputFilePreparedVars(ioPool, srcGroup, indices, dtimeValues, dtimeEpoch,
                                     lonValues, latValues, destGroup);

    // Collect, from the source group, variable lists and dimension mappings
    VarUtils::Vec_Named_Variable regularVarList;
    VarUtils::Vec_Named_Variable dimVarList;
    VarUtils::VarDimMap dimsAttachedToVars;
    Dimensions_t maxVarSize0;  // unused in this function
    VarUtils::collectVarDimInfo(srcGroup, regularVarList, dimVarList,
                                dimsAttachedToVars, maxVarSize0);

    // Create the remaining dimensions and variables. This will place everything in
    // the new input file except for the dimension and variable data values.
    readerCreateInputFileVariables(dimVarList, dimsAttachedToVars, destGroup);

    // Transfer the variable data to the destination group
    readerInputFileTransferVarData(ioPool, dimVarList, dimsAttachedToVars, indices, destGroup);
}

//--------------------------------------------------------------------------------
void readerBuildInputFiles(const ReaderPoolBase & ioPool,
                           const Group & srcGroup,
                           const std::vector<int> & assocAllRanks,
                           const std::vector<int> & ioPoolRanks,
                           const std::vector<std::size_t> & locIndicesAllRanks,
                           const std::vector<int> & locIndicesStarts,
                           const std::vector<int> & locIndicesCounts,
                           const std::vector<int64_t> & dtimeValues,
                           const std::string & dtimeEpoch,
                           const std::vector<float> & lonValues,
                           const std::vector<float> & latValues) {
    // First identify which ranks, from the commAll communicator group, are io pool members.
    // These are the unique values in assocAllRanks.
    std::set<int> ioPoolMembers(assocAllRanks.begin(), assocAllRanks.end());
    for (const auto & i : ioPoolMembers) {
        // For this io pool member, record the io pool rank (for the file suffix),
        // and the list of indices that go into the associated file (with the
        // corresponding start, count values).
        int allRank = i;
        int poolRank = ioPoolRanks[i];
        std::vector<int> starts;
        std::vector<int> counts;
        std::vector<std::size_t> indices;
        std::vector<int> destAllRanks;
        readerSetFileSelection(allRank, assocAllRanks, locIndicesAllRanks, locIndicesStarts,
                               locIndicesCounts, indices, destAllRanks, starts, counts);

        // Create the associate file
        readerBuildAssocInputFile(ioPool, srcGroup, allRank, poolRank, indices,
                                  destAllRanks, starts, counts, dtimeValues, dtimeEpoch,
                                  lonValues, latValues);
    }
}

//--------------------------------------------------------------------------------
void readerCreateFileSet(const ReaderPoolBase & ioPool, const Group & srcGroup,
                         const std::vector<int64_t> & dtimeValues,
                         const std::string & dtimeEpoch,
                         const std::vector<float> & lonValues,
                         const std::vector<float> & latValues) {
    // We want to gather all the rank assignments and indices on rank 0 in the all
    // communicator, since rank 0 is rank that has the input file open. This can be
    // obtained from pairing up which ranks are grouped together, and then tagging on
    // their location indices. Note, an entry in ioPoolRank for ranks not in the ioPool
    // will be set to -1.
    std::vector<int> assocAllRanks;
    std::vector<int> ioPoolRanks;
    readerGatherAssociatedRanks(ioPool, assocAllRanks, ioPoolRanks);

    std::vector<std::size_t> locIndicesAllRanks;
    std::vector<int> locIndicesStarts;
    std::vector<int> locIndicesCounts;
    readerGatherLocationIndices(ioPool, locIndicesAllRanks,
                                locIndicesStarts, locIndicesCounts);

    // Build the files. One file per io pool member, location indices in the order of
    // the rank assignments starting with the pool member rank. Only rank 0 has the
    // input file open, and rank 0 has all the information needed to create
    // all of the io pool member files. So, have just rank 0 take it from here.
    if (ioPool.commAll().rank() == 0) {
        readerBuildInputFiles(ioPool, srcGroup, assocAllRanks, ioPoolRanks,
                              locIndicesAllRanks, locIndicesStarts, locIndicesCounts,
                              dtimeValues, dtimeEpoch, lonValues, latValues);
    }

    // Have the other MPI ranks wait for rank 0 to create the input files
    ioPool.commAll().barrier();
}

//--------------------------------------------------------------------------------
void readerSerializeGroupStructure(const ReaderPoolBase & ioPool,
                                   const ioda::Group & fileGroup,
                                   const bool emptyFile,
                                   std::string & groupStructureYaml) {
    // Have the pool member query the file to get the group structure, then
    // serialize to yaml into a string (using a stringstream) and then
    // use MPI send/receive to distribute the yaml string to the assigned ranks.
    //
    // If we have an empty file (source nlocs == zero), then only list out the
    // single dimension Location. Otherwise inspect the input file and dump
    // out according to what is found in the input file.
    if (ioPool.commPool() != nullptr) {
        std::stringstream yamlStream;
        if (emptyFile) {
            // list out the one dimension (Location) of zero size.
            yamlStream << "dimensions:" << std::endl
                       << constants::indent4 << "- dimension:" << std::endl
                       << constants::indent8 << "name: Location" << std::endl
                       << constants::indent8 << "data type: int" << std::endl
                       << constants::indent8 << "size: 0" << std::endl;
        } else {
            // First describe the group structure, list out group names and attributes
            // associated with those groups.

            // Top level group attributes
            AttrUtils::listAttributesAsYaml(fileGroup.atts, constants::indent0, yamlStream);

            const auto groupObjects = fileGroup.listObjects(ObjectType::Group, true);
            yamlStream << "groups:" << std::endl;
            for (const auto & groupName : groupObjects.at(ObjectType::Group)) {
                yamlStream << constants::indent4 << "- group:" << std::endl
                           << constants::indent8 << "name: " << groupName << std::endl;
                // subgroup attributes
                AttrUtils::listAttributesAsYaml(fileGroup.open(groupName).atts,
                                                constants::indent8, yamlStream);
            }

            // query fileGroup for variable lists and dimension mappings
            VarUtils::Vec_Named_Variable regularVarList;
            VarUtils::Vec_Named_Variable dimVarList;
            VarUtils::VarDimMap dimsAttachedToVars;
            Dimensions_t maxVarSize0;  // unused in this function
            VarUtils::collectVarDimInfo(fileGroup, regularVarList, dimVarList,
                                        dimsAttachedToVars, maxVarSize0);

            // List out dimension variables (these all belong in the top level group).
            yamlStream << "dimensions:" << std::endl;
            VarUtils::listDimensionsAsYaml(dimVarList, constants::indent4, yamlStream);

            // List out regular variables
            yamlStream << "variables:" << std::endl;
            VarUtils::listVariablesAsYaml(regularVarList, dimsAttachedToVars,
                                          constants::indent4, yamlStream);
        }

        // convert the stream to a string and send it to the assigned ranks
        groupStructureYaml = yamlStream.str();
        for (auto & rankAssign : ioPool.rankAssignment()) {
            oops::mpi::sendString(ioPool.commAll(), groupStructureYaml, rankAssign.first);
        }
    } else {
        // On a non pool task
        for (auto & rankAssign : ioPool.rankAssignment()) {
            oops::mpi::receiveString(ioPool.commAll(), groupStructureYaml, rankAssign.first);
        }
    }
}

//--------------------------------------------------------------------------------
void readerDefineYamlAnchors(const ReaderPoolBase & ioPool,
                             std::string & groupStructureYaml) {
    std::stringstream yamlStream;
    yamlStream << "definitions:" << std::endl;

    // Anchor for number of locations: &numLocations
    // Each MPI task has its own number of locations. The input yaml has an
    // alias (*numLocations) in its definition, and this routine will add the
    // anchor (&numLocations) that goes with that alias. This way the number of
    // locations can change on a task-by-task basis.
    yamlStream << constants::indent4 << "number locations: &numLocations "
               << ioPool.nlocs() << std::endl;

    // Anchor for the dateTime epoch value: &dtimeEpoch
    yamlStream << constants::indent4 << "dtime epoch: &dtimeEpoch \""
               << ioPool.dtimeEpoch() << "\"" << std::endl;

    // prepend the definitions section with the anchors to the group structure YAML
    groupStructureYaml = yamlStream.str() + groupStructureYaml;
}

//--------------------------------------------------------------------------------
void readerDeserializeGroupStructure(ioda::Group & memGroup,
                                     const std::string & groupStructureYaml) {
    // Deserialize the yaml string into an eckit YAML configuration object. Then
    // walk through that structure building the structure as you go.
    const eckit::YAMLConfiguration config(groupStructureYaml);

    // create the top level group attributes from the "attributes" section
    std::vector<eckit::LocalConfiguration> attrConfigs;
    config.get("attributes", attrConfigs);
    AttrUtils::createAttributesFromConfig(memGroup.atts, attrConfigs);

    // create the sub groups from the "groups" section
    std::vector<eckit::LocalConfiguration> groupConfigs;
    config.get("groups", groupConfigs);
    for (size_t i = 0; i < groupConfigs.size(); ++i) {
        const std::string groupName = groupConfigs[i].getString("group.name");
        Group subGroup = memGroup.create(groupName);
        attrConfigs.clear();
        groupConfigs[i].get("group.attributes", attrConfigs);
        AttrUtils::createAttributesFromConfig(subGroup.atts, attrConfigs);
    }

    // create dimensions from the "dimensions" section
    std::vector<eckit::LocalConfiguration> dimConfigs;
    config.get("dimensions", dimConfigs);
    VarUtils::createDimensionsFromConfig(memGroup.vars, dimConfigs);

    // create variables from the "variables" section
    std::vector<eckit::LocalConfiguration> varConfigs;
    config.get("variables", varConfigs);
    VarUtils::createVariablesFromConfig(memGroup.vars, varConfigs);
}

//--------------------------------------------------------------------------------
void readerCopyGroupStructure(const ReaderPoolBase & ioPool,
                              const ioda::Group & fileGroup, const bool emptyFile,
                              ioda::Group & memGroup, std::string & groupStructureYaml) {
    // Serialize into a string containing YAML the structure of the fileGroup, and
    // use MPI send/receive to transfer the YAML string to all the assigned tasks.
    readerSerializeGroupStructure(ioPool, fileGroup, emptyFile, groupStructureYaml);

    // Each task has its own number of locations which is set by the initialize step
    // prior to this call.
    readerDefineYamlAnchors(ioPool, groupStructureYaml);

    // Deserialize the YAML string into a constructed group structure in the memGroup
    readerDeserializeGroupStructure(memGroup, groupStructureYaml);
}

//------------------------------------------------------------------------------------
void recordDimSizes(const eckit::YAMLConfiguration & config,
                    std::map<std::string, ioda::Dimensions_t> & dimSizes) {
    // Create a map with the dimension name as the key and dimension size as the value
    std::vector<eckit::LocalConfiguration> dimConfigs;
    config.get("dimensions", dimConfigs);
    for (std::size_t i = 0; i < dimConfigs.size(); ++i) {
        const std::string dimName = dimConfigs[i].getString("dimension.name");
        const ioda::Dimensions_t dimSize = dimConfigs[i].getLong("dimension.size");
        dimSizes[dimName] = dimSize;
    }
}

//------------------------------------------------------------------------------------
ioda::Dimensions_t maxDimSize(const std::map<std::string, ioda::Dimensions_t> & dimSizes) {
    ioda::Dimensions_t maxSize = 0;
    for (const auto & dimPair : dimSizes) {
        if (dimPair.second > maxSize) {
            maxSize = dimPair.second;
        }
    }
    return maxSize;
}

//------------------------------------------------------------------------------------
ioda::Dimensions_t calcSourceMaxElements(const ReaderPoolBase & ioPool,
                                         const eckit::YAMLConfiguration & config) {
    // Record the dimension sizes in the config, then look up which dimensions are
    // attached to each variable to get the total number of elements for that variable.
    std::map<std::string, ioda::Dimensions_t> dimSizes;
    recordDimSizes(config, dimSizes);

    // Walk through the variables and get the max number of elements. Use the source nlocs
    // value instead of the size of the Location dimension for calculating sourceMaxElements.
    // Note that the entry in for "Location" in dimSizes will be the local nlocs, which can
    // be smaller than the nlocs from the source (input file).
    //
    // We want sourceMaxElements to be zero when on a non-pool member
    // since the associated srcBuffer will not be used.
    ioda::Dimensions_t sourceMaxElements = 0;
    if (ioPool.commPool() != nullptr) {
        dimSizes["Location"] = ioPool.sourceNlocs();       // override as noted above
        // Set sourceMaxElements after adjusting the Location dimension size
        sourceMaxElements = maxDimSize(dimSizes);
        std::vector<eckit::LocalConfiguration> varConfigs;
        config.get("variables", varConfigs);
        for (std::size_t i = 0; i < varConfigs.size(); ++i) {
            std::vector<std::string> dimNames =
                varConfigs[i].getStringVector("variable.dimensions");
            // Number of elements is the product of all the dimension sizes
            ioda::Dimensions_t numSourceElements = 1;
            for (std::size_t j = 0; j < dimNames.size(); ++j) {
                numSourceElements *= dimSizes.at(dimNames[j]);
            }
            if (numSourceElements > sourceMaxElements) {
                sourceMaxElements = numSourceElements;
            }
        }
    }
    return sourceMaxElements;
}

//------------------------------------------------------------------------------------
ioda::Dimensions_t calcDestMaxElements(const ReaderPoolBase & ioPool,
                                       const eckit::YAMLConfiguration & config) {
    // Record the dimension sizes in the config, then look up which dimensions are
    // attached to each variable to get the total number of elements for that variable.
    std::map<std::string, ioda::Dimensions_t> dimSizes;
    recordDimSizes(config, dimSizes);

    // When on a pool member, you have to have enough space in your destBuffer to be able
    // to send to your own obs space as well as your assigned ranks. In this case set
    // the destMaxElements based on the max of the nlocs for yourself and all of
    // your assigned ranks. Note that the rankAssignment member of ioPool contains the
    // nlocs of your assigned ranks.
    //
    // When on a non-pool member, you only need to have enough space in your destBuffer
    // to be able to receive from your assigned pool member. In this case set the
    // destMaxElements based on your own nlocs (which is the entry in the dimSizes map).
    if (ioPool.commPool() != nullptr) {
        ioda::Dimensions_t maxNlocs = dimSizes.at("Location");
        for (auto & rankAssign : ioPool.rankAssignment()) {
            if (rankAssign.second > maxNlocs) {
                maxNlocs = rankAssign.second;
            }
        }
        dimSizes["Location"] = maxNlocs;
    }

    // Set destMaxElements after adjusting the Location dimension size
    ioda::Dimensions_t destMaxElements = maxDimSize(dimSizes);
    std::vector<eckit::LocalConfiguration> varConfigs;
    config.get("variables", varConfigs);
    for (std::size_t i = 0; i < varConfigs.size(); ++i) {
        const std::vector<std::string> dimNames =
            varConfigs[i].getStringVector("variable.dimensions");
        ioda::Dimensions_t numDestElements = 1;
        // Number of elements is the product of all the dimension sizes
        for (std::size_t j = 0; j < dimNames.size(); ++j) {
            numDestElements *= dimSizes.at(dimNames[j]);
        }
        if (numDestElements > destMaxElements) {
            destMaxElements = numDestElements;
        }
    }
    return destMaxElements;
}

//--------------------------------------------------------------------------------
bool setDoLocSelection(const ioda::Dimensions_t & srcNlocs,
                       const ioda::Dimensions_t & destNlocs,
                       const std::string & varName, const std::string & firstDimName) {
    // Need to do location selection when destination nlocs is less than source nlocs and:
    //    varName is Location or the firstDimName is Location
    bool doLocSelection = false;
    if (destNlocs < srcNlocs) {
        if (varName == "Location") {
            doLocSelection = true;
        } else if (firstDimName == "Location") {
            doLocSelection = true;
        }
    }
    return doLocSelection;
}

//--------------------------------------------------------------------------------
// The purpose of these templated functions is so that a std::string would return
// the size of a char * pointer instead of the size of a std::string object. This
// is the form that is passed to/from the backend for a string or vector of strings.
template <class DataType>
ioda::Dimensions_t getDataTypeSize() {
    return sizeof(DataType);
}

template <>
ioda::Dimensions_t getDataTypeSize<std::string>() {
    return sizeof(char *);
}

//--------------------------------------------------------------------------------
ioda::Dimensions_t getMaxDataTypeSize() {
    // Find the max data type size using a combination of the forEachSupportedVariableType
    // and switchOnSupportedVariableType utilities.
    ioda::Dimensions_t maxDataTypeSize = 0;
    VarUtils::forEachSupportedVariableType(
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
            ioda::Dimensions_t dataTypeSize = getDataTypeSize<T>();
            if (dataTypeSize > maxDataTypeSize) {
                maxDataTypeSize = dataTypeSize;
            }
        });
    return maxDataTypeSize;
}

//------------------------------------------------------------------------------------
ioda::Selection createEntireVarSelection(const ioda::Variable & var) {
    // Want the selection to cover the entire data space of the variable
    const std::vector<Dimensions_t> varCounts = var.getDimensions().dimsCur;
    const std::vector<Dimensions_t> varStarts(varCounts.size(), 0);

    Selection varSelect;
    varSelect.extent(varCounts).select({ SelectionOperator::SET, varStarts, varCounts });
    return varSelect;
}

//--------------------------------------------------------------------------------
template <class VarType>
void replaceFillWithMissingImpl(const VarType & fillValue, const VarType & missingValue,
                                const ioda::Dimensions_t & numElements,
                                std::vector<char> & srcValues) {
    // Use a span<VarType> to interpret data in the character buffer (vector<char>)
    // as the proper data type. This will allow walking through the buffer and replacing
    // the fill values with the missing values. Note the use of reinterpret_cast is
    // highly discouraged since it is typically unsafe. However in this case the character
    // buffer has been carefully managed to have its proper data type identified and its
    // size allocated to hold enough memory.
    gsl::span<VarType> srcValuesSpan{reinterpret_cast<VarType *>(srcValues.data()),
                                     static_cast<gsl::index>(numElements)};
    for (std::size_t i = 0; i < static_cast<size_t>(numElements); ++i) {
        if ((srcValuesSpan[i] == fillValue) || (std::isinf(srcValuesSpan[i])) ||
            (std::isnan(srcValuesSpan[i]))) {
            srcValuesSpan[i] = missingValue;
        }
    }
}

//--------------------------------------------------------------------------------
// specialization for strings
template <>
void replaceFillWithMissingImpl<std::shared_ptr<std::string>>(
                                 const std::shared_ptr<std::string> & fillValue,
                                 const std::shared_ptr<std::string> & missingValue,
                                 const ioda::Dimensions_t & numElements,
                                 std::vector<char> & srcValues) {
    // Use a span<VarType> to interpret data in the character buffer (vector<char>)
    // as the proper data type. This will allow walking through the buffer and replacing
    // the fill values with the missing values. Note the use of reinterpret_cast is
    // highly discouraged since it is typically unsafe. However in this case the character
    // buffer has been carefully managed to have its proper data type identified and its
    // size allocated to hold enough memory.
    //
    // The srcValues vector is composed of char * pointers that point to the C-style
    // strings. The hdf5 API takes ownership (responsibility for allocating and
    // deallocating the memory for the strings) so we need to simply switch the pointer
    // to the JEDI string missing value. This value is a "static const std::string" in
    // oops which will persist through the lifetime of the obs space. Note that because
    // of the above situation, there is no need for this code to do any
    // management (ie, allocation, deallocation) of the string memory.
    gsl::span<char *> srcValuesSpan{reinterpret_cast<char **>(srcValues.data()),
                                    static_cast<gsl::index>(numElements)};
    for (std::size_t i = 0; i < static_cast<size_t>(numElements); ++i) {
        if (strcmp(srcValuesSpan[i], (*fillValue).c_str()) == 0) {
            srcValuesSpan[i] = const_cast<char *>((*missingValue).c_str());
        }
    }
}

//--------------------------------------------------------------------------------
// Replaces fill values from obs source with JEDI missing values. This routine and
// the "Impl" versions above are for processing the primary vector<char> buffer
// that is used by all of the variable transfers except for dateTime, longitude
// and latitude.
template <class VarType>
void replaceFillWithMissing(const ReaderPoolBase & ioPool,
                            const ioda::Variable & srcVar,
                            const ioda::Dimensions_t & numElements,
                            std::vector<char> & srcValues) {
    // If there is no fill value on the source variable then there is no need to do any
    // replacement.
    if (srcVar.hasFillValue()) {
        VarType fillValue;
        const ioda::detail::FillValueData_t srcFvData = srcVar.getFillValue();
        fillValue = ioda::detail::getFillValue<VarType>(srcFvData);
        const VarType missingValue = getMissingValue<VarType>();

        // No need to replace if the fill value and missing value are already equal.
        if (fillValue != missingValue) {
            replaceFillWithMissingImpl<VarType>(fillValue, missingValue,
                                                numElements, srcValues);
        }
    }
}

// Specialization for string
template <>
void replaceFillWithMissing<std::string>(const ReaderPoolBase & ioPool,
                                         const ioda::Variable & srcVar,
                                         const ioda::Dimensions_t & numElements,
                                         std::vector<char> & srcValues) {
    // If there is no fill value on the source variable then there is no need to do any
    // replacement.
    if (srcVar.hasFillValue()) {
        std::shared_ptr<std::string> fillValue;
        const ioda::detail::FillValueData_t srcFvData = srcVar.getFillValue();
        fillValue = std::make_shared<std::string>(
            ioda::detail::getFillValue<std::string>(srcFvData));
        const std::shared_ptr<std::string> missingValue =
            getMissingValue<std::shared_ptr<std::string>>(ioPool);

        // No need to replace if the fill value and missing value are already equal.
        if (fillValue != missingValue) {
            replaceFillWithMissingImpl<std::shared_ptr<std::string>>(
                fillValue, missingValue, numElements, srcValues);
        }
    }
}

//--------------------------------------------------------------------------------
// Replaces fill values from obs source with JEDI missing values. This routine
// is used for the variable transfers for dateTime (int64_t), longitude (float) and
// latitude (float).
template <class VarType>
void replaceFillWithMissingSpecial(const ioda::Variable & srcVar,
                                   const ioda::Dimensions_t & numElements,
                                   std::vector<VarType> & srcValues) {
    // If there is no fill value on the source variable then there is no need to do any
    // replacement.
    if (srcVar.hasFillValue()) {
        VarType fillValue;
        const ioda::detail::FillValueData_t srcFvData = srcVar.getFillValue();
        fillValue = ioda::detail::getFillValue<VarType>(srcFvData);
        const VarType missingValue = getMissingValue<VarType>();

        // No need to replace if the fill value and missing value are already equal.
        if (fillValue != missingValue) {
            for (std::size_t i = 0; i < static_cast<size_t>(numElements); ++i) {
                if (srcValues[i] == fillValue) {
                    srcValues[i] = missingValue;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------
template <class DataType>
void selectVarValues(const std::vector<DataType> & srcBuffer,
                     const std::vector<std::size_t> & locIndices,
                     const ioda::Dimensions_t dataTypeSize,
                     const std::vector<ioda::Dimensions_t> & varShape,
                     std::vector<DataType> & destBuffer) {
    // srcBuffer and destBuffer have been resized to their proper length. locIndices
    // contains the indices referencing srcBuffer to copy into destBuffer. Both buffers
    // are the same type.
    //
    // In the case where a byte-wise buffer is used (type char) for different
    // variable types (float, int, etc) the dataTypeSize argument is used to resolve
    // offsets into the buffers. Always set dataTypeSet to 1 when the buffers are
    // matching the actual data type being used for the buffers. If using byte-wise
    // buffers, then set dataType to the size (in bytes) of the actual variable data type.
    //
    // Examples
    //    Calling this function with float buffers for the MetaData/latitude variable ->
    //          set dataTypeSize to 1
    //    Calling this function with byte-wise buffers for the
    //    ObsValue/brightnessTemperature variable that is a float date type ->
    //          set the dataTypeSize to 4
    //
    // This flexibility is being done to accommodate the special dateTime (int64_t),
    // longitude (float) and latitude (float) buffers being used in other parts of
    // the reader. Note that all other variables should transfer using the byte-wise
    // buffer which is optimal for copying data between two groups.
    //
    // Note that the number of elements per Location is equal to the product of the
    // variable dimension sizes (varShape) with the first dimension (Location) size
    // set to 1.
    const ioda::Dimensions_t count = dataTypeSize * std::accumulate(
        varShape.begin()+1, varShape.end(), 1, std::multiplies<ioda::Dimensions_t>());
    for (std::size_t i = 0; i < locIndices.size(); ++i) {
        const ioda::Dimensions_t srcStart = locIndices[i] * count;
        const ioda::Dimensions_t destStart = i * count;
        for (std::size_t j = 0; j < static_cast<size_t>(count); ++j) {
            destBuffer[destStart + j] = srcBuffer[srcStart + j];
        }
    }
}

//------------------------------------------------------------------------------------
ioda::Dimensions_t getVarDataTypeSize(const ioda::Variable & var,
                                      const std::string & varName) {
    ioda::Dimensions_t varDataTypeSize;
    VarUtils::forAnySupportedVariableType(
        var,
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
                varDataTypeSize = getDataTypeSize<T>();
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
    return varDataTypeSize;
}

//------------------------------------------------------------------------------------
void readerLoadSourceVarReplaceFill(const ReaderPoolBase & ioPool,
                                    const Variable & srcVar,
                                    const std::string & srcVarName,
                                    std::vector<char> & srcBuffer) {
    // Read variable from the source (input file) and replace any fill values with
    // the corresponding JEDI missing value.
    const ioda::Selection srcSelect = createEntireVarSelection(srcVar);
    VarUtils::forAnySupportedVariableType(
        srcVar,
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
            const Type srcType =
                Types::GetType_Wrapper<T>::GetType(srcVar.getTypeProvider());
            srcVar.read(srcBuffer, srcType, srcSelect, srcSelect);
            const ioda::Dimensions_t numElements = srcVar.getDimensions().numElements;
            replaceFillWithMissing<T>(ioPool, srcVar, numElements, srcBuffer);
        },
        VarUtils::ThrowIfVariableIsOfUnsupportedType(srcVarName));
}

//------------------------------------------------------------------------------------
ioda::Dimensions_t calcAdjustedNumElements(const std::vector<ioda::Dimensions_t> & varShape,
                                           const ioda::Dimensions_t & newNlocs,
                                           const bool doLocSelection) {
    // When doing location selection, we want the number of elements to be adjusted
    // the the substituting the newNlocs value for the fisrt dimension (Location)
    // in varShape. Otherwise, the number of elements is simply the product of
    // all of the dimension sizes in varShape.
    ioda::Dimensions_t numElements;
    if (doLocSelection) {
        // Product of newNlocs with the second through last entry in varShape.
        // Note when varShape has a size of 1, std::accumulate will return 1 which is what
        // we need.
        numElements = newNlocs * std::accumulate(varShape.begin()+1, varShape.end(), 1,
                                                 std::multiplies<ioda::Dimensions_t>());
    } else {
        numElements = std::accumulate(varShape.begin(), varShape.end(), 1,
                                      std::multiplies<ioda::Dimensions_t>());
    }
    return numElements;
}

//------------------------------------------------------------------------------------
void readerTransferBuffers(const std::vector<char> & srcBuffer,
                           const std::vector<std::size_t> & index,
                           const ioda::Dimensions_t varDataTypeSize,
                           const std::vector<ioda::Dimensions_t> & varShape,
                           const ioda::Dimensions_t numBytes,
                           const bool doLocSelection,
                           std::vector<char> & destBuffer) {
    if (doLocSelection) {
        // Copy with location selection
        selectVarValues<char>(srcBuffer, index, varDataTypeSize, varShape, destBuffer);
    } else {
        // Copy without location selection.
        memcpy(destBuffer.data(), srcBuffer.data(), numBytes);
    }
}

//------------------------------------------------------------------------------------
void readerSaveDestVar(const std::string & varName, const std::vector<char> & destBuffer,
                       ioda::Variable & destVar) {
    oops::Log::trace() << "readerSaveDestVar: writing: " << varName << std::endl;
    // write data into destination variable
    const ioda::Selection destSelect = createEntireVarSelection(destVar);
    VarUtils::forAnySupportedVariableType(
        destVar,
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
            const Type destType =
                    Types::GetType_Wrapper<T>::GetType(destVar.getTypeProvider());
            destVar.write(destBuffer, destType, destSelect, destSelect);
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
}

//------------------------------------------------------------------------------------
void readerSaveDestVarLocal(const std::string & varName,
                            const std::vector<char> & srcBuffer,
                            const std::vector<std::size_t> & index,
                            const ioda::Dimensions_t destNlocs,
                            const bool doLocSelection,
                            std::vector<char> & destBuffer, ioda::Variable & destVar) {
    // Note caller allocates destBuffer so that you don't keep allocating and deallocating
    // which can lead to fragmentation on the heap.

    // Calculate the number of bytes for tranferring the buffers, then transfer from the
    // source buffer to the destination buffer.
    //
    // Note that for variables not dimensioned by Location, varShape will be
    // the same for source (file) and destination (obs space) groups. For the
    // variables dimensioned by Location, varShape will serve as a template
    // shape of which the code will need to replace the Location (first) dimension
    // with the proper size before using.
    const ioda::Dimensions_t varDataTypeSize = getVarDataTypeSize(destVar, varName);
    const std::vector<ioda::Dimensions_t> varShape = destVar.getDimensions().dimsCur;
    const ioda::Dimensions_t numBytes = varDataTypeSize *
                                  calcAdjustedNumElements(varShape, destNlocs, doLocSelection);
    if (numBytes > 0) {
        readerTransferBuffers(srcBuffer, index, varDataTypeSize, varShape, numBytes,
                              doLocSelection, destBuffer);

        // Write the destBuffer data into the destVar
        readerSaveDestVar(varName, destBuffer, destVar);
    }
}

//------------------------------------------------------------------------------------
int findMaxStringLength(std::vector<char> & destBuffer, const int numStrings) {
    // destBuff holds a series of char * pointers upon entry. Find the maximum string length
    // using a string span placed over the destBuff. Then use that maximum string length
    // to allocate a char array large enough to hold all of the strings.
    const gsl::span<char *> stringSpan(
        reinterpret_cast<char **>(destBuffer.data()), numStrings);
    int maxStringLength = 0;
    for (const auto & stringElement : stringSpan) {
        if (strlen(stringElement) > maxStringLength) {
            maxStringLength = strlen(stringElement);
        }
    }
    return maxStringLength;
}

//------------------------------------------------------------------------------------
void packStringsIntoCharArray(std::vector<char> & destBuffer,
                              const std::vector<int> charArrayShape,
                              std::vector<char> & strBuffer) {
    // Create a char * span across destBuffer which can be used to copy strings to
    // the strBuffer. charArrayShape[0] is the number of strings, charArrayShape[1]
    // is the fixed string length (which allows for a trailing null byte).
    const gsl::span<char *> stringSpan(
        reinterpret_cast<char **>(destBuffer.data()), charArrayShape[0]);
    for (std::size_t i = 0; i < stringSpan.size(); ++i) {
        const int strLen = strlen(stringSpan[i]);
        for (std::size_t j = 0; j < strLen; ++j) {
            strBuffer[(i * charArrayShape[1]) + j] = stringSpan[i][j];
        }
        strBuffer[strLen] = '\0';
    }
}

//------------------------------------------------------------------------------------
void allocateStringMemForDestBuffer(std::vector<std::string> & strValues,
                                    const std::vector<int> & charArrayShape,
                                    std::vector<char> & destBuffer) {
    // Allocate memory to hold the string values and point the destBuffer
    // contents to that memory. Use a string vector (from caller) to get the
    // nice memory management (ie, to prevent leaks). Use the charArrayShape
    // values to allocate enough memory so the unpackStringsFromCharArray
    // can simply do strcpy's to transfer the data.
    const std::string fillString(charArrayShape[1], '\0');
    strValues.assign(charArrayShape[0], fillString);

    // Use a char * span to facilitate the assignment of the string vector's allocated
    // memory to the char * pointers in destBuffer.
    const gsl::span<char *> stringSpan(
        reinterpret_cast<char **>(destBuffer.data()), charArrayShape[0]);
    for (std::size_t i = 0; i < charArrayShape[0]; ++i) {
        stringSpan[i] = strValues[i].data();
    }
}

//------------------------------------------------------------------------------------
void unpackStringsFromCharArray(const std::vector<char> & strBuffer,
                                const std::vector<int> & charArrayShape,
                                std::vector<char> & destBuffer) {
    // destBuffer is assumed to be setup with char * pointers that point to memory
    // allocated with enough size to hold the values in strBuffer. That leaves it up
    // to this function to just do strcpy's to transfer the data. Use a char * span
    // to facilitate the transfer of the string values.
    const gsl::span<char *> stringSpan(
        reinterpret_cast<char **>(destBuffer.data()), charArrayShape[0]);
    for (size_t i = 0; i < charArrayShape[0]; ++i) {
        const int offset = i * charArrayShape[1];
        const auto strEnd = std::find(strBuffer.begin() + offset, strBuffer.end(), '\0');
        if (strEnd == strBuffer.end()) {
            throw Exception("End of string not found during MPI transfer", ioda_Here());
        }
        strncpy(stringSpan[i], strBuffer.data() + offset, charArrayShape[1]);
    }
}

//------------------------------------------------------------------------------------
void readerSaveDestVarGlobal(const ReaderPoolBase & ioPool,
                             const std::string & varName,
                             const std::vector<char> & srcBuffer,
                             const ioda::Dimensions_t & destNlocs,
                             const bool doLocSelection,
                             const int & varNumber,
                             std::vector<char> & destBuffer,
                             ioda::Variable & destVar) {
    // Note that for variables not dimensioned by Location, varShape will be
    // the same for source (file) and destination (obs space) groups. For the
    // variables dimensioned by Location, varShape will serve as a template
    // shape of which the code will need to replace the Location (first) dimension
    // with the proper size before using.
    const ioda::Dimensions_t varDataTypeSize = getVarDataTypeSize(destVar, varName);
    const std::vector<ioda::Dimensions_t> varShape = destVar.getDimensions().dimsCur;
    const bool varIsStringVector = destVar.isA<std::string>();

    // Avoid unnecessary work when the destination variable in the non-pool task is empty.
    // In this case, the creation of the variable has already put the variable in the desired
    // state (ie, zero size). Note that doing this also supports the zero obs case.
    //
    // When the variable is a string vector, the vector entries are char * pointers to
    // allocated memory holding the string values. Need to convert the string vector to
    // a char array, transfer the char array, then convert back to a string vector.
    if (ioPool.commPool() != nullptr) {
        // Transfer the variable data to the assigned ranks' obs spaces
        for (const auto & distMap : ioPool.distributionMap()) {
            ioda::Dimensions_t destVarSize;
            ioPool.commAll().receive(&destVarSize, 1, distMap.first, msgIsVariableSize);
            if (destVarSize > 0) {
                // select variable values into the destBuffer according to the distMap.
                const int numBytes = varDataTypeSize *
                    calcAdjustedNumElements(varShape, distMap.second.size(), doLocSelection);
                readerTransferBuffers(srcBuffer, distMap.second, varDataTypeSize,
                                      varShape, numBytes, doLocSelection, destBuffer);

                // Send the data to the destination rank
                if (varIsStringVector) {
                    // charArrayShape is a two element vector, first element is the number
                    // of strings, the second is the maximum string length plus one to hold
                    // a terminating null byte (which makes it easier to unpack).
                    std::vector<int> charArrayShape(2, 0);
                    charArrayShape[0] = numBytes / sizeof(char *);
                    charArrayShape[1] =
                        findMaxStringLength(destBuffer, charArrayShape[0]) + 1;
                    ioPool.commAll().send(charArrayShape.data(), 2,
                                          distMap.first, msgIsVariableSize);

                    // Allocate a string buffer (vector of char) that has enough space
                    // to hold the strings pointed to by the char * pointers in
                    // destBuffer. Then copy the strings into the string buffer.
                    std::vector<char> strBuffer(charArrayShape[0] * charArrayShape[1], '\0');
                    packStringsIntoCharArray(destBuffer, charArrayShape, strBuffer);
                    ioPool.commAll().send(strBuffer.data(), strBuffer.size(),
                                          distMap.first, varNumber);
                } else {
                    ioPool.commAll().send(destBuffer.data(), numBytes,
                                          distMap.first, varNumber);
                }
            }
        }
    } else {
        ioda::Dimensions_t destVarSize = destVar.getDimensions().numElements;
        ioPool.commAll().send(&destVarSize, 1, ioPool.rankAssignment()[0].first,
                              msgIsVariableSize);
        if (destVarSize > 0) {
            // Receive the data from the pool member rank
            const int numBytes = varDataTypeSize *
                calcAdjustedNumElements(varShape, destNlocs, doLocSelection);
            // Used for transferring string variable values.
            std::vector<std::string> strValues;
            if (varIsStringVector) {
                // Get the character array shape from the sender
                std::vector<int> charArrayShape(2, 0);
                ioPool.commAll().receive(charArrayShape.data(), 2,
                                         ioPool.rankAssignment()[0].first, msgIsVariableSize);

                // Allocate a string buffer (vector of char) that has enough space
                // to hold the strings pointed to by the char * pointers in
                // destBuffer. Then copy the strings into the string buffer.
                std::vector<char> strBuffer(charArrayShape[0] * charArrayShape[1], '\0');
                ioPool.commAll().receive(strBuffer.data(), strBuffer.size(),
                                         ioPool.rankAssignment()[0].first, varNumber);

                // Allocate memory to hold the string values and point the destBuffer
                // contents to that memory. Use a string vector to get the
                // nice memory management (ie, to prevent leaks).
                allocateStringMemForDestBuffer(strValues, charArrayShape, destBuffer);
                unpackStringsFromCharArray(strBuffer, charArrayShape, destBuffer);
            } else {
                ioPool.commAll().receive(destBuffer.data(), numBytes,
                                         ioPool.rankAssignment()[0].first, varNumber);
            }

            // Write values into memGroup (obs space)
            readerSaveDestVar(varName, destBuffer, destVar);
        }
    }
}

//------------------------------------------------------------------------------------
void readerTransferVarData(const ReaderPoolBase & ioPool,
                           const ioda::Group & fileGroup, ioda::Group & memGroup,
                           std::string & groupStructureYaml,
                           std::vector<int64_t> & dtimeValues) {
    // Deserialize the yaml string into an eckit YAML configuration object which
    // can be used to figure out whether each variable is dimensioned by Location.
    const eckit::YAMLConfiguration config(groupStructureYaml);

    // Allocate a buffer (vector of char) for reading data from the file (srcBuffer).
    // This buffer needs to be large enough to hold any of the variables in the file
    // so calculate the maximum number of elements in any variable and multiply that
    // by the maximum data type size to get the number of bytes large enough to hold any
    // of the file's variables. Note that only tasks in the io pool need a srcBuffer,
    // so set the size of srcBuffer to zero on the non-pool members.
    //
    // Do the same for a destination buffer, destBuffer.
    //
    // Note that sourceMaxElements is based on the number of Locations in the file,
    // whereas destMaxElements is based on the number of Locations for the obs space
    // on each task.
    const ioda::Dimensions_t sourceMaxElements = calcSourceMaxElements(ioPool, config);
    const ioda::Dimensions_t destMaxElements = calcDestMaxElements(ioPool, config);
    const ioda::Dimensions_t maxDataTypeSize = getMaxDataTypeSize();
    std::vector<char>srcBuffer(sourceMaxElements * maxDataTypeSize);
    std::vector<char>destBuffer(destMaxElements * maxDataTypeSize);

    // Record the number of locations from the source (input file) and the number
    // of locations for this rank.
    const std::size_t srcNlocs = ioPool.sourceNlocs();
    const std::size_t destNlocs = ioPool.nlocs();

    // Set up a variable number that will be used for the tag value for the
    // MPI send/recv calls. Need to start numbering at a specified value.
    // See comments above where mpiVariableNumberStart is set.
    int varNumber = mpiVariableNumberStart;

    // Walk through the dimensions section of the configuration and transfer the
    // data from pool member to itself and its assigned ranks.
    std::vector<eckit::LocalConfiguration> dimConfigs;
    config.get("dimensions", dimConfigs);
    for (std::size_t i = 0; i < dimConfigs.size(); ++i) {
        const std::string dimName = dimConfigs[i].getString("dimension.name");
        ioda::Variable destVar = memGroup.vars.open(dimName);

        // Determine if we need to do location selection on this variable.
        const bool doLocSelection = setDoLocSelection(srcNlocs, destNlocs, dimName, dimName);

        // On pool members only, read in from the source into the source buffer
        // and save in the local obs space.
        if (ioPool.commPool() != nullptr) {
            // Avoid unnecessary work when the destination variable is empty. In this case,
            // the creation of the variable has already put the variable in the desired
            // state (ie, zero size). Note that doing this also supports the zero obs case.
            if (destVar.getDimensions().numElements > 0) {
                // Read variable from the source (input file), and replace fill with
                // the corresponding JEDI missing value.
                Variable srcVar = fileGroup.vars.open(dimName);
                readerLoadSourceVarReplaceFill(ioPool, srcVar, dimName, srcBuffer);

                // Transfer the variable data to this rank's obs space
                readerSaveDestVarLocal(dimName, srcBuffer, ioPool.index(), destNlocs,
                                       doLocSelection, destBuffer, destVar);
            }
        }

        // Transfer data from the pool members to their assigned non pool member ranks.
        readerSaveDestVarGlobal(ioPool, dimName, srcBuffer, destNlocs, doLocSelection,
                                varNumber, destBuffer, destVar);
        varNumber += 1;
    }

    // Walk through the variables section of the configuration and transfer the data
    // from pool member to itself and its assigned ranks. Note that date time values
    // can be one of three formats in the input file (offset, string or epoch), and
    // for now we are using the dtimeValues parameter (which have already been converted
    // to the epoch format) instead of reading these from the file.
    std::vector<eckit::LocalConfiguration> varConfigs;
    config.get("variables", varConfigs);
    for (std::size_t i = 0; i < varConfigs.size(); ++i) {
        const std::string varName = varConfigs[i].getString("variable.name");
        const std::vector<std::string> varDimNames =
            varConfigs[i].getStringVector("variable.dimensions");

        // All ranks will need to open the destination (memGroup) variable. We can
        // get useful info from this variable.
        //
        // Note that for variables not dimensioned by Location, varShape will be
        // the same for source (file) and destination (obs space) groups. For the
        // variables dimensioned by Location, varShape will serve as a template
        // shape of which the code will need to replace the Location (first) dimension
        // with the proper size before using.
        ioda::Variable destVar = memGroup.vars.open(varName);

        // Determine if we need to do location selection on this variable.
        const bool doLocSelection =
            setDoLocSelection(srcNlocs, destNlocs, varName, varDimNames[0]);

        // On pool members only, read in from the source into the source buffer
        // and save in the local obs space.
        if (ioPool.commPool() != nullptr) {
            // Read variable from the source (input file), and replace fill with
            // the corresponding JEDI missing value.
            if (varName == "MetaData/dateTime") {
                const ioda::Dimensions_t varDataTypeSize = getVarDataTypeSize(destVar, varName);
                memcpy(srcBuffer.data(), dtimeValues.data(), (srcNlocs * varDataTypeSize));
            } else {
                // Read variable from the source (input file), and replace fill with
                // the corresponding JEDI missing value.
                Variable srcVar = fileGroup.vars.open(varName);
                readerLoadSourceVarReplaceFill(ioPool, srcVar, varName, srcBuffer);
            }

            // Transfer the variable data to this rank's obs space
            readerSaveDestVarLocal(varName, srcBuffer, ioPool.index(), destNlocs,
                                   doLocSelection, destBuffer, destVar);
        }

        // Transfer data from the pool members to their assigned non pool member ranks.
        readerSaveDestVarGlobal(ioPool, varName, srcBuffer, destNlocs, doLocSelection,
                                varNumber, destBuffer, destVar);
        varNumber += 1;
    }
}

//------------------------------------------------------------------------------------
// Old reader functions
//------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
template <typename VarType>
void readerCreateVariable(const std::string & varName, const Variable & srcVar,
                          const ioda::Dimensions_t adjustNlocs, Has_Variables & destVars,
                          ioda::Dimensions_t & globalMaxElements,
                          ioda::Dimensions_t & maxElements) {
    oops::Log::trace() << "readerCreateVariable: creating: " << varName << std::endl;
    // Record the max number of elements on the source side and on the destination side.
    // These values will be used by the readerCopyVarData function.
    Dimensions varDims = srcVar.getDimensions();
    if (varDims.numElements > globalMaxElements) {
        globalMaxElements = varDims.numElements;
    }
    // If adjust Nlocs is >= 0, this means that this is a variable that needs
    // to be created with the total number of locations from the MPI tasks in the pool.
    // In other words, the first dimension of this variable is "Location", whose size
    // could have been reduced from the reader preprocessing (ie, time window filtering
    // and MPI distribution), and we need to adjust accordingly.
    //
    // We want to be able to resize along the Locations dimension so we want the variables
    // using Locations as their first dimension to have unlimited max size.
    if (adjustNlocs >= 0) {
        varDims.dimsCur[0] = adjustNlocs;
        varDims.dimsMax[0] = ioda::Unlimited;
        varDims.numElements = std::accumulate(varDims.dimsCur.begin(), varDims.dimsCur.end(),
                                              1, std::multiplies<ioda::Dimensions_t>());
    }

    if (varDims.numElements > maxElements) {
        maxElements = varDims.numElements;
    }

    VariableCreationParameters params = VariableCreationParameters::defaults<VarType>();
    params.setFillValue<VarType>(getMissingValue<VarType>());
    // Don't want compression in the memory image.
    params.noCompress();
    // TODO(srh) For now use a default chunk size (10000) for the chunk size in the creation
    // parameters when the first dimension is Location. This is being done since the size
    // of location can vary across MPI tasks, and we need it to be constant for the parallel
    // io to work properly. The assigned chunk size may need to be optimized further than
    // using a rough guess of 10000.
    std::vector<ioda::Dimensions_t> chunkDims = varDims.dimsCur;
    if (adjustNlocs >= 0) {
        chunkDims[0] = VarUtils::DefaultChunkSize;
    }
    params.setChunks(chunkDims);

    Variable destVar = destVars.create<VarType>(varName, varDims, params);
    copyAttributes(srcVar.atts, destVar.atts);
}

//--------------------------------------------------------------------------------
void readerCopyVarData(const ReaderPoolBase & ioPool,
                       const ioda::Group & src, ioda::Group & dest,
                       const VarUtils::Vec_Named_Variable & srcVarsList,
                       const VarUtils::VarDimMap & dimsAttachedToVars,
                       std::vector<int64_t> & dtimeVals,
                       std::vector<float> & lonVals,
                       std::vector<float> & latVals,
                       const ioda::Dimensions_t globalMaxElements,
                       const ioda::Dimensions_t maxElements,
                       const bool isParallelIo) {
    // At the level of this data transfer, all data types are stored directly in contiguous
    // memory except for variable length strings. In the variable length string case,
    // char * pointers are stored in contiguous memory that point to allocated memory
    // containing the actual strings. For the variable length string, it works
    // to pass the contiguous memory filled with char * pointers to the underlying variable
    // read and write routines to do the transfer. Therefore transfers for all supported
    // data types can be done using a reusable byte-wise memory buffer such as a
    // vector<char> structure.
    //
    // Two buffers can be allocated here and reused for all variables. This will help
    // guard against memory fragmentation issues. The first buffer is based on the source
    // number of locations and the second is based on the destination number of locations.
    //
    // In situations where there are more than one MPI task the second buffer will be
    // smaller than the first and the selection of the locations for each MPI task will
    // be done between them. This approach will help guard against the inefficienies with
    // using the hdf5 selection mechanism for selecting arbitrarily spaced locations. (The
    // hdf5 selection mechanism is geared more toward selecting regularly spaced patterns.)

    // Need the globalMaxElements, maxElements and the max data type size to know how
    // much memory to allocate for the data buffers.
    ioda::Dimensions_t maxDataTypeSize = getMaxDataTypeSize();

    // If we are not doing MPI distribution (one MPI task) then we can get by using the
    // source buffer for transferring the data.
    //
    // If we have more than one MPI task, then we need to allocate both the source and
    // destination buffers. The selection of the locations for this MPI task can then
    // be done between the source and destination buffers.
    //
    // It is important to use the location indices vectors to test whether the destination
    // buffer is needed. This will make sure the proper action is taken according to
    // the MPI distribution. For example if the globalMaxElements and localMaxElements
    // were to be used instead, that could fail in the case where there are a small number
    // of locations along with a meta data variable that (for some reason) is larger
    // than the number of locations.
    std::vector<char> srcBuffer(globalMaxElements * maxDataTypeSize);
    std::vector<char> destBuffer(maxElements * maxDataTypeSize);
    std::size_t srcNlocs = ioPool.sourceNlocs();
    std::size_t destNlocs = ioPool.nlocs();

    // Do the data transfers. If the variable is dimensions by locations, and we
    // need to do a selection on the locations, then read the source into srcBuffer
    // select according to locIndices into the destBuffer and write destBuffer.
    // Otherwise, read the data into srcBuffer and immediatly write from srcBuffer.
    const std::vector<std::size_t> & locIndices = ioPool.index();
    for (auto & srcNamedVar : srcVarsList) {
        std::string varName = srcNamedVar.name;
        // Skip the following because we either are skipping over obsolete date time
        // formats, or we already have the data in a buffer.
        if ((varName == "MetaData/datetime") || (varName == "MetaData/time") ||
            (varName == "MetaData/longitude") || (varName == "MetaData/latitude")) {
            continue;
        }

        // Avoid unnecessary work when the destination variable is empty. In this case,
        // the creation of the variable has already put the variable in the desired
        // state (ie, zero size). Note that doing this also supports the zero obs case.
        ioda::Variable destVar = dest.vars.open(varName);
        if (destVar.getDimensions().numElements > 0) {
            // Read variable from the source (input file), and replace fill with
            // the corresponding JEDI missing value.
            Variable srcVar = src.vars.open(varName);
            readerLoadSourceVarReplaceFill(ioPool, srcVar, varName, srcBuffer);

            // Determine if we need to do location selection
            std::string firstDimName = "";
            if (dimsAttachedToVars.find(srcNamedVar) != dimsAttachedToVars.end()) {
                firstDimName = dimsAttachedToVars.at(srcNamedVar)[0].name;
            }
            bool doLocSelection =
                setDoLocSelection(srcNlocs, destNlocs, varName, firstDimName);

            // Transfer the variable data to this rank's obs space
            readerSaveDestVarLocal(varName, srcBuffer, ioPool.index(), destNlocs,
                                   doLocSelection, destBuffer, destVar);
        }
    }

    // Write out the dateTime, longitude and latitude values. Note that all three of these
    // variables are dimensioned by Location. First off, replace fill values in longitude
    // and latitude with JEDI missing values.
    replaceFillWithMissingSpecial<float>(
        src.vars.open("MetaData/longitude"), lonVals.size(), lonVals);
    replaceFillWithMissingSpecial<float>(
        src.vars.open("MetaData/latitude"), latVals.size(), latVals);

    ioda::Variable dtimeVar = dest.vars.open("MetaData/dateTime");
    ioda::Variable lonVar = dest.vars.open("MetaData/longitude");
    ioda::Variable latVar = dest.vars.open("MetaData/latitude");
    if (destNlocs < srcNlocs) {
        if (destNlocs > 0) {
            // Apply location indices selection
            std::vector<int64_t> destInt64Vals(destNlocs);
            std::vector<float> destFloatVals(destNlocs);
            std::vector<ioda::Dimensions_t> varShape(1, destNlocs);

            selectVarValues<int64_t>(dtimeVals, locIndices, 1, varShape, destInt64Vals);
            dtimeVar.write(destInt64Vals);

            selectVarValues<float>(lonVals, locIndices, 1, varShape, destFloatVals);
            lonVar.write(destFloatVals);

            selectVarValues<float>(latVals, locIndices, 1, varShape, destFloatVals);
            latVar.write(destFloatVals);
        }
    } else {
        // Can write entire buffer into the destination variables
        dtimeVar.write(dtimeVals);
        lonVar.write(lonVals);
        latVar.write(latVals);
    }
}

//--------------------------------------------------------------------------------
// Definitions of public functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void ioReadGroup(const ReaderPoolBase & ioPool, const ioda::Group& fileGroup,
                 ioda::Group& memGroup, const DateTimeFormat dtimeFormat,
                 std::vector<int64_t> & dtimeVals, const std::string & dtimeEpoch,
                 std::vector<float> & lonVals, std::vector<float> & latVals,
                 const bool isParallelIo, const bool emptyFile) {
    // Query old data for variable lists and dimension mappings
    VarUtils::Vec_Named_Variable allVarsList;
    VarUtils::Vec_Named_Variable regularVarList;
    VarUtils::Vec_Named_Variable dimVarList;
    VarUtils::VarDimMap dimsAttachedToVars;
    Dimensions_t maxVarSize0;  // unused in this function
    VarUtils::collectVarDimInfo(fileGroup, regularVarList, dimVarList,
                                dimsAttachedToVars, maxVarSize0);

    allVarsList = regularVarList;
    allVarsList.insert(allVarsList.end(), dimVarList.begin(), dimVarList.end());

    // For all ranks, create the group, variable structure. Once this
    // structure is in place, then transfer the variable data.
    //
    // Copy hierarchical group structure from memGroup to fileGroup
    copyGroupStructure(fileGroup, memGroup);

    // Make all variables and copy data and most attributes.
    // Dimension mappings & scales are handled later.
    ioda::Dimensions_t globalMaxElements = 0;
    ioda::Dimensions_t maxElements = 0;
    std::size_t numLocs = ioPool.nlocs();
    for (const auto& namedVar : allVarsList) {
       const std::string srcVarName = namedVar.name;
       if ((srcVarName == "MetaData/datetime") || (srcVarName == "MetaData/time")) {
           // Skip the old date time formats from the obs source
           continue;
       }
       const ioda::Variable fileVar = namedVar.var;

       // adjustNlocs is set to numLocs when the variable's first dimension is "Location",
       // otherwise it is set to -1. This tells readerCreateVariable whether to adjust the
       // incoming Location dimension size (according to the MPI distribution results).
       ioda::Dimensions_t adjustNlocs = -1;
       if (srcVarName == "Location") {
           adjustNlocs = numLocs;
       } else if (dimsAttachedToVars.find(namedVar) != dimsAttachedToVars.end()) {
           if (dimsAttachedToVars.at(namedVar)[0].name == "Location") {
               adjustNlocs = numLocs;
           }
       }
       VarUtils::forAnySupportedVariableType(
           fileVar,
           [&](auto typeDiscriminator) {
               typedef decltype(typeDiscriminator) T;
               readerCreateVariable<T>(srcVarName, fileVar, adjustNlocs, memGroup.vars,
                                                   globalMaxElements, maxElements);
           },
           VarUtils::ThrowIfVariableIsOfUnsupportedType(srcVarName));
    }
    // If the obs source did not contain the epoch style date time format, then we
    // need to create that variable here. Note that MetaData/dateTime has Location
    // as the first (and only) dimension so it needs to be set for unlimited max
    // size.
    if ((dtimeFormat != DateTimeFormat::Epoch) && (dtimeFormat != DateTimeFormat::None)) {
        std::vector<Dimensions_t> nlocsVec(1, numLocs);
        std::vector<Dimensions_t> unlimVec(1, ioda::Unlimited);
        ioda::Dimensions varDims(nlocsVec, unlimVec, 1, numLocs);
        ioda::VariableCreationParameters params =
            VariableCreationParameters::defaults<int64_t>();
        params.setFillValue<int64_t>(getMissingValue<int64_t>());
        params.noCompress();
        // TODO(srh) For now use a default chunk size (10000) for the chunk size in the creation
        // parameters when the first dimension is Location. This is being done since the size
        // of location can vary across MPI tasks, and we need it to be constant for the parallel
        // io to work properly. The assigned chunk size may need to be optimized further than
        // using a rough guess of 10000.
        std::vector<ioda::Dimensions_t> chunkDims(1, VarUtils::DefaultChunkSize);
        params.setChunks(chunkDims);
        ioda::Variable dtimeVar =
            memGroup.vars.create<int64_t>("MetaData/dateTime", varDims, params);
        dtimeVar.atts.add<std::string>("units", dtimeEpoch);
    }

    // Make new dimension scales
    for (auto& dim : dimVarList) {
        memGroup.vars[dim.name].setIsDimensionScale(dim.var.getDimensionScaleName());
    }

    // Attach all dimension scales to all variables.
    // We separate this from the variable creation (above)
    // since we use a collective call for performance.
    std::vector<std::pair<ioda::Variable, std::vector<ioda::Variable>>> dimsAttachedToNewVars;
    for (const auto &old : dimsAttachedToVars) {
      if ((old.first.name == "MetaData/datetime") || (old.first.name == "MetaData/time")) {
          // Skip the old date time formats from the obs source
          continue;
      }
      ioda::Variable new_var = memGroup.vars[old.first.name];
      std::vector<ioda::Variable> new_dims;
      for (const auto &old_dim : old.second) {
          new_dims.push_back(memGroup.vars[old_dim.name]);
      }
      dimsAttachedToNewVars.push_back(make_pair(new_var, std::move(new_dims)));
    }
    // If the obs source did not contain the epoch style date time format, then we
    // need to attach the dimension scales here.
    if ((dtimeFormat != DateTimeFormat::Epoch) && (dtimeFormat != DateTimeFormat::None)) {
      ioda::Variable new_var = memGroup.vars.open("MetaData/dateTime");
      std::vector<ioda::Variable> new_dims(1, memGroup.vars.open("Location"));
      dimsAttachedToNewVars.push_back(make_pair(new_var, std::move(new_dims)));
    }
    memGroup.vars.attachDimensionScales(dimsAttachedToNewVars);

    // Transfer the variable data.
    if (!emptyFile) {
        readerCopyVarData(ioPool, fileGroup, memGroup, allVarsList, dimsAttachedToVars,
                          dtimeVals, lonVals, latVals, globalMaxElements, maxElements,
                          isParallelIo);
    }
}

}  // namespace IoPool
}  // namespace ioda
