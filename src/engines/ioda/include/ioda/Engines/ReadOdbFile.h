#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/ReaderFactory.h"
#include "ioda/Engines/ODC.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// ReadOdbFile
//----------------------------------------------------------------------------------------

// Parameters

class ReadOdbFileParameters : public ReaderParametersBase {
    OOPS_CONCRETE_PARAMETERS(ReadOdbFileParameters, ReaderParametersBase)

  public:
    /// \brief Path to input file
    oops::Parameter<std::string> fileName{"obsfile", "", this};

    /// \brief Paths to multiple input file
    oops::Parameter<std::vector<std::string>> fileNames{"obsfiles", { }, this};


    /// \brief Path to varno mapping file
    oops::RequiredParameter<std::string> mappingFileName{"mapping file", this};

    /// \brief Path to odc query specs
    oops::RequiredParameter<std::string> queryFileName{"query file", this};

    /// Maximum number of channels (levels) allowed in any profile. Used to even
    /// out profiles which contain a varying number of levels.
    /// Optional: defaults to zero.
    oops::Parameter<int> maxNumberChannels{"max number channels", 0, this};

    /// \brief Extended lower bound of time window (datetime in ISO-8601 format).
    oops::OptionalParameter<util::DateTime>
      timeWindowExtendedLowerBound{"time window extended lower bound", this};

    /// \brief action to take if input file is missing
    /// \details the error action is the default which will write an error message
    /// and throw an exception stopping the execution.
    oops::Parameter<std::string> missingFileAction{"missing file action", "error", this};

    bool isFileBackend() const override { return true; }
};

// Classes

class ReadOdbFile: public ReaderBase {
 public:
  typedef ReadOdbFileParameters Parameters_;

  // Constructor via parameters
  ReadOdbFile(const Parameters_ & params, const ReaderCreationParameters & createParams);

  std::string fileName() const override;
  void print(std::ostream & os) const override;

 private:
  std::string fileName_;
};

}  // namespace Engines
}  // namespace ioda
