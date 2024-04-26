/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

#include "ioda/Engines/ReadScriptFile.h"

namespace ioda {
namespace Engines {
  //---------------------------------------------------------------------
  // ReadScriptFile
  //---------------------------------------------------------------------

  static ReaderMaker<ReadScriptFile> maker("script");

  Script::Script_Parameters::ArgType ReadScriptFile::convertArg(const std::string& argValue) const
  {
    try
    {
      return Script::Script_Parameters::ArgType(std::stod(argValue));
    }
    catch (...) {}

    try
    {
      return Script::Script_Parameters::ArgType(std::stoi(argValue));
    }
    catch (...) {}

    return Script::Script_Parameters::ArgType(argValue);
  }

  ReadScriptFile::ReadScriptFile(const Parameters_ & params,
                                 const ReaderCreationParameters & createParams)
      : ReaderBase(createParams), fileName_(params.scriptFile)
  {
    oops::Log::trace() << "ioda::Engines::ReadScriptFile start constructor" << std::endl;

    // Create an in-memory backend
    Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    Engines::BackendCreationParameters backendParams;
    Group backend = constructBackend(backendName, backendParams);

    // Load the BUFR file into the backend
    Engines::Script::Script_Parameters scriptParams;
    scriptParams.scriptFile = params.scriptFile;

    for (auto arg : params.args.value())
    {
      scriptParams.args[arg.first] = convertArg(arg.second);
    }

    obs_group_ = Engines::Script::openFile(scriptParams, backend);

    oops::Log::trace() << "ioda::Engines::ReadScriptFile end constructor" << std::endl;
  }

  void ReadScriptFile::print(std::ostream & os) const
  {
    os << fileName_;
  }

  std::string ReadScriptFile::fileName() const
  {
    return fileName_;
  }
}  // namespace Engines
}  // namespace ioda
