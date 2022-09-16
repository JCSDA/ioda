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
// WriteH5File
//----------------------------------------------------------------------------------------

// Parameters

class WriteH5FileParameters : public WriterParametersBase {
    OOPS_CONCRETE_PARAMETERS(WriteH5FileParameters, WriterParametersBase)

  public:
    /// \brief Allow an existing file to be overwritten
    oops::Parameter<bool> allowOverwrite{"allow overwrite", true, this};

    /// \brief Path to input file
    oops::RequiredParameter<std::string> fileName{"obsfile", this};
};

// Classes

class WriteH5File : public WriterBase {
 public:
  typedef WriteH5FileParameters Parameters_;

  // Constructor via parameters
  WriteH5File(const Parameters_ & params, const util::DateTime & winStart,
              const util::DateTime & winEnd, const eckit::mpi::Comm & comm,
              const eckit::mpi::Comm & timeComm, const std::vector<std::string> & obsVarNames);

  void print(std::ostream & os) const override;

 private:
  // output file name
  std::string fileName_;
};

}  // namespace Engines
}  // namespace ioda
