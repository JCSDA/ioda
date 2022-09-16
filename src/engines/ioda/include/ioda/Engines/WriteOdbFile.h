#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>

#include "ioda/Engines/WriterBase.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// WriteOdbFile
//----------------------------------------------------------------------------------------

// Parameters

class WriteOdbFileParameters : public WriterParametersBase {
    OOPS_CONCRETE_PARAMETERS(WriteOdbFileParameters, WriterParametersBase)

  public:
    /// \brief Allow an existing file to be overwritten
    oops::Parameter<bool> allowOverwrite{"allow overwrite", true, this};

    /// \brief Path to input file
    oops::RequiredParameter<std::string> fileName{"obsfile", this};

    /// \brief Path to varno mapping file
    oops::RequiredParameter<std::string> mappingFileName{"mapping file", this};
};

// Classes

class WriteOdbFile : public WriterBase {
 public:
  typedef WriteOdbFileParameters Parameters_;

  // Constructor via parameters
  WriteOdbFile(const Parameters_ & params, const util::DateTime & winStart,
               const util::DateTime & winEnd, const eckit::mpi::Comm & comm,
               const eckit::mpi::Comm & timeComm, const std::vector<std::string> & obsVarNames);

  void print(std::ostream & os) const override;

 private:
  std::string fileName_;
};

}  // namespace Engines
}  // namespace ioda
