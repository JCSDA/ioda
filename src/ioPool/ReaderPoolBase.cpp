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
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/ioPool/ReaderPoolUtils.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {

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
            const util::DateTime & winStart, const util::DateTime & winEnd,
            const std::vector<std::string> & obsVarNames,
            const std::shared_ptr<Distribution> & distribution,
            const std::vector<std::string> & obsGroupVarList)
                : IoPoolCreationParameters(commAll, commTime),
                  readerParams(readerParams), winStart(winStart), winEnd(winEnd),
                  obsVarNames(obsVarNames), distribution(distribution),
                  obsGroupVarList(obsGroupVarList) {
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
                      winStart_(createParams.winStart), winEnd_(createParams.winEnd),
                      obsVarNames_(createParams.obsVarNames),
                      distribution_(createParams.distribution),
                      obsGroupVarList_(createParams.obsGroupVarList),
                      sourceNlocs_(0), nrecs_(0), sourceNlocsInsideTimeWindow_(0),
                      sourceNlocsOutsideTimeWindow_(0), sourceNlocsRejectQC_(0) {
}

}  // namespace ioda