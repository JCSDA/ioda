/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/load/loadObsFromOdb.hpp"

#include "eckit/exception/Exceptions.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ODC.h"
#include "ioda/reader/OsdfFrameFacade.hpp"
#include "ioda/Engines/ReadOdbFile.h"
#include "ioda/ObsDataIoParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace reader {

//---------------------------------------------------------------------
void loadOsdfFromOdb(const ObsDataInParameters & dataInParams,
                     const eckit::mpi::Comm & ioPoolComm,
                     std::unique_ptr<osdf::IFrame>& destOsdf,
                     osdf::FrameMetadata& osdfMetadata)
{
  const auto &readerParams = dynamic_cast<const Engines::ReadOdbFileParameters&>(
    dataInParams.engine.value().engineParameters.value());

  if (dataInParams.readMultipleFiles.value()) {
    const std::string errMsg =
        "ioda::reader::loadOsdfFromOdb: Reading multiple files is not supported with ODB engine.";
    throw eckit::NotImplemented(errMsg, Here());
  }

  const std::string fileName = readerParams.fileName.value();
  const std::string missingFileAction = readerParams.missingFileAction.value();
  if (!Engines::haveFileReadAccess(fileName)) {
    if (missingFileAction == "warn") {
      oops::Log::info() << "WARNING: ioda::reader::loadOsdfFromOdb: input file is not "
                        << "readable, will continue with empty file representation" << std::endl
                        << "WARNING:     file: " << fileName << std::endl;
      // Leave destOsdf/osdfMetadata untouched (ie, no columns, no rows), matching the
      // state loadObs()/distributeOsdfMetadata() already tolerate for io pool ranks that
      // read zero locations from a (present) ODB file.
      return;
    } else if (missingFileAction != "error") {
      const std::string errMsg = std::string("Unrecognized input file missing action: ") +
                      missingFileAction;
      throw eckit::BadParameter(errMsg, Here());
    }
    throw eckit::ReadError(fileName, Here());
  }

  Engines::ODC::ODC_Parameters odcparams;
  odcparams.filename    = readerParams.fileName;
  odcparams.mappingFile = readerParams.mappingFileName;
  odcparams.queryFile   = readerParams.queryFileName;
  const util::DateTime missingDate = util::missingValue<util::DateTime>();
  // Time window filtering will be done later, once all data have been read.
  odcparams.timeWindowStart = missingDate;
  odcparams.timeWindowExtendedLowerBound =
    readerParams.timeWindowExtendedLowerBound.value() != boost::none ?
      readerParams.timeWindowExtendedLowerBound.value().value() : missingDate;
  odcparams.chunksPerProcess = readerParams.frameDistributionSpread;

  OsdfFrameFacade container(*destOsdf, osdfMetadata);
  Engines::ODC::openFile(odcparams, container, &ioPoolComm);
}

}  // namespace reader
}  // namespace ioda
