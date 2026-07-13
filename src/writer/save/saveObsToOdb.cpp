/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/writer/save/saveObsToOdb.hpp"

#include <optional>
#include <string>
#include <fstream>

#include "ioda/Engines/ContainerFacade.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ODC.h"
#include "ioda/Engines/WriteOdbFile.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/OsdfFrameFacade.hpp"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace writer {

void saveOsdfToOdb(const ObsDataOutParameters &dataOutParams, const eckit::mpi::Comm &ioPoolComm,
                   std::unique_ptr<osdf::IFrame> &srcOsdf, osdf::FrameMetadata &osdfMetadata) {
  // saveObs (caller of this function) has already verified that the
  // output file type is ODB.

  const auto &writerParams = dynamic_cast<const Engines::WriteOdbFileParameters &>(
    dataOutParams.engine.value().engineParameters.value());
  std::string outputFileName = writerParams.fileName.value();
  std::string outputFileNameFromRank = outputFileName;

  oops::Log::info() << "Saving to file: " << outputFileName << std::endl;

  // Set up unique file names for each ioPool rank member
  if (ioPoolComm.size() > 1) {
    // todo(SRH): For now we are ignoring the time communicator rank number.
    // We eventually need to support this for when we output files on
    // successive time steps. Setting timeCommRank to -1 will cause the output
    // file name to not include the time communicator rank number. Only append
    // the rank number if there is more than one rank in the io pool.
    const int timeCommRank = -1;
    outputFileNameFromRank = ioda::Engines::uniquifyFileName(outputFileNameFromRank, true,
                                                             ioPoolComm.rank(), timeCommRank);
  }

  Engines::ODC::ODC_Parameters odcparams;
  odcparams.queryFile                    = writerParams.queryFileName;
  odcparams.mappingFile                  = writerParams.mappingFileName;
  odcparams.outputFile                   = outputFileNameFromRank;
  odcparams.odbType                      = writerParams.odbType;
  odcparams.missingObsSpaceVariableAbort = writerParams.missingObsSpaceVariableAbort;
  odcparams.ignoreChannelDimensionWrite  = writerParams.ignoreChannelDimensionWrite;

  std::shared_ptr<const detail::DataLayoutPolicy> dataLayoutPolicy
    = detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroupODB,
                                         odcparams.mappingFile, {});
  Engines::ContainerOptions containerOptions;

  OsdfFrameFacade container(*srcOsdf, osdfMetadata, true);
  container.initialize(srcOsdf->numRows(), osdfMetadata.getDimNums("Channel"), dataLayoutPolicy,
                       containerOptions);
  Engines::ODC::createFile(odcparams, container);

  ioPoolComm.barrier();

  // Merge files if writeMultipleFiles == false
  if (!dataOutParams.writeMultipleFiles.value()
       && ioPoolComm.rank() == 0 && outputFileNameFromRank != outputFileName) {
    const std::size_t mpiRankSize     = ioPoolComm.size();
    std::ofstream outFile(outputFileName,
                          std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    for (std::size_t irank = 0; irank < mpiRankSize; irank++) {
      const int timeCommRank = -1;
      const std::string inFileName
        = Engines::uniquifyFileName(outputFileName, true, irank, timeCommRank);
      std::ifstream inFile(inFileName, std::ios_base::binary | std::ios_base::in);
      outFile << inFile.rdbuf();
      inFile.close();
      std::remove(inFileName.c_str());
    }
    outFile.close();
  }
}

}  // namespace writer
}  // namespace ioda
