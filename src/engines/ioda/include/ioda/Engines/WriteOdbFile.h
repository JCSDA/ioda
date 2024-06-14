#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>

#include "ioda/Engines/WriterBase.h"
#include "ioda/Engines/WriterFactory.h"
#include "ioda/Engines/ODC.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// WriteOdbFile
//----------------------------------------------------------------------------------------

// Parameters

class WriteOdbFileParameters : public WriterParametersBase {
    OOPS_CONCRETE_PARAMETERS(WriteOdbFileParameters, WriterParametersBase)

  public:
    /// \brief Path to varno mapping file
    oops::RequiredParameter<std::string> mappingFileName{"mapping file", this};

    /// \brief Path to varno mapping file
    oops::RequiredParameter<std::string> queryFileName{"query file", this};

    /// \brief Abort if a value is missing in the ObsSpace that is in the mapping map
    oops::Parameter<bool> missingObsSpaceVariableAbort{
        "abort when variable missing", false, this};

    /// \brief Ignore the ObsSpace Channel dimension when writing the output file
    oops::Parameter<bool> ignoreChannelDimensionWrite{
        "ignore channel dimension write", false, this};

    /// \brief Type of ODB
    oops::Parameter<std::string> odbType{"odb type", "", this};
};

// Classes

class WriteOdbFile : public WriterBase {
 public:
  typedef WriteOdbFileParameters Parameters_;

  // Constructor via parameters
  WriteOdbFile(const Parameters_ & params, const WriterCreationParameters & createParams);

  void print(std::ostream & os) const override;

  void finalize() override;

 private:
  Parameters_ params_;
};

//----------------------------------------------------------------------------------------
// WriteOdbProc
//----------------------------------------------------------------------------------------

class WriteOdbProc : public WriterProcBase {
 public:
  typedef WriteOdbFileParameters Parameters_;

  // Constructor via parameters
  WriteOdbProc(const Parameters_ & params, const WriterCreationParameters & createParams);

  /// \brief post processor to be run after the WriteOdbFile object is destructed
  /// \detail This post processor is associated with WriteH5File, but placed in a
  /// separate class structure so that the hdf5 file can be closed before running this
  /// post processor.
  void post() override;

  /// \brief Flag to check whether the backend can use parallel io.
  bool backendCanUseParallelIO() const override;

  void print(std::ostream & os) const override;

 private:
  Parameters_ params_;
};

}  // namespace Engines
}  // namespace ioda
