/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/ReaderPoolBase.h"

#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"

#include "ioda/Copying.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace IoPool {

// For the MPI communicator splitting
constexpr int readerPoolColor = 3;
constexpr int readerNonPoolColor = 4;
const char readerPoolCommName[] = "readerIoPool";
const char readerNonPoolCommName[] = "readerNonIoPool";

//------------------------------------------------------------------------------------
// Reader pool creation parameters
//------------------------------------------------------------------------------------
ReaderPoolCreationParameters::ReaderPoolCreationParameters(
            const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
            const oops::RequiredPolymorphicParameter
                <Engines::ReaderParametersBase, Engines::ReaderFactory> & readerParams,
            const util::TimeWindow timeWindow,
            const std::vector<std::string> & obsVarNames,
            const std::shared_ptr<Distribution> & distribution,
            const std::vector<std::string> & obsGroupVarList,
            const std::string & inputFilePrepType)
                : IoPoolCreationParameters(commAll, commTime),
                  readerParams(readerParams), timeWindow(timeWindow),
                  obsVarNames(obsVarNames), distribution(distribution),
                  obsGroupVarList(obsGroupVarList), inputFilePrepType(inputFilePrepType) {
}

//------------------------------------------------------------------------------------
// Reader pool base class
//------------------------------------------------------------------------------------
ReaderPoolBase::ReaderPoolBase(const IoPoolParameters & configParams,
                               const ReaderPoolCreationParameters & createParams)
                    : IoPoolBase(configParams, createParams.commAll, createParams.commTime,
                                 readerPoolColor, readerNonPoolColor,
                                 readerPoolCommName, readerNonPoolCommName),
                      readerParams_(createParams.readerParams),
                      timeWindow_(createParams.timeWindow),
                      obsVarNames_(createParams.obsVarNames),
                      distribution_(createParams.distribution),
                      obsGroupVarList_(createParams.obsGroupVarList),
                      inputFilePrepType_(createParams.inputFilePrepType),
                      sourceNlocs_(0), nrecs_(0), sourceNlocsInsideTimeWindow_(0),
                      sourceNlocsOutsideTimeWindow_(0), sourceNlocsRejectQC_(0) {
    // Save a persistent copy of the JEDI missing value for a string variable that can
    // be used to properly replace a string fill value from the obs source with this
    // JEDI missing value. The replaceFillWithMissing function needs a char * pointing
    // to this copy of the JEDI missing value to transfer that value to the obs space
    // container.
    stringMissingValue_ = std::make_shared<std::string>(util::missingValue<std::string>());
}

//------------------------------------------------------------------------------------
std::string ReaderPoolBase::prepInfoFileSuffix() { return std::string("_prep_file_info"); }

//--------------------------------------------------------------------------------------
std::string ReaderPoolBase::setPrepInfoFileName(const std::string & fileName) {
    std::string prepInfoFileName;
    if (this->commAll().rank() == 0) {
        // Construct the path, name to the new input file. Strip off the trailing suffix
        // (.nc4, .odb, .nc, etc) and replace with "_<ioPoolRank>.nc4". For now, we will
        // always use the hdf5 backend for these files.
        prepInfoFileName = Engines::formFileWithNewExtension(fileName, ".nc4");
        prepInfoFileName = Engines::formFileWithSuffix(prepInfoFileName, prepInfoFileSuffix());
    }
    oops::mpi::broadcastString(this->commAll(), prepInfoFileName, 0);
    return prepInfoFileName;
}

//--------------------------------------------------------------------------------------
std::string ReaderPoolBase::setNewInputFileName(const std::string & fileName,
                                                const int poolRankNumber) {
    std::string newInputFileName;
    // Determine the time communicator rank number. A value of -1 tells the
    // uniquifyFileName routing to not add a suffix for the time communicator rank.
    int timeRankNum = -1;
    if (this->commTime().size() > 1) {
        timeRankNum = this->commTime().rank();
    }
    // Form the rank number suffixes. The first argument to formFileSuffixFromRankNums
    // is "write multiple files" which needs to be true to allow formFileSuffixFromRankNums
    // to tag on the poolRank number.
    const std::string fileSuffix =
        Engines::formFileSuffixFromRankNums(true, poolRankNumber, timeRankNum);

    // Construct the path, name to the new input file. Strip off the trailing suffix
    // (.nc4, .odb, .nc, etc) and replace with "_<ioPoolRank>.nc4". For now, we will
    // always use the hdf5 backend for these files.
    newInputFileName = Engines::formFileWithNewExtension(fileName, ".nc4");
    newInputFileName = Engines::formFileWithSuffix(newInputFileName, fileSuffix);
    return newInputFileName;
}

}  // namespace IoPool
}  // namespace ioda
