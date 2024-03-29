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

class IoPoolParameters : public oops::Parameters {
     OOPS_CONCRETE_PARAMETERS(IoPoolParameters, oops::Parameters)

 public:
    /// maximum pool size in number of MPI processes
    oops::Parameter<int> maxPoolSize{"max pool size", -1, this};

    /// chunk size in bytes
    oops::OptionalParameter<std::size_t> chunkSize{"chunk size", this};

    /// chunk cache size in bytes
    oops::OptionalParameter<std::size_t> chunkCacheSize{"chunk cache size", this};

    /// maximum file size in megabytes
    oops::OptionalParameter<std::size_t> maxFileSize{"max file size", this};

    /// write multiple files (write one file per io pool task)
    /// default is false meaning a single output file will be written
    oops::Parameter<bool> writeMultipleFiles{"write multiple files", false, this};
};

}  // namespace ioda

/// @}
