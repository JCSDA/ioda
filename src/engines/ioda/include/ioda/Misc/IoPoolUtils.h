#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_io IoPool
 * \brief Public API for ioda::IoPool
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file IoPoolUtils.h
 * \brief Utility functions for ioda::IoPool and related classes.
 */

#include <string>

#include "ioda/defs.h"
namespace ioda {

  /// \brief uniquify the output file name
  /// \details This function will tag on the MPI task number to the end of the file name
  /// to avoid collisions when running with multiple MPI tasks.
  /// \param fileName raw output file name
  /// \param rankNum MPI group communicator rank number
  /// \param timeRankNum MPI time communicator rank number
  std::string uniquifyFileName(const std::string & fileName, std::size_t rankNum,
                               int timeRankNum);

}  // namespace ioda

/// @}
