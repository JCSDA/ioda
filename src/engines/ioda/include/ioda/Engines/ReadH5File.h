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

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// ReadH5File
//----------------------------------------------------------------------------------------

// Parameters

class ReadH5FileParameters : public ReaderParametersBase {
    OOPS_CONCRETE_PARAMETERS(ReadH5FileParameters, ReaderParametersBase)

  public:
    /// \brief Path to input file
    oops::Parameter<std::string> fileName{"obsfile", "", this};

    /// \brief Paths to multiple input file
    oops::Parameter<std::vector<std::string>> fileNames{"obsfiles", { }, this};

    /// \brief action to take if input file is missing
    /// \details the warning action is the default which will write a warning message
    /// and continue with a representation of an empty file.
    oops::Parameter<std::string> missingFileAction{"missing file action", "warn", this};

    bool isFileBackend() const override { return true; }
};

// Classes

class ReadH5File: public ReaderBase {
 public:
  typedef ReadH5FileParameters Parameters_;

  // Constructor via parameters
  ReadH5File(const Parameters_ & params, const ReaderCreationParameters & createParams);

  std::string fileName() const override;
  void print(std::ostream & os) const override;

 private:
  std::string fileName_;
};

}  // namespace Engines
}  // namespace ioda
