/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

#include "ioda/Engines/WriterBase.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// WriterParametersBase 
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// WriterCreationParameters
//---------------------------------------------------------------------
WriterCreationParameters::WriterCreationParameters(const eckit::mpi::Comm & comm,
                          const eckit::mpi::Comm & timeComm, const bool createMultipleFiles,
                          const bool isParallelIo)
                              : comm(comm), timeComm(timeComm),
                                createMultipleFiles(createMultipleFiles),
                                isParallelIo(isParallelIo) {
}

//---------------------------------------------------------------------
// WriterBase 
//---------------------------------------------------------------------

//--------------------------- public functions ---------------------------------------
WriterBase::WriterBase(const WriterCreationParameters & createParams)
                           : createParams_(createParams) {
}

//---------------------------------------------------------------------
// WriterProcBase 
//---------------------------------------------------------------------

//--------------------------- public functions ---------------------------------------
WriterProcBase::WriterProcBase(const WriterCreationParameters & createParams)
                                   : createParams_(createParams) {
}


}  // namespace Engines
}  // namespace ioda
