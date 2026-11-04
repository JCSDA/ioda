#pragma once
/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file OsdfSchemaCheck.h
/// \brief Check that an IFrame (OSDF) container has the same schema on every MPI task

#include <string>

#include "eckit/mpi/Comm.h"

namespace osdf {
  class IFrame;
}

namespace ioda {

/// @brief Check that the IFrame schema is identical on all MPI tasks in a communicator
/// @detail IODA requires that every MPI task holds the same columns, in the same order and with
/// the same data types, regardless of how many rows (locations) each task holds. This is a
/// collective call and must be made by every task in comm. Every task throws if the schemas differ.
/// @param[in] frame   IFrame object whose schema is checked
/// @param[in] comm    Communicator across which the schema must be identical
/// @param[in] context Description of the call site, included in the error message
void osdfCheckSchemaAcrossRanks(const osdf::IFrame & frame,
                                const eckit::mpi::Comm & comm,
                                const std::string & context);

}  // namespace ioda
