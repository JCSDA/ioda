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

namespace ioda {
namespace reader {

/// \brief load specified range of locations from the input file to the OSDF container
/// \details This function is not MPI aware, rather it is the low level function that handles
/// loading data from an input hdf5 file on a single process. It takes a range of locations
/// which will allow it to be run on multiple processes where the range of locations point
/// to different sections of the data for different ranks.
/// Note that the specification of the location range is given as a start location number
/// and a count. This lines up with the manner in which netcdf/hdf5 hyperslab block
/// specification is made. The idea is to give the first N locations to rank 0, then the
/// second N locations to rank 1, and so forth in a load balanced manner.
/// \param fileName path to input netcdf file
/// \param startLoc beginning of location range
/// \param locCount number of locations in range
/// \param destOSDF destination OSDF container object
int loadObsContainerFromNetcdf(const std::string & fileName,
                               const std::size_t startLoc,
                               const std::size_t locCount,
                               std::unique_ptr<osdf::IFrame> & destOSDF);

}  // namespace reader
}  // namespace ioda
