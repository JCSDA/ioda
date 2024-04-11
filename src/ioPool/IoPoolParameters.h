#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_io IoPoolParameters
 * \brief Public API for ioda::IoPoolParameters
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file IoPoolParameters.h
 * \brief Parameter classes for ioda::IoPool and related classes.
 */

#include <string>
#include <vector>

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"


namespace eckit {
  class Configuration;
}

namespace ioda {
namespace IoPool {

class IoPoolPrepFileParameters : public oops::Parameters {
     OOPS_CONCRETE_PARAMETERS(IoPoolPrepFileParameters, oops::Parameters)

 public:
    /// size of MPI communicator that will be used in the DA run
    oops::RequiredParameter<int> mpiCommSize{"mpi communicator size", this};

    /// output path and base name for the file set produced by the standalone app
    oops::RequiredParameter<std::string> outputFile{"output file", this};
};

class IoPoolParameters : public oops::Parameters {
     OOPS_CONCRETE_PARAMETERS(IoPoolParameters, oops::Parameters)

 public:
    /// maximum pool size in number of MPI processes
    oops::Parameter<int> maxPoolSize{"max pool size", -1, this};

    /// maximum file size in megabytes
    oops::OptionalParameter<std::size_t> maxFileSize{"max file size", this};

    /// write multiple files (write one file per io pool task)
    /// default is false meaning a single output file will be written
    oops::Parameter<bool> writeMultipleFiles{"write multiple files", false, this};

    /// Select the reader pool
    /// Two options for now:
    ///    SinglePoolAllTasks is for the current reader where every MPI task joins the
    ///                       the io pool (essentially no pool)
    ///    SinglePool is for the case of using the reader pool in a scheme where only one
    ///               pool is used at a time (as opposed to creating multiple pools to read
    //                in multple obs spaces in parallel)
    oops::Parameter<std::string> readerPoolName{"reader name", "SinglePoolAllTasks", this};

    /// Select the writer pool
    /// For now we have only one option:
    ///    SinglePool is for the case of using the writer pool in a scheme where only one
    ///               pool is used at a time (as opposed to creating multiple pools to write
    //                out multple obs spaces in parallel)
    oops::Parameter<std::string> writerPoolName{"writer name", "SinglePool", this};

    /// specify a working directory, default (workDir == empty string)
    /// is to use the directory holding the input file. Note that workDir will end
    /// up holding subdirectories named after the input files and it is these subdirectories
    /// that will be removed by default (or kept if "keep work directory contents"
    /// is set to true)
    oops::Parameter<std::string> workDir{"work directory", "", this};

    // For the standalone file preparation applications
    oops::OptionalParameter<IoPoolPrepFileParameters>
        prepFileParameters{"file preparation", this};
};

}  // namespace IoPool
}  // namespace ioda

/// @}
