/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <memory>

// Forward class declarations
namespace osdf {
    class IFrame;
}
namespace eckit {
    namespace mpi {
        class Comm;
    }
}
namespace ioda {
    class ObsDataInParameters;
    namespace IoPool {
        class IoPoolParameters;
    }
}

namespace ioda {
namespace reader {

/// \brief parallel load across all ranks in the io pool
/// \param dataInParams parameters for obsdatain configuration
/// \param ioPoolComm eckit MPI communicator group for the io pool
/// \param destOSDF destination OSDF container
void loadOsdfFromNetcdf(const ObsDataInParameters & dataInParams,
                        const eckit::mpi::Comm & ioPoolComm,
                        std::unique_ptr<osdf::IFrame> & destOSDF);

/// \brief distribute column metadata from an io pool rank to all ranks not in the io pool
/// \details This function needs to be called on all ranks in the mainComm
///          communicator. One rank from the io pool will be selected to
///          serialize its column metadata, and then send that serialized
///          data to all the ranks not in the io pool. The non-io pool
///          ranks will then use that serialized data to configure their
///          columns to match those on the io pool ranks.
/// \param mainComm eckit MPI communicator group for all ranks
/// \param inIoPool integer flag indicating if this rank is in the io pool (1) or not (0)
/// \param destOSDF destination OSDF container
void distributeOsdfColumnMetadata(const eckit::mpi::Comm & mainComm, int inIoPool,
                                  std::unique_ptr<osdf::IFrame> & destOSDF);

}  // namespace reader
}  // namespace ioda
