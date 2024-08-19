/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#pragma once

#include <variant>
#include <string>
#include <vector>

#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/ReaderFactory.h"
#include "ioda/Engines/Script.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// ReadScriptFile
//----------------------------------------------------------------------------------------

// Parameters

class ReadScriptFileParameters : public ReaderParametersBase {
  OOPS_CONCRETE_PARAMETERS(ReadScriptFileParameters, ReaderParametersBase)

public:
  /// \brief Path to input file
  oops::RequiredParameter<std::map<std::string, std::string>> args{"args", this};

  /// \brief Path to odc query specs
  oops::RequiredParameter<std::string> scriptFile{"script file", this};

  bool isFileBackend() const override { return false; }

  std::string getFileName() const override { return std::string(""); }
};

// Classes

class ReadScriptFile: public ReaderBase {
public:
  typedef ReadScriptFileParameters Parameters_;

  // Constructor via parameters
  ReadScriptFile(const Parameters_& params, const ReaderCreationParameters & createParams);

  void print(std::ostream & os) const final;

  std::string fileName() const final;

private:
  std::string fileName_;

  Script::Script_Parameters::ArgType convertArg(const std::string& argValue) const;
};

}  // namespace Engines
}  // namespace ioda
