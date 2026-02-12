/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/load/loadObs.hpp"

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "ioda/containers/IFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/reader/load/loadObsFromNetcdf.hpp"

#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace reader {

//--------------------------------------------------------------------------------
// Declarations for private functions
//--------------------------------------------------------------------------------

/// \brief distribute osdf related metadata from an io pool rank to all ranks not in the io pool
/// \details This function needs to be called on all ranks in the mainComm
///          communicator. One rank from the io pool will be selected to
///          be the source for copying osdf related metadata to all of the ranks
///          that are not in the io pool. There are two sets of metadata involved:
///            1. the osdf ColumnMetadata (special row in the data frame object)
///            2. the osdf FrameMetadata (stand alone object that the obs space stores)
/// \param mainComm eckit MPI communicator group for all ranks
/// \param inIoPool flag indicating if this rank is in the io pool (true) or not (false)
/// \param destOsdf destination OSDF container object
/// \param osdfMetadata frame metadata for dest OSDF
static void distributeOsdfMetadata(const eckit::mpi::Comm & mainComm, bool inIoPool,
                                   std::unique_ptr<osdf::IFrame> & destOsdf,
                                   osdf::FrameMetadata & osdfMetadata);

//--------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void loadObs(const ObsDataInParameters & dataInParams,
             const IoPool::IoPoolParameters & ioPoolParams,
             const eckit::mpi::Comm & commAll,
             std::unique_ptr<osdf::IFrame> & destOsdf,
             osdf::FrameMetadata & osdfMetadata) {
  oops::Log::trace() << "reader::loadObs start" << std::endl;
  // todo(SRH): for now only supporting load from a netcdf file.
  // will want to eventually support ODB and BUFR files.
  const std::string inputFileType =
    dataInParams.engine.value().engineParameters.value().type.value();

  if (inputFileType == "H5File") {
    // Reading a netcdf/hdf5 file.
    // Create an IoPool object, and pass the pool communicator to the
    // loadOsdfFromNetcdf function.
    std::unique_ptr<ObsIoPool::ObsIoPool> obsIoPool =
      std::make_unique<ObsIoPool::ObsIoPool>(ioPoolParams, commAll);

    // Collectively call the loadOsdfFromNetcdf function with all io pool members.
    if (obsIoPool->inIoPool()) {
      loadOsdfFromNetcdf(dataInParams, obsIoPool->commPool(), destOsdf, osdfMetadata);
    }

    // Distribute the column metadata (definitions) from an io pool rank to all
    // non-io pool ranks so that all ranks have consistent column definitions.
    distributeOsdfMetadata(obsIoPool->commAll(), obsIoPool->inIoPool(),
                           destOsdf, osdfMetadata);
  } else {
    const std::string errMsg = std::string("Unsupported input file type: ")
      + inputFileType + std::string(". Must use 'H5File' for now.");
    throw eckit::BadParameter(errMsg, Here());
  }
  oops::Log::trace() << "reader::loadObs end" << std::endl;
}

//--------------------------------------------------------------------------------
// Private functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void distributeOsdfMetadata(const eckit::mpi::Comm & mainComm, bool inIoPool,
                            std::unique_ptr<osdf::IFrame> & destOsdf,
                            osdf::FrameMetadata & osdfMetadata) {
  // Find the rank in the io pool that will send the serialized column metadata
  // This will be the lowest rank in the main comm that is also in the io pool.
  // Note that converting inIoPool to an integer value (1 == true, 0 == false)
  // facilitates the MPI transfers (vector of bool implementation is platform
  // dependent).
  std::vector<int> ioPoolMembers(1, (inIoPool ? 1 : 0));
  oops::mpi::allGatherv(mainComm, ioPoolMembers);
  const int rootRank = std::distance(ioPoolMembers.begin(),
                                     std::find(ioPoolMembers.begin(), ioPoolMembers.end(), 1));

  // Send the serialized column metadata from the rootRank to all non-io pool ranks
  // During the send/recv sequence, the tag values are:
  //  0 - send/receive the size of the serialized metadata
  //  1 - send/receive the serialized metadata
  const int myRank = mainComm.rank();
  const int mySize = mainComm.size();
  if (mySize > 1) {
    if (myRank == rootRank) {
      const std::string serializedColMetadata = destOsdf->serializeColumnMetadata();
      const int metadataSize = serializedColMetadata.size();
      for (std::size_t i = 0; i < mainComm.size(); ++i) {
        if (ioPoolMembers[i] == 0) {
          // Send the osdf column metadata to the non-io pool ranks
          mainComm.send(metadataSize, i, 0);
          mainComm.send(serializedColMetadata.data(), metadataSize, i, 1);

          // Need to synchronize the osdfMetadata object:
          //   frameType
          //   chanNums
          //   numVars
          //   dateTimeEpoch
          //   varsWithChans

          // frameType
          oops::mpi::sendString(mainComm, osdfMetadata.getFrameType(), i);

          // chanNums
          int sendInt = osdfMetadata.getChanNums().size();
          mainComm.send(sendInt, i, 2);
          mainComm.send(osdfMetadata.getChanNums().data(), sendInt, i , 3);

          // numVars
          sendInt = osdfMetadata.getNumVars();
          mainComm.send(sendInt, i, 4);

          // dateTimeEpoch
          oops::mpi::sendString(mainComm, osdfMetadata.getDateTimeEpoch(), i);

          // varsWithChans
          sendInt = osdfMetadata.getVarsWithChans().size();
          mainComm.send(sendInt, i, 5);
          for (const auto & varName : osdfMetadata.getVarsWithChans()) {
            oops::mpi::sendString(mainComm, varName, i);
          }
        }
      }
    } else {
      // Remaining ranks, some will be in the pool, but all of the non-pool ranks
      // will be here. Only the non-pool ranks will receive the metadata.
      if (!inIoPool) {
        // Receive the column metadata and update the destOsdf container
        int metadataSize;
        mainComm.receive(metadataSize, rootRank, 0);
        std::vector<char> serializedColMetadata(metadataSize);
        mainComm.receive(serializedColMetadata.data(), metadataSize, rootRank, 1);
        // Now deserialize the metadata into the destOsdf container
        destOsdf->deserializeColumnMetadata(std::string(serializedColMetadata.data(),
                                                        serializedColMetadata.size()));

        // Need to read and store the osdfMetada data members:
        //   frameType
        //   chanNums
        //   numVars
        //   dateTimeEpoch
        //   varsWithChans

        // frameType
        std::string recvString;
        oops::mpi::receiveString(mainComm, recvString, rootRank);
        osdfMetadata.setFrameType(recvString);

        // chanNums
        int recvInt;
        mainComm.receive(recvInt, rootRank, 2);
        std::vector<int> chanNums(recvInt);
        mainComm.receive(chanNums.data(), recvInt, rootRank, 3);
        osdfMetadata.setChanNums(chanNums);

        // numVars
        mainComm.receive(recvInt, rootRank, 4);
        osdfMetadata.setNumVars(recvInt);

        // dateTimeEpoch
        oops::mpi::receiveString(mainComm, recvString, rootRank);
        osdfMetadata.setDateTimeEpoch(recvString);

        // varsWithChans
        mainComm.receive(recvInt, rootRank, 5);
        for (int i = 0; i < recvInt; ++i) {
          std::string varName;
          oops::mpi::receiveString(mainComm, varName, rootRank);
          osdfMetadata.addVarToVarsWithChans(varName);
        }
      }
    }
    mainComm.barrier();
  }
}

}  // namespace reader
}  // namespace ioda
