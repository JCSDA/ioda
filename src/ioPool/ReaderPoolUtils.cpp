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
#include <cstring>
#include <numeric>

#include "gsl/gsl-lite.hpp"

#include "eckit/geometry/Point2.h"
#include "eckit/mpi/Comm.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/Copying.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/ioPool/ReaderPoolBase.h"
#include "ioda/Variables/Fill.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

#include "oops/util/Logger.h"

namespace ioda {

namespace detail {
  using std::to_string;

  /// An overload of to_string() taking a string and returning the same string.
  std::string to_string(std::string s) {
    return s;
  }
}  // namespace detail

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
DataType getMissingValue(const ioda::ReaderPoolBase & ioPool) {
    DataType missVal = util::missingValue<DataType>();
    return missVal;
}

template<>
std::shared_ptr<std::string> getMissingValue(const ioda::ReaderPoolBase & ioPool) {
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
                          std::string & sourceName, ioda::DateTimeFormat & dtimeFormat,
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
                        const bool emptyFile, const ioda::DateTimeFormat dtimeFormat,
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
                detail::FillValueData_t lonFvData = lonVar.getFillValue();
                float lonFillValue = detail::getFillValue<float>(lonFvData);
                detail::FillValueData_t latFvData = latVar.getFillValue();
                float latFillValue = detail::getFillValue<float>(latFvData);

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
        const bool emptyFile, const std::shared_ptr<Distribution> & distribution,
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
template <typename VarType>
void readerCreateVariable(const std::string & varName, const Variable & srcVar,
                          const ioda::Dimensions_t adjustNlocs, Has_Variables & destVars,
                          ioda::Dimensions_t & globalMaxElements,
                          ioda::Dimensions_t & maxElements) {
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
    params.setFillValue<VarType>(ioda::getMissingValue<VarType>());
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
    std::vector<Dimensions_t> varCounts = var.getDimensions().dimsCur;
    std::vector<Dimensions_t> varStarts(varCounts.size(), 0);

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
void replaceFillWithMissing(const ioda::ReaderPoolBase & ioPool,
                            const ioda::Variable & srcVar,
                            const ioda::Dimensions_t & numElements,
                            std::vector<char> & srcValues) {
    // If there is no fill value on the source variable then there is no need to do any
    // replacement.
    if (srcVar.hasFillValue()) {
        VarType fillValue;
        detail::FillValueData_t srcFvData = srcVar.getFillValue();
        fillValue = detail::getFillValue<VarType>(srcFvData);
        VarType missingValue = getMissingValue<VarType>();

        // No need to replace if the fill value and missing value are already equal.
        if (fillValue != missingValue) {
            replaceFillWithMissingImpl<VarType>(fillValue, missingValue,
                                                numElements, srcValues);
        }
    }
}

// Specialization for string
template <>
void replaceFillWithMissing<std::string>(const ioda::ReaderPoolBase & ioPool,
                                         const ioda::Variable & srcVar,
                                         const ioda::Dimensions_t & numElements,
                                         std::vector<char> & srcValues) {
    // If there is no fill value on the source variable then there is no need to do any
    // replacement.
    if (srcVar.hasFillValue()) {
        std::shared_ptr<std::string> fillValue;
        detail::FillValueData_t srcFvData = srcVar.getFillValue();
        fillValue = std::make_shared<std::string>(
            detail::getFillValue<std::string>(srcFvData));
        std::shared_ptr<std::string> missingValue =
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
        detail::FillValueData_t srcFvData = srcVar.getFillValue();
        fillValue = detail::getFillValue<VarType>(srcFvData);
        VarType missingValue = getMissingValue<VarType>();

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
        ioda::Dimensions_t srcStart = locIndices[i] * count;
        ioda::Dimensions_t destStart = i * count;
        for (std::size_t j = 0; j < static_cast<size_t>(count); ++j) {
            destBuffer[destStart + j] = srcBuffer[srcStart + j];
        }
    }
}

//--------------------------------------------------------------------------------
void readerCopyVarData(const ioda::ReaderPoolBase & ioPool,
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
    std::vector<char> destBuffer(0);
    std::size_t srcNlocs = ioPool.sourceNlocs();
    std::size_t destNlocs = ioPool.nlocs();
    if (destNlocs < srcNlocs) {
        // Have more than one MPI task and are doing selection of locations when
        // transferring variable data.
        destBuffer.resize(maxElements * maxDataTypeSize);
    }

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

        // Determine if we need to do location selection
        bool doLocSelection = false;
        if (destNlocs < srcNlocs) {
            if (varName == "Location") {
                doLocSelection = true;
            } else if (dimsAttachedToVars.find(srcNamedVar) != dimsAttachedToVars.end()) {
                if (dimsAttachedToVars.at(srcNamedVar)[0].name == "Location") {
                    doLocSelection = true;
                }
            }
        }

        // Need to use selection objects that cover the entire variable space. This
        // is because the extent of the srcBuffer can go beyond the extent of the
        // variable (eg, float variable). In this case, we can use the same selection
        // object for both the source and destination variables.
        ioda::Variable srcVar = srcNamedVar.var;
        ioda::Selection srcSelect = createEntireVarSelection(srcVar);
        ioda::Variable destVar = dest.vars.open(varName);
        ioda::Selection destSelect = createEntireVarSelection(destVar);
        VarUtils::forAnySupportedVariableType(
            destVar,
            [&](auto typeDiscriminator) {
                typedef decltype(typeDiscriminator) T;
                Type srcType =
                    Types::GetType_Wrapper<T>::GetType(srcVar.getTypeProvider());
                srcVar.read(srcBuffer, srcType, srcSelect, srcSelect);
                ioda::Dimensions_t numElements = srcVar.getDimensions().numElements;
                replaceFillWithMissing<T>(ioPool, srcVar, numElements, srcBuffer);
                Type destType =
                    Types::GetType_Wrapper<T>::GetType(destVar.getTypeProvider());
                if (doLocSelection) {
                    if (destNlocs > 0) {
                        selectVarValues<char>(srcBuffer, locIndices, getDataTypeSize<T>(),
                                              destVar.getDimensions().dimsCur, destBuffer);
                        destVar.write(destBuffer, destType, destSelect, destSelect);
                    }
                } else {
                    destVar.write(srcBuffer, destType, destSelect, destSelect);
                }
            },
            VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
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
void ioReadGroup(const ioda::ReaderPoolBase & ioPool, const ioda::Group& fileGroup,
                 ioda::Group& memGroup, const ioda::DateTimeFormat dtimeFormat,
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
    // Get all variable and group names
    const auto fileObjects = fileGroup.listObjects(ObjectType::Ignored, true);

    // Make all groups and copy global group attributes.
    copyAttributes(fileGroup.atts, memGroup.atts);
    for (const auto &groupName : fileObjects.at(ObjectType::Group)) {
        ioda::Group srcGroup = fileGroup.open(groupName);
        ioda::Group destGroup = memGroup.create(groupName);
        copyAttributes(srcGroup.atts, destGroup.atts);
    }

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
        params.setFillValue<int64_t>(ioda::getMissingValue<int64_t>());
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

}  // namespace ioda
