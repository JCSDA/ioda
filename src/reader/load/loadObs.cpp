/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/load/loadObs.hpp"

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"
#include "eckit/io/Buffer.h"

#include "ioda/containers/IFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/reader/load/loadObsFromGenerators.hpp"
#include "ioda/reader/load/loadObsFromNetcdf.hpp"
#include "ioda/reader/load/loadObsFromOdb.hpp"

#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace reader {

//--------------------------------------------------------------------------------
// Declarations for private functions
//--------------------------------------------------------------------------------

/// \brief distribute osdf related metadata to ranks that do not have them (e.g. those outside the
/// io pool)
///
/// \details This function needs to be called on all ranks in the mainComm
///          communicator. One rank that already has the metadata will be selected to
///          be the source for copying osdf related metadata to all of the ranks
///          that do not have them. There are two sets of metadata involved:
///            1. the osdf ColumnMetadata (special row in the data frame object)
///            2. the osdf FrameMetadata (stand alone object that the obs space stores)
/// \param mainComm eckit MPI communicator group for all ranks
/// \param thisRankHasMetadata true if this rank already has the correct metadata, false otherwise
/// \param destOsdf destination OSDF container object
/// \param osdfMetadata frame metadata for dest OSDF
///
/// \note If the metadata are not available on any rank and thus there is nothing to distribute,
/// this function will return early.
static void distributeOsdfMetadata(const eckit::mpi::Comm & mainComm, bool thisRankHasMetadata,
                                   std::unique_ptr<osdf::IFrame> & destOsdf,
                                   osdf::FrameMetadata & osdfMetadata);

//--------------------------------------------------------------------------------
// Public functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void loadObs(const ObsDataInParameters & dataInParams,
             const IoPool::IoPoolParameters & ioPoolParams,
             const eckit::mpi::Comm & commAll,
             const std::vector<std::string> & obsVarNames,
             const util::TimeWindow & timeWindow,
             std::unique_ptr<osdf::IFrame> & destOsdf,
             osdf::FrameMetadata & osdfMetadata) {
  oops::Log::trace() << "reader::loadObs start" << std::endl;
  // todo(SRH): for now only supporting load from a netcdf file, an ODB file, or a generator
  // (GenList, GenRandom). Will want to eventually support BUFR files too.
  const std::string inputFileType =
    dataInParams.engine.value().engineParameters.value().type.value();
  if (inputFileType != "H5File" && inputFileType != "ODB" &&
      inputFileType != "GenList" && inputFileType != "GenRandom") {
    const std::string errMsg = std::string("Unsupported input file type: ")
    + inputFileType + std::string(". Must use 'H5File', 'ODB', 'GenList' or 'GenRandom' for now.");
    throw eckit::BadParameter(errMsg, Here());
  }

  std::unique_ptr<ObsIoPool::ObsIoPool> obsIoPool =
    std::make_unique<ObsIoPool::ObsIoPool>(ioPoolParams, commAll);
  bool thisRankHasMetadata = false;

  if (inputFileType == "H5File") {
    // Reading a netcdf/hdf5 file.
    // Collectively call the loadOsdfFromNetcdf function with all io pool members.
    if (obsIoPool->inIoPool()) {
      loadOsdfFromNetcdf(dataInParams, obsIoPool->commPool(), destOsdf, osdfMetadata);
    }

    // The netcdf/hdf5 file reader will generate column metadata for all ranks
    // in the pool when using a single input file. However, when reading a set
    // of input files (one per rank in the io pool) it is possible for one or
    // more of those files to have zero locations and currently an empty file
    // will result in an OSDF without column metadata. To cover this case,
    // check if the destOsdf has any columns: if numCols > 0, then it has
    // metadata, if 0 then it does not have metadata.
    thisRankHasMetadata = (destOsdf->numCols() > 0);
  } else if (inputFileType == "ODB") {
    // Reading an ODB file.
    // Collectively call the loadOsdfFromOdb function with all io pool members.
    if (obsIoPool->inIoPool()) {
      loadOsdfFromOdb(dataInParams, obsIoPool->commPool(), destOsdf, osdfMetadata);
    }
    // The ODB reader generates column metadata only on io pool ranks that get to read at least one
    // location.
    const bool thisRankHasLocations = destOsdf->numRows() > 0;
    thisRankHasMetadata = thisRankHasLocations && obsIoPool->inIoPool();
  } else {
    // Generating data (GenList or GenRandom).
    // Generate the full set of locations on the lead io pool rank only, so that the rows are
    // non-overlapping across ranks (as the later distribute step assumes). The metadata
    // distribution below copies the column definitions to the other ranks, and the distribute
    // step spreads the rows.
    if (obsIoPool->inIoPool() && obsIoPool->commPool().rank() == 0) {
      if (inputFileType == "GenList") {
        loadOsdfFromGenList(dataInParams, obsVarNames, destOsdf, osdfMetadata);
      } else {
        ASSERT(inputFileType == "GenRandom");
        loadOsdfFromGenRandom(dataInParams, obsVarNames, timeWindow, destOsdf, osdfMetadata);
      }
    }
    thisRankHasMetadata = (destOsdf->numCols() > 0);
  }

  // Distribute the column metadata (definitions) from the first io pool rank that has them (if
  // there is one) to all ranks that do not have them yet, so that all ranks have consistent column
  // definitions.
  distributeOsdfMetadata(obsIoPool->commAll(), thisRankHasMetadata, destOsdf, osdfMetadata);

  if (obsIoPool->inIoPool()) {
    int anyRankHasLocations = destOsdf->numRows() > 0;
    obsIoPool->commPool().reduceInPlace(anyRankHasLocations, eckit::mpi::Operation::Code::MAX, 0);

    if (obsIoPool->commPool().rank() == 0 && !anyRankHasLocations) {
      oops::Log::warning() << "WARNING: Input file "
                           << dataInParams.engine.value().engineParameters.value().getFileName()
                           << " contains zero observations" << std::endl;
    }
  }
  oops::Log::trace() << "reader::loadObs end" << std::endl;
}

//--------------------------------------------------------------------------------
// Private functions
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
void distributeOsdfMetadata(const eckit::mpi::Comm & mainComm, bool thisRankHasMetadata,
                            std::unique_ptr<osdf::IFrame> & destOsdf,
                            osdf::FrameMetadata & osdfMetadata) {
  // Find the rank that will send the serialized column metadata
  // This will be the lowest rank in the main comm that already has these metadata.
  // Note that converting thisRankHasMetadata to an integer value (1 == true, 0 == false)
  // facilitates the MPI transfers (vector of bool implementation is platform
  // dependent).
  std::vector<int> rankHasMetadata(1, (thisRankHasMetadata ? 1 : 0));
  oops::mpi::allGatherv(mainComm, rankHasMetadata);
  const int rootRank = std::distance(rankHasMetadata.begin(),
                                     std::find(rankHasMetadata.begin(), rankHasMetadata.end(), 1));
  if (rootRank >= static_cast<int>(rankHasMetadata.size()))
    return;  // No rank has metadata, so there's nothing to distribute.

  // Send the serialized column metadata from the rootRank to ranks that do not have the metadata.
  // During the send/recv sequence, the tag values are:
  //  0 - send/receive the size of the serialized metadata
  //  1 - send/receive the serialized metadata
  const int myRank = mainComm.rank();
  const int mySize = mainComm.size();
  if (mySize > 1) {
    if (myRank == rootRank) {
      eckit::Buffer columnMetadataBufr(destOsdf->getColumnMetadataBufferSize());
      const std::size_t colMetadataSize = destOsdf->serializeColumnMetadata(columnMetadataBufr);

      eckit::Buffer frameMetadataBufr(osdfMetadata.bufrSize());
      const std::size_t frameMetadataSize = osdfMetadata.serialize(frameMetadataBufr);
      for (std::size_t i = 0; i < mainComm.size(); ++i) {
        if (rankHasMetadata[i] == 0) {
          // Send the osdf column metadata to ranks that do not have them
          mainComm.send(colMetadataSize, i, 0);
          mainComm.send(
            static_cast<const char*>(columnMetadataBufr.data()), colMetadataSize, i, 1);

          mainComm.send(frameMetadataSize, i, 2);
          mainComm.send(
            static_cast<const char*>(frameMetadataBufr.data()), frameMetadataSize, i, 3);
        }
      }
    } else {
      // Remaining ranks, some of which need -- and will receive -- the metadata.
      if (!thisRankHasMetadata) {
        // Receive the column metadata and update the destOsdf container
        std::size_t colMetadataSize;
        mainComm.receive(colMetadataSize, rootRank, 0);
        eckit::Buffer columnMetadataBuffer(colMetadataSize);
        mainComm.receive(
          static_cast<char *>(columnMetadataBuffer.data()), colMetadataSize, rootRank, 1);

        destOsdf->deserializeColumnMetadata(columnMetadataBuffer);

        // Receive the frame metadata and update the osdfMetadata object
        std::size_t frameMetadataSize;
        mainComm.receive(frameMetadataSize, rootRank, 2);
        eckit::Buffer frameMetadataBuffer(frameMetadataSize);
        mainComm.receive(
          static_cast<char *>(frameMetadataBuffer.data()), frameMetadataSize, rootRank, 3);

        osdfMetadata.deserialize(frameMetadataBuffer);
      }
    }
    mainComm.barrier();
  }
}

}  // namespace reader
}  // namespace ioda
