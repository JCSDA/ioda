/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

#include "ioda/Engines/ReaderBase.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// ReaderParametersBase 
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// ReaderCreationParameters
//---------------------------------------------------------------------

ReaderCreationParameters::ReaderCreationParameters(
                             const util::DateTime & winStart, const util::DateTime & winEnd,
                             const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                             const std::vector<std::string> & obsVarNames,
                             const bool isParallelIo)
                                 : winStart(winStart), winEnd(winEnd),
                                   comm(comm), timeComm(timeComm),
                                   obsVarNames(obsVarNames), isParallelIo(isParallelIo) {
}

//---------------------------------------------------------------------
// ReaderBase 
//---------------------------------------------------------------------

//--------------------------- public functions ---------------------------------------
ReaderBase::ReaderBase(const ReaderCreationParameters & createParams)
                           : createParams_(createParams) {
}

}  // namespace Engines
}  // namespace ioda
