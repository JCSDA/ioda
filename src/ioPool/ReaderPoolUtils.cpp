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

/// @brief special MPI broadcast for the DateTimeFormat enum type.
/// @param comm MPI communicator group
/// @param enumVar DateTimeFormat enum variable
/// @param root broadcast root rank
void broadcastDateTimeFormat(const eckit::mpi::Comm & comm, DateTimeFormat & enumVar,
                             const std::size_t root);

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
/// @param varName variable name
/// @param firstDimName name of first dimension of variable
static bool setDoLocSelection(const std::string & varName, const std::string & firstDimName);

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

/// @brief add supplemental attributes that come from the io pool object
/// @param ioPool io pool object holding information to be saved in attributes
/// @param targetCommAllSize targeted size of the commAll communicator group
/// @param targetCommPoolSize targeted size of the commPool communicator group
/// @param destGroup output ioda Group object that will receive the new attributes
void readerAddSupplementalAttributes(const ReaderPoolBase & ioPool,
                                     const int targetCommAllSize,
                                     const int targetCommPoolSize,
                                     ioda::Group & destGroup);

/// @brief remove the special file preparation group from the lists
/// descibing the group structure
/// @param varList is the list of variables (not dimension scales).
/// @param dimVarList is the list of dimension scales.
/// @param dimsAttachedToVars is the mapping of the scales attached to each variable.
void readerRemoveFilePrepGroup(VarUtils::Vec_Named_Variable & varList,
                               VarUtils::Vec_Named_Variable & dimVarList,
                               VarUtils::VarDimMap & dimsAttachedToVars);

/// @brief build file that holds file preparation information
/// @details There needs to be a means for the standalone application to be run on a
/// single process to prepare files for the downstream MPI configuration (which could
/// be many MPI tasks, and multiple tasks be assigned to the io pool). In the standalone
/// application case (ie, external file prep mode) there also exists information about
/// the original source file that will not be visible in the prepared file set, so the
/// prep info file also holds that information. An example is how many locations were
/// in the original source file and of these location how many got filtered out due
/// to the time window filter and QC checks (performed by the standalone app).
/// @param ioPool ReaderPoolBase object
/// @param targetCommAllSize targeted size of the commAll communicator group
/// @param targetCommPoolSize targeted size of the commPool communicator group
/// @param allNlocs number of locations for each rank the commAll communicator group
/// @param ioPoolRanks rank number in the commAll communicator group
/// @details -1 indicates that a rank is not in the io pool
/// @param assocRanks associated rank number
/// @details Each io pool member is associated with itself, and the non io pool members
/// is associated with a pool member rank. This allos the reconstruction of the rank
/// assignments which then describe how the data in the prepared input files are mapped
/// across MPI tasks in the commAll communicator group.
void readerBuildPrepInfoFile(const ReaderPoolBase & ioPool,
                             const int targetCommAllSize,
                             const int targetCommPoolSize,
                             const std::vector<int> & allNlocs,
                             const std::vector<int> & ioPoolRanks,
                             const std::vector<int> & assocRanks);

/// @brief Convert epoch string into a DateTime object.
/// @param epochString epoch specification in ISO 8601 format
/// @param epochDtime DateTime object with equivalent value to the epochString
void convertEpochStringToDtime(const std::string & epochString, util::DateTime & epochDtime);

/// @brief Check obs source for required variables
/// @param srcGroup ioda Group object holding obs source data (file or generator)
/// @param sourceName name of the input obs source
/// @param dtimeFormat format of the datetime variable in the obs source
/// @param emptyFile flag when true have a file with zero obs
void checkForRequiredVars(const ioda::Group & srcGroup, const std::string & sourceName,
                          DateTimeFormat & dtimeFormat, bool & emptyFile);

/// @brief Read date time variable values from obs source
/// @param obsSource ioda Group object holding obs source data (file or generator)
/// @param emptyFile flag when true have a file with zero obs
/// @param dtimeFormat enum value denoting which datetime format exists in the obs source
/// @param dtimeVals vector of int64_t to hold date time values
/// @param dtimeEpoch string value for datetime variable units
void readSourceDtimeVar(const ioda::Group & srcGroup, const bool emptyFile,
                        const DateTimeFormat dtimeFormat, std::vector<int64_t> & dtimeVals,
                        std::string & dtimeEpoch);

/// @brief Initialize the location indices
/// @detail If applyLocCheck is false, then sourceLocIndices is initialize to the entire
/// set of locations in the obs source. Otherwise the timing window filter is applied
/// along with the removal of locations with missing lon and lat values.
/// @param obsSource ioda Group object holding obs source data (file or generator)
/// @param emptyFile flag when true have a file with zero obs
/// @param dtimeValues vector of int64_t to hold date time values
/// @param timeWindow DA time window
/// @param applyLocCheck boolean flag, true indicates that locations need to be checked
/// @param lonValues vector of float to hold longitude values
/// @param latValues vector of float to hold latitude values
/// @param sourceLocIndices location indices that will be kept on this MPI process
/// @param srcNlocs total number of locations from the obs source
/// @param srcNlocsInsideTimeWindow count of locations that are inside the time window
/// @param srcNlocsOutsideTimeWindow count of locations that are outside the time window
/// @param srcNlocsRejectQc count of locations that were rejected by the QC checks
/// @param globalNlocs total number of locations kept (before MPI distribution)
void initSourceIndices(const ioda::Group & srcGroup, const bool emptyFile,
        const std::vector<int64_t> & dtimeValues, const util::TimeWindow & timeWindow,
        const bool applyLocCheck, std::vector<float> & lonValues,
        std::vector<float> & latValues, std::vector<std::size_t> & sourceLocIndices,
        std::size_t & srcNlocs, std::size_t & srcNlocsInsideTimeWindow,
        std::size_t & srcNlocsOutsideTimeWindow, std::size_t & srcNlocsRejectQc,
        std::size_t & globalNlocs);

/// @brief build a list of keys based on the obs grouping variables
/// @param srcGroup ioda Group object holding obs source data (file or generator)
/// @param dtimeValues vector of int64_t to hold date time values
/// @param lonValues vector of float to hold longitude values
/// @param latValues vector of float to hold latitude values
/// @param sourceLocIndices index list generated by initSourceIndices
/// @param obsGroupVarList list of grouping vars from YAML spec
/// @param groupingKeys keys (strings) that hold tuple values based on the obs grouping vars
void buildObsGroupingKeys(const ioda::Group & srcGroup,
                          const std::vector<int64_t> & dtimeValues,
                          const std::vector<float> & lonValues,
                          const std::vector<float> & latValues,
                          const std::vector<std::string> & obsGroupVarList,
                          const std::vector<std::size_t> & sourceLocIndices,
                          std::vector<std::string> & groupingKeys);

/// @brief Assign record numbers based on the obs grouping (if specified)
/// @detail If obs grouping is specified, then form the groups and generate record numbers
/// accordingly. That is, one unique record number per group. If obs grouping is not specified,
/// simply generate sequential record numbers starting with zero (ie, one-to-one mapping
/// with the location indices). Do either of these algorithms with awareness of any
/// locations that got filtered out during the QC checks (eg., outside the timing window).
/// @param srcGroup ioda Group object holding obs source data (file or generator)
/// @param dtimeValues vector of int64_t to hold date time values
/// @param lonValues vector of float to hold longitude values
/// @param latValues vector of float to hold latitude values
/// @param sourceLocIndices index list generated by initSourceIndices
/// @param obsGroupVarList list of groupgin vars from YAML spec
/// "obs space.obsdatain.obsgrouping.group variables"
/// @param sourceRecNums assigned record number for each entry in sourceLocIndices
void assignRecordNumbers(const ioda::Group & srcGroup, const bool emptyFile,
                         const std::vector<int64_t> & dtimeValues,
                         const std::vector<float> & lonValues,
                         const std::vector<float> & latValues,
                         const std::vector<std::size_t> & sourceLocIndices,
                         const std::vector<std::string> & obsGroupVarList,
                         std::vector<std::size_t> & sourceRecNums);

/// @brief emulate the formation of the io pool by the MPI split communicator command
/// @details This function emulates running the MPI split communcator when you have
/// the epected MPI communicator group for splitting. This function is intended to be
/// used by the standalone app that creates the input file set offline.
/// @param targetCommSize targeted number of ranks in "all" communicator group
/// @param rankGrouping map showing how the ranks are grouped together in the target io pool
/// @param assocAllRanks shows association between a given rank and the io pool it belongs to
/// @param ioPoolRanks shows which ranks belong to the io pool, as well as their pool rank
void emulateMpiSplitComm(const int targetCommSize, const IoPoolGroupMap & rankGrouping,
                         std::vector<int> & assocAllRanks, std::vector<int> & ioPoolRanks);

/// @brief Emulate the Round Robin MPI distribution
/// @details This funcion is intended to be used by the standalone application that builds
/// the input file set. The standalone app is to be run on a single process so it needs
/// a way to emulate the round robin distribution.
/// @param targetCommSize targeted number of ranks in "all" communicator group
/// @param sourceLocIndices index list generated by initSourceIndices
/// @param sourceRecNums record numbers generated by assignRecordNumbers
/// @param locIndicesAllRanks contains distributed locations for all ranks
/// @param locIndicesStarts contains the starting points for each rank in locIndicesAllRanks
/// @param locIndicesCounts contains the count of indices for each rank in locIndicesAllRanks
/// @param recNumsAllRanks same as locIndicesAllRanks except for local record numbers
void emulateRoundRobinDist(const int targetCommSize,
                           const std::vector<std::size_t> & sourceLocIndices,
                           const std::vector<std::size_t> & sourceRecNums,
                           std::vector<std::size_t> & locIndicesAllRanks,
                           std::vector<int> & locIndicesStarts,
                           std::vector<int> & locIndicesCounts,
                           std::vector<std::size_t> & recNumsAllRanks);

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
void checkForRequiredVars(const ioda::Group & srcGroup, const std::string & sourceName,
                          DateTimeFormat & dtimeFormat, bool & emptyFile) {
    // Get number of locations from obs source
    const std::size_t sourceNlocs = srcGroup.vars.open("Location").getDimensions().dimsCur[0];
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
      const std::string errorMsg =
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
}

//--------------------------------------------------------------------------------
void readSourceDtimeVar(const ioda::Group & srcGroup, const bool emptyFile,
                        const DateTimeFormat dtimeFormat, std::vector<int64_t> & dtimeVals,
                        std::string & dtimeEpoch) {
    // Initialize the output variables to values corresponding to an empty file. That way
    // if we have an empty file, then we can skip the file read and broadcast steps.
    dtimeVals.resize(0);
    dtimeEpoch = "seconds since 1970-01-01T00:00:00Z";

    if (!emptyFile) {
        // Read in variable data (converting if necessary) and determine epoch value
        ioda::Variable dtimeVar;
        if (dtimeFormat == DateTimeFormat::Epoch) {
            // Simply read in var values and copy the units attribute
            dtimeVar = srcGroup.vars.open("MetaData/dateTime");
            dtimeVar.atts.open("units").read<std::string>(dtimeEpoch);
            dtimeVar.read<int64_t>(dtimeVals);
        } else if (dtimeFormat == DateTimeFormat::String) {
            // Set the epoch to the linux standard epoch
            const std::string epochDtimeString = std::string("1970-01-01T00:00:00Z");
            dtimeEpoch = std::string("seconds since ") + epochDtimeString;

            std::vector<std::string> dtStrings;
            dtimeVar = srcGroup.vars.open("MetaData/datetime");
            dtimeVar.read<std::string>(dtStrings);

            const util::DateTime epochDtime(epochDtimeString);
            dtimeVals =  convertDtStringsToTimeOffsets(epochDtime, dtStrings);
        } else if (dtimeFormat == DateTimeFormat::Offset) {
            // Set the epoch to the "date_time" global attribute
            int refDtimeInt;
            srcGroup.atts.open("date_time").read<int>(refDtimeInt);

            const int year = refDtimeInt / 1000000;     // refDtimeInt contains YYYYMMDDhh
            int tempInt = refDtimeInt % 1000000;
            const int month = tempInt / 10000;       // tempInt contains MMDDhh
            tempInt = tempInt % 10000;
            const int day = tempInt / 100;           // tempInt contains DDhh
            const int hour = tempInt % 100;
            const util::DateTime refDtime(year, month, day, hour, 0, 0);

            dtimeEpoch = std::string("seconds since ") + refDtime.toString();

            std::vector<float> dtTimeOffsets;
            dtimeVar = srcGroup.vars.open("MetaData/time");
            dtimeVar.read<float>(dtTimeOffsets);
            dtimeVals.resize(dtTimeOffsets.size());
            for (std::size_t i = 0; i < dtTimeOffsets.size(); ++i) {
                dtimeVals[i] = static_cast<int64_t>(lround(dtTimeOffsets[i] * 3600.0));
            }
        }
    }
}

//--------------------------------------------------------------------------------
void initSourceIndices(const ioda::Group & srcGroup, const bool emptyFile,
        const std::vector<int64_t> & dtimeValues, const util::TimeWindow & timeWindow,
        const bool applyLocCheck, std::vector<float> & lonValues,
        std::vector<float> & latValues, std::vector<std::size_t> & sourceLocIndices,
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
            const float lonFillValue = ioda::detail::getFillValue<float>(lonFvData);
            ioda::detail::FillValueData_t latFvData = latVar.getFillValue();
            const float latFillValue = ioda::detail::getFillValue<float>(latFvData);

            // Keep all locations that fall inside the timing window. Note numLocsSelected
            // will be set to the number of locations stored in the output vectors after
            // exiting the following for loop.
            const std::vector<bool> timeMask = timeWindow.createTimeMask(dtimeValues);
            for (std::size_t i = 0; i < dtimeValues.size(); ++i) {
                // Check the timing window first since having a location outside the timing
                // window likely occurs more than having issues with the lat and lon values.
                // Note that a datetime that appears on the lower time boundary will be
                // accepted if the `bound to include` parameter is `begin`, and rejected
                // otherwise. The opposite logic applies on the upper time boundary.
                // This is done to prevent such a datetime appearing in two adjacent windows.
                bool keepThisLocation = timeMask[i];
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
void assignRecordNumbers(const ioda::Group & srcGroup, const bool emptyFile,
                         const std::vector<int64_t> & dtimeValues,
                         const std::vector<float> & lonValues,
                         const std::vector<float> & latValues,
                         const std::vector<std::size_t> & sourceLocIndices,
                         const std::vector<std::string> & obsGroupVarList,
                         std::vector<std::size_t> & sourceRecNums) {
    // Initialize the output variables to values corresponding to an empty file. That way
    // if we have an empty file, then we can skip the file read and broadcast steps.
    sourceRecNums.resize(0);

    if (!emptyFile) {
        // If the obsGroupVarList is empty, then the obs grouping feature is not being
        // used and the record number assignment can simply be sequential numbering
        // starting with zero. Otherwise, assign unique record numbers to each unique
        // combination of the values in the obsGroupVarList.
        const std::size_t locSize = sourceLocIndices.size();
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
    }
}

//--------------------------------------------------------------------------------
// Special case for broadcasting a DateTimeFormat enum type via eckit broadcast.
void broadcastDateTimeFormat(const eckit::mpi::Comm & comm, DateTimeFormat & enumVar,
                             const std::size_t root) {
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
// Definitions of public functions
//--------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
std::string filePrepGroupName() { return std::string("_iodaFilePrepInfo"); }

//------------------------------------------------------------------------------------
void applyMpiDistribution(const std::shared_ptr<ioda::Distribution> & dist,
                          const bool emptyFile, const std::vector<float> & lonValues,
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

//------------------------------------------------------------------------------------
void emulateRoundRobinDist(const int targetCommSize,
                           const std::vector<std::size_t> & sourceLocIndices,
                           const std::vector<std::size_t> & sourceRecNums,
                           std::vector<std::size_t> & locIndicesAllRanks,
                           std::vector<int> & locIndicesStarts,
                           std::vector<int> & locIndicesCounts,
                           std::vector<std::size_t> & recNumsAllRanks) {
    // In round robin the record numbers are doled out to the MPI task, where the
    // record number modulo communicator size denotes the destination rank number (ie, like
    // dealing cards).
    //
    // Also with round robin every MPI task gets a mutually exclusive set of locations
    // (ie, no overlap) which means the locIndicesAllRanks and recNumsAllRanks are
    // both of the same size as sourceLocIndices and sourceRecNums.
    //
    // Note that locIndicesStarts and locIndicesCounts are of size equal to the size
    // of the target communicator group.
    locIndicesAllRanks.resize(sourceLocIndices.size());
    locIndicesStarts.resize(targetCommSize, 0);
    locIndicesCounts.resize(targetCommSize, 0);
    recNumsAllRanks.resize(sourceLocIndices.size());

    // First determine the starts and counts since this can be done in a straight
    // forward manner. The record numbers go from 0 to n-1 so the number of records is
    // given by the max record number plus 1.
    for (std::size_t i = 0; i < sourceLocIndices.size(); ++i) {
        const int destRank = sourceRecNums[i] % targetCommSize;
        locIndicesCounts[destRank] += 1;
    }
    // Note locIndicesStarts[0] is already set to zero
    for (std::size_t i = 1; i < targetCommSize; ++i) {
        locIndicesStarts[i] = locIndicesStarts[i - 1] + locIndicesCounts[i - 1];
    }

    // Reorder locations and recNums by their destination rank
    std::vector<int> counters(targetCommSize, 0);
    for (std::size_t i = 0; i < locIndicesAllRanks.size(); ++i) {
        const int destRank = sourceRecNums[i] % targetCommSize;
        const int destIndex = locIndicesStarts[destRank] + counters[destRank];
        locIndicesAllRanks[destIndex] = sourceLocIndices[i];
        recNumsAllRanks[destIndex] = sourceRecNums[i];
        counters[destRank] += 1;
    }
}

//------------------------------------------------------------------------------------
void emulateMpiSplitComm(const int targetCommSize, const IoPoolGroupMap & rankGrouping,
                         std::vector<int> & assocAllRanks, std::vector<int> & ioPoolRanks) {
    // First generate the associated rank vector which shows the io pool rank that
    // every rank in the target commAll communicator group is associated with.
    assocAllRanks.resize(targetCommSize);
    ioPoolRanks.resize(targetCommSize);
    for (const auto & rankPair : rankGrouping) {
        const int poolRank = rankPair.first;
        assocAllRanks[poolRank] = poolRank;
        for (const auto & assocRankIndex : rankPair.second) {
            assocAllRanks[assocRankIndex] = poolRank;
        }
    }

    // Go through the associated rank vector and determine what the io pool rank numbers
    // are for each commAll rank. -1 means that this rank is not in the io pool.
    //
    // Note that the MPI split command is using the commAll rank number as a key so
    // the assigned pool ranks increase as the commAll rank numbers increase.
    int poolRank = 0;
    for (std::size_t i = 0; i < targetCommSize; ++i) {
        if (assocAllRanks[i] == i) {
            // On a pool rank
            ioPoolRanks[i] = poolRank;
            poolRank++;
        } else {
            // On a non-pool rank
            ioPoolRanks[i] = -1;
        }
    }
}

//------------------------------------------------------------------------------------
void emulateMpiDistribution(const std::string & distName, const bool emptyFile,
                            const int targetCommSize, const int targetPoolSize,
                            const IoPoolGroupMap & rankGrouping,
                            const std::vector<std::size_t> & sourceLocIndices,
                            const std::vector<std::size_t> & sourceRecNums,
                            std::vector<int> & assocAllRanks,
                            std::vector<int> & ioPoolRanks,
                            std::vector<std::size_t> & locIndicesAllRanks,
                            std::vector<int> & locIndicesStarts,
                            std::vector<int> & locIndicesCounts,
                            std::vector<std::size_t> & recNumsAllRanks) {
    // Check for supported distributions (by name)
    if (distName != "RoundRobin") {
        const std::string errMsg =
            std::string("emulateMpiDistribution: Unrecognized distribution name: ") +
            distName + std::string("\n    Supported distributions: RoundRobin");
        throw Exception(errMsg, ioda_Here());
    }

    // Expand the rankGrouping information into lists that describe the target
    // io pool structure.
    emulateMpiSplitComm(targetCommSize, rankGrouping, assocAllRanks, ioPoolRanks);

    // Emulate the mpi distibution. Generate the local location indices and local
    // record numbers for all ranks. Only supporting round robin for now, but putting
    // in if-else struct for when more distributions are added in later.
    if (distName == "RoundRobin") {
        emulateRoundRobinDist(targetCommSize, sourceLocIndices, sourceRecNums,
                              locIndicesAllRanks, locIndicesStarts,
                              locIndicesCounts, recNumsAllRanks);
    }
}

//--------------------------------------------------------------------------------
void extractGlobalInfoFromSource(const eckit::mpi::Comm & comm,
    const ioda::Group & srcGroup, const std::string & readerSource,
    const util::TimeWindow & timeWindow, const bool applyLocCheck,
    const std::vector<std::string> & obsGroupVarList, std::vector<int64_t> & dtimeValues,
    std::vector<float> & lonValues, std::vector<float> & latValues,
    std::vector<std::size_t> & sourceLocIndices, std::vector<std::size_t> & sourceRecNums,
    bool & emptyFile, DateTimeFormat & dtimeFormat, std::string & dtimeEpoch,
    std::size_t & globalNlocs, std::size_t & sourceNlocs,
    std::size_t & sourceNlocsInsideTimeWindow, std::size_t & sourceNlocsOutsideTimeWindow,
    std::size_t & sourceNlocsRejectQC) {

    if (comm.rank() == 0) {
        // Check for required variables
        checkForRequiredVars(srcGroup, readerSource, dtimeFormat, emptyFile);

        // Read and convert the dtimeValues to the current epoch format if older formats are
        // being used in the source.
        readSourceDtimeVar(srcGroup, emptyFile, dtimeFormat, dtimeValues, dtimeEpoch);

        // Convert the window start and end times to int64_t offsets from the dtimeEpoch
        // value. This will provide for a very fast "inside the timing window check".
        util::DateTime epochDt;
        convertEpochStringToDtime(dtimeEpoch, epochDt);
        timeWindow.setEpoch(epochDt);

        // The initSourceIndices function will skip QC checks if applyLocCheck is false.
        // in this case the sourceLocIndices vector is initialized to the entire set from
        // the obs source. The initSourceIndices uses the lon and lat values so it
        // also will read in those values from the obs source.
        initSourceIndices(srcGroup, emptyFile, dtimeValues, timeWindow,
                          applyLocCheck, lonValues, latValues, sourceLocIndices,
                          sourceNlocs, sourceNlocsInsideTimeWindow,
                          sourceNlocsOutsideTimeWindow,
                          sourceNlocsRejectQC, globalNlocs);

        // Assign a record number for each entry in sourceLocIndices. Use the obs grouping
        // feature if obsGroupVarList is not empty. Otherwise assign sequential
        // nubmers starting with zero.
        assignRecordNumbers(srcGroup, emptyFile, dtimeValues, lonValues, latValues,
                            sourceLocIndices, obsGroupVarList, sourceRecNums);
    }

    // broadcast variables
    oops::mpi::broadcastBool(comm, emptyFile, 0);
    broadcastDateTimeFormat(comm, dtimeFormat, 0);
    oops::mpi::broadcastString(comm, dtimeEpoch, 0);
    comm.broadcast(globalNlocs, 0);
    comm.broadcast(sourceNlocs, 0);
    comm.broadcast(sourceNlocsInsideTimeWindow, 0);
    comm.broadcast(sourceNlocsOutsideTimeWindow, 0);
    comm.broadcast(sourceNlocsRejectQC, 0);

    // broadcast vector data
    oops::mpi::broadcastVector<int64_t>(comm, dtimeValues, 0);
    oops::mpi::broadcastVector<float>(comm, latValues, 0);
    oops::mpi::broadcastVector<float>(comm, lonValues, 0);
    oops::mpi::broadcastVector<std::size_t>(comm, sourceLocIndices, 0);
    oops::mpi::broadcastVector<std::size_t>(comm, sourceRecNums, 0);
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
    distributionMap.clear();
    if (ioPool.commPool() != nullptr) {
        // On an io pool member, save your own local indices, then collect the local
        // source indices from all of the associated non io pool members.
        distributionMap[ioPool.commAll().rank()] = localLocIndices;
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
void readerGatherAssociatedRanks(const ReaderPoolBase & ioPool,
                                 std::vector<int> & assocAllRanks,
                                 std::vector<int> & ioPoolRanks,
                                 std::vector<std::string> & assocFileNames) {
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

    // Gather up the associated new input file names.
    assocFileNames = std::vector<std::string>(1, ioPool.newInputFileName());
    oops::mpi::allGatherv(ioPool.commAll(), assocFileNames);
}

//--------------------------------------------------------------------------------
void readerGatherLocationInfo(const ReaderPoolBase & ioPool,
                              std::vector<std::size_t> & locIndicesAllRanks,
                              std::vector<int> & locIndicesStarts,
                              std::vector<int> & locIndicesCounts,
                              std::vector<std::size_t> & recNumsAllRanks) {
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
    for (int i = 1; i < numTasks; ++i) {
        locIndicesStarts[i] = locIndicesStarts[i - 1] + locIndicesCounts[i - 1];
        recvSize += locIndicesCounts[i];
    }
    locIndicesAllRanks.resize(recvSize);
    ioPool.commAll().allGatherv(ioPool.index().begin(), ioPool.index().end(),
                                locIndicesAllRanks.begin(), locIndicesCounts.data(),
                                locIndicesStarts.data());

    // The same start, count pattern for the location indices als applies
    // for the record numbers.
    recNumsAllRanks.resize(recvSize);
    ioPool.commAll().allGatherv(ioPool.recnums().begin(), ioPool.recnums().end(),
                                recNumsAllRanks.begin(), locIndicesCounts.data(),
                                locIndicesStarts.data());
}

//--------------------------------------------------------------------------------
void readerSetFileSelection(const int allRank,
                            const std::vector<int> & assocAllRanks,
                            const std::vector<std::size_t> & locIndicesAllRanks,
                            const std::vector<int> & locIndicesStarts,
                            const std::vector<int> & locIndicesCounts,
                            const std::vector<std::size_t> & recNumsAllRanks,
                            std::vector<std::size_t> & indices,
                            std::vector<std::size_t> & recnums,
                            std::vector<int> & destAllRanks,
                            std::vector<int> & starts,
                            std::vector<int> & counts) {
    // Walk though the assocAllRanks and use the entries matching allRank to pull
    // out the indices associated with allRank. Generate the start, count values as
    // you go. Note the algorithm below is not very runtime efficient (eg. vector
    // push_back calls) and it is relying on the io pool not getting very
    // big (ie, 10s of ranks at the most).
    int start = 0;
    for (std::size_t i = 0; i < assocAllRanks.size(); ++i) {
        if (assocAllRanks[i] == allRank) {
            // Attach the entries in locIndices* to the output vectors.
            const int inputStart = locIndicesStarts[i];
            const int inputCount = locIndicesCounts[i];
            indices.insert(indices.end(), locIndicesAllRanks.begin() + inputStart,
                           locIndicesAllRanks.begin() + inputStart + inputCount);
            recnums.insert(recnums.end(), recNumsAllRanks.begin() + inputStart,
                           recNumsAllRanks.begin() + inputStart + inputCount);
            std::vector<int> dest(inputCount, i);
            destAllRanks.insert(destAllRanks.end(), dest.begin(), dest.end());
            starts.push_back(start);
            counts.push_back(inputCount);
            start += inputCount;
        }
    }
}

//--------------------------------------------------------------------------------
void readerWriteInputFileMpiMapping(const Group & srcGroup,
                                    const std::vector<std::size_t> & indices,
                                    const std::vector<std::size_t> & recnums,
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

    // Create the recordNumbers and destAllRanks variable in the top level group.
    VariableCreationParameters intParams = VariableCreationParameters::defaults<int>();
    intParams.setFillValue<int>(util::missingValue<int>());
    intParams.noCompress();
    std::string varName = filePrepGroupName() + std::string("/destinationRank");
    Variable destRankVar = destGroup.vars.createWithScales<int>(
                           varName, { destLocVar }, intParams);
    varName = filePrepGroupName() + std::string("/recordNumbers");
    Variable recNumVar = destGroup.vars.createWithScales<int64_t>(
                         varName, { destLocVar }, int64Params);

    // Only write out the values if nlocs > zero
    if (nlocs > 0) {
        // Location
        // Conversion from size_t to int64_t should be safe. Only unsafe for values greater
        // than ~1/2 the max size_t value.
        std::vector<int64_t> destInt64Values(indices.begin(), indices.end());
        destLocVar.write<int64_t>(destInt64Values);

        // destinationRank
        destRankVar.write<int>(destAllRanks);

        // recordNumbers
        destInt64Values.assign(recnums.begin(), recnums.end());
        recNumVar.write<int64_t>(destInt64Values);
    }
}

//--------------------------------------------------------------------------------
void readerWriteInputFilePreparedVars(const Group & srcGroup,
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
        const bool doLocSelection = setDoLocSelection(varName, firstDimName);

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
                               const std::string & inputFileName,
                               const std::vector<std::size_t> & indices,
                               const std::vector<std::size_t> & recnums,
                               const std::vector<int> & destAllRanks,
                               const std::vector<int> & starts,
                               const std::vector<int> & counts,
                               const std::vector<int64_t> & dtimeValues,
                               const std::string & dtimeEpoch,
                               const std::vector<float> & lonValues,
                               const std::vector<float> & latValues) {
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

    // Write out the mpi mapping data held in indices, recnums and destAllRanks. Place
    // these variables in a special group in the output file.
    // Then write out the prepared variables (dateTime, latitude, longitude)
    const ioda::Group filePrepGroup = destGroup.create(filePrepGroupName());
    readerWriteInputFileMpiMapping(srcGroup, indices, recnums, destAllRanks, destGroup);
    readerWriteInputFilePreparedVars(srcGroup, indices, dtimeValues, dtimeEpoch,
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
void readerBuildPrepInfoFile(const ReaderPoolBase & ioPool,
                             const int targetCommAllSize,
                             const int targetCommPoolSize,
                             const std::vector<int> & allNlocs,
                             const std::vector<int> & ioPoolRanks,
                             const std::vector<int> & assocRanks) {
    // Need to form the name of the file which is based on the path and name of the
    // prepared input files. The newInputFileName from the ioPool already has the
    // rank number suffix attached. Want to replace that suffix with "_prep_file_info"
    // to form the name of the prep info file.
    const std::string prepInfoFileName = ioPool.prepInfoFileName();

    // Open up an hdf5 writer backend and transfer the prep info data into the
    // output file. We need to create a new eckit configuration for the writer
    // engine factory.
    // Third and fourth arguments to constructFileWriterFromConfig are
    // "write multiple files" and "is parallel" respectively. We want
    // "write multiple files" to be false since we are tagging on the io pool
    // rank above.
    eckit::LocalConfiguration engineConfig =
        Engines::constructFileBackendConfig("hdf5", prepInfoFileName);
    std::unique_ptr<Engines::WriterBase> writerEngine =
        Engines::constructFileWriterFromConfig(ioPool.commAll(), ioPool.commTime(),
                                               false, false, engineConfig);
    Group destGroup = writerEngine->getObsGroup();

    // Add global attributes containing global information from the source file
    readerAddSupplementalAttributes(ioPool, targetCommAllSize, targetCommPoolSize, destGroup);

    // Add mpi and io pool related information
    // Create the Rank dimension
    const int numRanks = allNlocs.size();
    VariableCreationParameters intParams = VariableCreationParameters::defaults<int>();
    intParams.setFillValue<int>(util::missingValue<int>());
    intParams.noCompress();
    Variable rankVar = destGroup.vars.create<int>(
                       "Rank", { numRanks }, { numRanks }, intParams);
    rankVar.setIsDimensionScale("Rank");
    std::vector<int> rankNumbers(numRanks);
    std::iota(rankNumbers.begin(), rankNumbers.end(), 0);
    rankVar.write<int>(rankNumbers);

    // nlocs data
    destGroup.vars
        .createWithScales<int>("numberLocations", { rankVar }, intParams)
        .write(allNlocs);

    // rank allocation for the io pool that is consistent with the input file set
    destGroup.vars
        .createWithScales<int>("ioPoolRanks", { rankVar }, intParams)
        .write(ioPoolRanks);

    // association between pool member and non pool member ranks
    destGroup.vars
        .createWithScales<int>("rankAssociation", { rankVar }, intParams)
        .write(assocRanks);

    oops::Log::info() << "readerBuildPrepInfoFile: created prep info file: "
                      << prepInfoFileName << std::endl;
}

//--------------------------------------------------------------------------------
void readerBuildInputFiles(const ReaderPoolBase & ioPool,
                           const int targetCommAllSize,
                           const int targetCommPoolSize,
                           const Group & srcGroup,
                           const std::vector<int> & assocAllRanks,
                           const std::vector<int> & ioPoolRanks,
                           const std::vector<std::string> & assocFileNames,
                           const std::vector<std::size_t> & locIndicesAllRanks,
                           const std::vector<int> & locIndicesStarts,
                           const std::vector<int> & locIndicesCounts,
                           const std::vector<std::size_t> & recNumsAllRanks,
                           const std::vector<int64_t> & dtimeValues,
                           const std::string & dtimeEpoch,
                           const std::vector<float> & lonValues,
                           const std::vector<float> & latValues) {
    // Single file that supplements the input file set with global stats from the
    // source file, and mpi, ioPool related information. Note that locIndicesCounts
    // holds the nlocs value for each rank in the commAll communicator group.
    readerBuildPrepInfoFile(ioPool, targetCommAllSize, targetCommPoolSize,
                            locIndicesCounts, ioPoolRanks, assocAllRanks);

    // Identify which ranks, from the commAll communicator group, are io pool members.
    // These are the unique values in assocAllRanks.
    std::set<int> ioPoolMembers(assocAllRanks.begin(), assocAllRanks.end());
    for (const auto & i : ioPoolMembers) {
        // For this io pool member, record the io pool rank (for the file suffix),
        // and the list of indices that go into the associated file (with the
        // corresponding start, count values).
        int allRank = i;
        int poolRank = ioPoolRanks[i];
        const std::string inputFileName = assocFileNames[i];
        std::vector<int> starts;
        std::vector<int> counts;
        std::vector<std::size_t> indices;
        std::vector<std::size_t> recnums;
        std::vector<int> destAllRanks;
        readerSetFileSelection(allRank, assocAllRanks, locIndicesAllRanks, locIndicesStarts,
            locIndicesCounts, recNumsAllRanks, indices, recnums, destAllRanks, starts, counts);

        // Create the associate file, and record it for subsequent removal
        readerBuildAssocInputFile(ioPool, srcGroup, allRank, poolRank, inputFileName,
                                  indices, recnums, destAllRanks, starts, counts,
                                  dtimeValues, dtimeEpoch, lonValues, latValues);
        oops::Log::info() << "readerBuildInputFiles: created new input file: "
                          << inputFileName << std::endl;
    }
}

//--------------------------------------------------------------------------------
void readerAddSupplementalAttributes(const ReaderPoolBase & ioPool,
                                     const int targetCommAllSize,
                                     const int targetCommPoolSize,
                                     ioda::Group & destGroup) {
    // Add in information about the MPI communicator sizes
    destGroup.atts.add<int>("mpiCommAllSize", targetCommAllSize);
    destGroup.atts.add<int>("mpiCommPoolSize", targetCommPoolSize);

    // Add in location information about the original source file.
    destGroup.atts.add<int>("globalNlocs", ioPool.globalNlocs());
    destGroup.atts.add<int>("sourceNlocs", ioPool.sourceNlocs());
    destGroup.atts.add<int>("sourceNlocsInsideTimeWindow",
                            ioPool.sourceNlocsInsideTimeWindow());
    destGroup.atts.add<int>("sourceNlocsOutsideTimeWindow",
                            ioPool.sourceNlocsOutsideTimeWindow());
    destGroup.atts.add<int>("sourceNlocsRejectQC", ioPool.sourceNlocsRejectQC());

    // date time epoch value
    destGroup.atts.add<std::string>("dtimeEpoch", ioPool.dtimeEpoch());
}

//--------------------------------------------------------------------------------
void readerRemoveFilePrepGroup(VarUtils::Vec_Named_Variable & varList,
                               VarUtils::Vec_Named_Variable & dimVarList,
                               VarUtils::VarDimMap & dimsAttachedToVars) {
    // Go through each list and remove the variable entries that contain the
    // group name given in filePrepGroupName

    // dimension variables
    auto dimPos = dimVarList.begin();
    while (dimPos != dimVarList.end()) {
        if (dimPos->name.find(filePrepGroupName()) != std::string::npos) {
            dimVarList.erase(dimPos);
        } else {
            ++dimPos;
        }
    }

    // non dimension variables
    auto varPos = varList.begin();
    while (varPos != varList.end()) {
        if (varPos->name.find(filePrepGroupName()) != std::string::npos) {
            if (dimsAttachedToVars.find(*varPos) != dimsAttachedToVars.end()) {
                dimsAttachedToVars.erase(*varPos);
            }
            varList.erase(varPos);
        } else {
            ++varPos;
        }
    }
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
                // Skip over the special file preparation group info
                if (groupName == filePrepGroupName()) {
                    continue;
                }
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

            // Remove the special file preparation info group
            readerRemoveFilePrepGroup(regularVarList, dimVarList, dimsAttachedToVars);

            // List out dimension variables (these all belong in the top level group).
            yamlStream << "dimensions:" << std::endl;
            VarUtils::listDimensionsAsYaml(dimVarList, constants::indent4, yamlStream);

            // List out regular variables.
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
void readerDeserializeGroupStructure(const ReaderPoolBase & ioPool, ioda::Group & memGroup,
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
    VarUtils::createDimensionsFromConfig(memGroup.vars, dimConfigs, ioPool.globalNlocs());

    // create variables from the "variables" section
    std::vector<eckit::LocalConfiguration> varConfigs;
    config.get("variables", varConfigs);
    VarUtils::createVariablesFromConfig(memGroup.vars, varConfigs, ioPool.globalNlocs());
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
    readerDeserializeGroupStructure(ioPool, memGroup, groupStructureYaml);
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
                                         const std::size_t sourceNlocs,
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
        dimSizes["Location"] = sourceNlocs;       // override as noted above
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
bool setDoLocSelection(const std::string & varName, const std::string & firstDimName) {
    // Need to do location selection when varName is Location or the firstDimName is Location
    bool doLocSelection = false;
    if (varName == "Location") {
        doLocSelection = true;
    } else if (firstDimName == "Location") {
        doLocSelection = true;
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
        if (static_cast<int>(strlen(stringElement)) > maxStringLength) {
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
        for (int j = 0; j < strLen; ++j) {
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
    for (int i = 0; i < charArrayShape[0]; ++i) {
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
    for (int i = 0; i < charArrayShape[0]; ++i) {
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
            // skip over the entry for this rank
            if (static_cast<size_t>(distMap.first) == ioPool.commAll().rank()) {
                continue;
            }
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
                           std::string & groupStructureYaml) {
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
    std::size_t srcNlocs;
    if (ioPool.commPool() != nullptr) {
        // On a pool member, get the number of locations from the associated input file
        srcNlocs = fileGroup.vars.open("Location").getDimensions().dimsCur[0];
    } else {
        // On a non-pool member, set the srcNlocs to zero (note that srcBuffer is not
        // used on the non-pool members.
        srcNlocs = 0;
    }
    const std::size_t destNlocs = memGroup.vars.open("Location").getDimensions().dimsCur[0];

    const ioda::Dimensions_t sourceMaxElements =
        calcSourceMaxElements(ioPool, srcNlocs, config);
    const ioda::Dimensions_t destMaxElements = calcDestMaxElements(ioPool, config);
    const ioda::Dimensions_t maxDataTypeSize = getMaxDataTypeSize();
    std::vector<char>srcBuffer(sourceMaxElements * maxDataTypeSize);
    std::vector<char>destBuffer(destMaxElements * maxDataTypeSize);

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
        const bool doLocSelection = setDoLocSelection(dimName, dimName);

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
                const std::vector<std::size_t> & myLocIndices =
                    ioPool.distributionMap().at(ioPool.commAll().rank());
                readerSaveDestVarLocal(dimName, srcBuffer, myLocIndices, destNlocs,
                                       doLocSelection, destBuffer, destVar);
            }
        }

        // Transfer data from the pool members to their assigned non pool member ranks.
        readerSaveDestVarGlobal(ioPool, dimName, srcBuffer, destNlocs, doLocSelection,
                                varNumber, destBuffer, destVar);
        varNumber += 1;
    }

    // Walk through the variables section of the configuration and transfer the data
    // from pool member to itself and its assigned ranks.
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
        const bool doLocSelection = setDoLocSelection(varName, varDimNames[0]);

        // On pool members only, read in from the source into the source buffer
        // and save in the local obs space.
        if (ioPool.commPool() != nullptr) {
            // Read variable from the source (input file), and replace fill with
            // the corresponding JEDI missing value.
            Variable srcVar = fileGroup.vars.open(varName);
            readerLoadSourceVarReplaceFill(ioPool, srcVar, varName, srcBuffer);

            // Transfer the variable data to this rank's obs space
            const std::vector<std::size_t> & myLocIndices =
                ioPool.distributionMap().at(ioPool.commAll().rank());
            readerSaveDestVarLocal(varName, srcBuffer, myLocIndices, destNlocs,
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
                          const ioda::Dimensions_t adjustNlocs, const std::size_t globalNlocs,
                          Has_Variables & destVars, ioda::Dimensions_t & globalMaxElements,
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
    std::vector<ioda::Dimensions_t> chunkDims = varDims.dimsCur;
    if (adjustNlocs >= 0) {
        chunkDims[0] = VarUtils::getLocationChunkSize(globalNlocs);
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
            bool doLocSelection = setDoLocSelection(varName, firstDimName);

            // Transfer the variable data to this rank's obs space
            readerSaveDestVarLocal(varName, srcBuffer, locIndices, destNlocs,
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
               readerCreateVariable<T>(srcVarName, fileVar, adjustNlocs,
                                       VarUtils::getLocationChunkSize(ioPool.globalNlocs()),
                                       memGroup.vars, globalMaxElements, maxElements);
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
        std::vector<ioda::Dimensions_t> chunkDims(1,
                                        VarUtils::getLocationChunkSize(ioPool.globalNlocs()));
        params.setChunks(chunkDims);
        ioda::Variable dtimeVar =
            memGroup.vars.create<int64_t>("MetaData/dateTime", varDims, params);
        dtimeVar.atts.add<std::string>("units", dtimeEpoch);
    }

    // Make new dimension scales
    for (auto& dim : dimVarList) {
        memGroup.vars[dim.name].setIsDimensionScale(dim.name);
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
