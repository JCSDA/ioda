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
#include "ioda/OsdfFrameFacade.hpp"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace writer {

void saveOsdfToOdb(const ObsDataOutParameters &dataOutParams, const eckit::mpi::Comm &ioPoolComm,
                   const int timeCommRank, std::unique_ptr<osdf::IFrame> &srcOsdf,
                   osdf::FrameMetadata &osdfMetadata) {
  // saveObs (caller of this function) has already verified that the
  // output file type is ODB.

  const auto &writerParams = dynamic_cast<const Engines::WriteOdbFileParameters &>(
    dataOutParams.engine.value().engineParameters.value());
  const std::string configFileName = writerParams.fileName.value();

  // Each time rank writes its own output file(s), so append the time communicator
  // rank number (when timeCommRank >= 0) to the file name. Then set up unique file
  // names for each ioPool rank member. Only append the io pool rank number if there
  // is more than one rank in the io pool.
  const bool multiRankPool = (ioPoolComm.size() > 1);
  const std::string outputFileName = ioda::Engines::uniquifyFileName(
    configFileName, multiRankPool, ioPoolComm.rank(), timeCommRank);

  Engines::ODC::ODC_Parameters odcparams;
  odcparams.queryFile                    = writerParams.queryFileName;
  odcparams.mappingFile                  = writerParams.mappingFileName;
  odcparams.outputFile                   = outputFileName;
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

  // Merge files if writeMultipleFiles == false. The merge is done separately for
  // each time rank, so each time rank gets its own merged file.
  if (!dataOutParams.writeMultipleFiles.value()
       && ioPoolComm.rank() == 0 && multiRankPool) {
    const std::size_t mpiRankSize     = ioPoolComm.size();
    const std::string mergedFileName =
      ioda::Engines::uniquifyFileName(configFileName, false, 0, timeCommRank);
    std::ofstream outFile(mergedFileName,
                          std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    for (std::size_t irank = 0; irank < mpiRankSize; irank++) {
      const std::string inFileName
        = Engines::uniquifyFileName(configFileName, true, irank, timeCommRank);
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
