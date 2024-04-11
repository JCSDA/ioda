/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

/*! \defgroup ioda_cxx_io ReaderPrepInputFiles
 * \brief Public API for ioda::ReaderPrepInputFiles
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file ReaderPrepInputFiles.h
 * \brief Interfaces for ioda::ReaderPrepInputFiles and related classes.
 */

#include "ioda/defs.h"
#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/ReaderFactory.h"
#include "ioda/Group.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/ReaderPoolBase.h"

namespace ioda {
namespace IoPool {

class Distribution;
enum class DateTimeFormat;

/// \brief Reader prep input files
/// \details This class is not a full reader, rather it is intended to be used by the
/// standalone app that prepares the input file set. It will define empty load and finalize
/// functions to satisfy the base class requirements, but the app will only use the
/// initialize function.
/// \ingroup ioda_cxx_io
class ReaderPrepInputFiles : public ReaderPoolBase {
 public:
  /// \brief construct a ReaderPrepInputFiles object
  /// \param configParams Parameters for this io pool
  /// \param createParams Parameters for creating the reader pool
  ReaderPrepInputFiles(const IoPoolParameters & configParams,
                       const ReaderPoolCreationParameters & createParams);
  ~ReaderPrepInputFiles() {}

  /// \brief initialize the io pool after construction
  /// \detail This routine is here to do specialized initialization before the load
  /// function has been called and after the constructor is called.
  void initialize() override;

  /// \brief load obs data from the obs source (file or generator)
  /// \param destGroup destination ioda group to be loaded from the input file
  void load(Group & destGroup) override;

  /// \brief finalize the io pool before destruction
  /// \detail This routine is here to do specialized clean up after the load function has been
  /// called and before the destructor is called. The primary task is to clean up the eckit
  /// split communicator groups.
  void finalize() override;

  /// \brief fill in print routine for the util::Printable base class
  void print(std::ostream & os) const override;

 private:
};

}  // namespace IoPool
}  // namespace ioda

/// @}
