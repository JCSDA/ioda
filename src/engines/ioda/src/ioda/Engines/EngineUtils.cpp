/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/EngineUtils.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include "ioda/defs.h"
#include "ioda/Engines/HH.h"
#include "ioda/Engines/ODC.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/missingValues.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------
// Engine Utilities functions
//----------------------------------------------------------------------

// -----------------------------------------------------------------------------
std::string uniquifyFileName(const std::string & fileName, const bool createMultipleFiles,
                             const std::size_t rankNum, const int timeRankNum) {
    // The format for the output file name is:
    //
    //        fileName<rankNum><timeRankNum>
    //
    // For <rankNum>:
    //    If createMultipleFiles is true, then the string appended is "_0000" for rank 0,
    //    "_0001" for rank 1, etc.
    //    If createMultipleFiles is false, then no string is appended.
    //
    // For <timeRankNum>:
    //    If timeRankNum is >= zero, then the string appended is "_0000" for time rank 0,
    //    "_0001" for time rank 1, etc.
    //    If timeRankNum is < zero, then no string is appended.

    // Attach the rank number to the output file name to avoid collisions when running
    // with multiple MPI tasks.
    std::string uniqueFileName = fileName;

    // Find the right-most dot in the file name, and use that to pick off the file name
    // and file extension.
    std::size_t found = uniqueFileName.find_last_of(".");
    if (found == std::string::npos) found = uniqueFileName.length();

    // optionally include the rankNum and/or timeRankNum suffixes
    std::ostringstream ss;
    ss << "";
    if (createMultipleFiles) {
        ss << "_" << std::setw(4) << std::setfill('0') << rankNum;
    }
    if (timeRankNum >= 0) {
        // reset the width and fill character just in case they were changed above
        ss << std::setw(0) << std::setfill(' ');
        ss << "_" << std::setw(4) << std::setfill('0') << timeRankNum;
    }

    // Construct the output file name
    return uniqueFileName.insert(found, ss.str());
}

// -----------------------------------------------------------------------------
void storeGenData(const std::vector<float> & latVals,
                  const std::vector<float> & lonVals,
                  const std::vector<int64_t> & dts,
                  const std::string & epoch,
                  const std::vector<std::string> & obsVarNames,
                  const std::vector<float> & obsValues,
                  const std::vector<float> & obsErrors,
                  ObsGroup &obsGroup) {
    // Generated data is a set of vectors for now.
    //     MetaData group
    //        latitude
    //        longitude
    //        datetime
    //
    //     ObsError group
    //        list of simulated variables in obsVarNames

    Variable LocationVar = obsGroup.vars["Location"];

    const float missingFloat = util::missingValue(missingFloat);
    const std::string missingString("missing");
    const int64_t missingInt64 = util::missingValue(missingInt64);

    ioda::VariableCreationParameters float_params;
    float_params.chunk = true;
    float_params.compressWithGZIP();
    float_params.setFillValue<float>(missingFloat);

    ioda::VariableCreationParameters int64_params;
    int64_params.chunk = true;
    int64_params.compressWithGZIP();
    int64_params.setFillValue<int64_t>(missingInt64);

    std::string latName("MetaData/latitude");
    std::string lonName("MetaData/longitude");
    std::string dtName("MetaData/dateTime");

    // Create, write and attach units attributes to the variables
    obsGroup.vars.createWithScales<float>(latName, { LocationVar }, float_params)
        .write<float>(latVals)
        .atts.add<std::string>("units", std::string("degrees_east"));
    obsGroup.vars.createWithScales<float>(lonName, { LocationVar }, float_params)
        .write<float>(lonVals)
        .atts.add<std::string>("units", std::string("degrees_north"));
    obsGroup.vars.createWithScales<int64_t>(dtName, { LocationVar }, int64_params)
        .write<int64_t>(dts)
        .atts.add<std::string>("units", epoch);

    for (std::size_t i = 0; i < obsVarNames.size(); ++i) {
        const std::string varErrName = std::string("ObsError/") + obsVarNames[i];
        std::vector<float> obsErrVals(latVals.size(), obsErrors[i]);
        obsGroup.vars.createWithScales<float>(varErrName, { LocationVar }, float_params)
            .write<float>(obsErrVals);
        if (!obsValues.empty()){
            const std::string varValName = std::string("ObsValue/") + obsVarNames[i];
            std::vector<float> obsVals(latVals.size(), obsValues[i]);
            obsGroup.vars.createWithScales<float>(varValName, {LocationVar}, float_params)
                    .write<float>(obsVals);
        }
    }
}

//----------------------------------------------------------------------
Group constructFromCmdLine(int argc, char** argv, const std::string& defaultFilename) {
  /* Options:
  Start with --ioda-engine-options. All options after this are
  positional options.

  --ioda-engine-options name parameters...

  Currently supported engine names:
  1. HDF5 with file backend [HDF5-file]
  2. HDF5 in-memory backend [HDF5-mem]
3. ObsStore in-memory backend [obs-store]

  Engine parameters:
  1. Needs a file name, file opening properties [create, open],
     access mode [read, read_write, create, truncate].
  2. Needs a file name, file size increment length (in MB), flush_on_close (bool).
  3. None
  */
  BackendCreationParameters params;
  BackendNames backendName;

  using std::string;
  std::vector<string> opts(argc);
  for (size_t i = 0; i < (size_t)argc; ++i) opts[i] = string(argv[i]);

  // Find the "--ioda-engine-options" option.
  // If none is specified, use a default.
  auto it = std::find(opts.cbegin(), opts.cend(), "--ioda-engine-options");
  if (it == opts.cend()) {
    // No options --> create a file using the defaultFileName
    backendName       = BackendNames::Hdf5File;
    params.fileName   = defaultFilename;
    params.action     = BackendFileActions::Create;
    params.createMode = BackendCreateModes::Truncate_If_Exists;
  } else {
    ++it;
    if (it == opts.cend()) {
      throw Exception("Bad option --ioda-engine-options. Got the "
        "--ioda-engine-options token but nothing else.", ioda_Here());
    }

    // convert array of c-string options into vector of strings
    auto readOpts = [&opts, &it](size_t n) -> std::vector<string> {
      std::vector<string> res(n);
      for (size_t i = 0; i < n; ++i) {
        ++it;
        if (it == opts.cend())
          throw Exception("Bad option --ioda-engine-options. "
            "Wrong number of elements.", ioda_Here()).add("Expected", n);
        res[i] = std::string(*it);
      }
      return res;
    };

    string sEngine = *it;
    if (sEngine == "HDF5-file") {
      auto engineOpts = readOpts(3);
      backendName     = BackendNames::Hdf5File;
      params.fileName = engineOpts[0];

      enum class open_or_create {
        open,
        create
      } action
        = (engineOpts[1] == "create") ? open_or_create::create : open_or_create::open;
      if (action == open_or_create::open) {
        // open action
        params.action   = BackendFileActions::Open;
        params.openMode = (engineOpts[2] == "read_write") ? BackendOpenModes::Read_Write
                                                          : BackendOpenModes::Read_Only;
      } else {
        // create action
        params.action     = BackendFileActions::Create;
        params.createMode = (engineOpts[2] == "truncate") ? BackendCreateModes::Truncate_If_Exists
                                                          : BackendCreateModes::Fail_If_Exists;
      }
    } else if (sEngine == "HDF5-mem") {
      auto engineOpts = readOpts(3);
      // TODO(ryan): Allow open / create
      // enum class open_or_create { open, create } action
      //	= (engineOpts[1] == "create") ? open_or_create::create :
      //	open_or_create::open;

      backendName       = BackendNames::Hdf5Mem;
      params.fileName   = engineOpts[0];
      params.action     = BackendFileActions::Create;
      params.createMode = BackendCreateModes::Truncate_If_Exists;

      string sAllocLen_MB = engineOpts[1];
      string sFlush       = engineOpts[2];

      params.allocBytes = gsl::narrow<size_t>(((size_t)std::stoul(sAllocLen_MB)) * 1024 * 1024);
      params.flush      = (sFlush == "true");
    } else if (sEngine == "obs-store") {
      backendName = BackendNames::ObsStore;
    } else {
      throw Exception("Bad option --ioda-engine-options. "
              "Unknown engine.", ioda_Here()).add("Engine", sEngine);
    }
  }
  return constructBackend(backendName, params);
}

Group constructBackend(BackendNames name, BackendCreationParameters& params) {
  Group backend;
  if (name == BackendNames::Hdf5File) {
    if (params.action == BackendFileActions::Open) {
      return HH::openFile(params.fileName, params.openMode);
    }
    if (params.action == BackendFileActions::Create) {
      return HH::createFile(params.fileName, params.createMode,
                 HH::HDF5_Version_Range(HH::HDF5_Version::V18, HH::HDF5_Version::V110));
    }
    if (params.action == BackendFileActions::CreateParallel) {
      return HH::createParallelFile(params.fileName, params.createMode, params.comm,
                 HH::HDF5_Version_Range(HH::HDF5_Version::V18, HH::HDF5_Version::V110));
    }
    throw Exception("Unknown BackendFileActions value", ioda_Here());
  }
  if (name == BackendNames::Hdf5Mem) {
    if (params.action == BackendFileActions::Open) {
      return HH::openMemoryFile(params.fileName, params.openMode, params.flush,
                                params.allocBytes);
    }
    if (params.action == BackendFileActions::Create) {
      return HH::createMemoryFile(params.fileName, params.createMode, params.flush,
                                  params.allocBytes);
    }
    throw Exception("Unknown BackendFileActions value", ioda_Here());
  }
  if (name == BackendNames::ObsStore) return ObsStore::createRootGroup();
  if (name == BackendNames::ODB) {
    Engines::ODC::ODC_Parameters odcparams;
    // TODO(srh, djd) We need to get rid of this hard coded path/filename in the long term. 
    // In fact, this entire function here needs to be refactored into the Engines reader
    // and writer backend class structure (see code in src/engines/ioda/src/ioda/Engines
    // for details). Doing that refactoring will enable the replacement of this hard
    // coded path with a YAML configuration.
    odcparams.mappingFile = "testinput/odb_default_name_map.yml";
    if (params.action == BackendFileActions::Create) {
      return ODC::createFile(odcparams);
    }
    throw Exception("Unknown BackendFileActions value", ioda_Here());
  }

  // If we get to here, then we have a backend name that is
  // not implemented yet.
  throw Exception("Backend not implemented yet", ioda_Here());
}

std::ostream& operator<<(std::ostream& os, const ioda::Engines::BackendCreateModes& mode)
{
  using namespace ioda::Engines;
  static const std::map<BackendCreateModes, std::string> names {
    {BackendCreateModes::Undefined, "Undefined"},
    {BackendCreateModes::Truncate_If_Exists, "Truncate_If_Exists"},
    {BackendCreateModes::Fail_If_Exists, "Fail_If_Exists"}
  };

  if (names.count(mode) == 0) throw Exception("Unhandled backend creation mode", ioda_Here());
  os << "ioda::Engines::BackendCreateModes::" << names.at(mode);
  return os;
}

std::ostream& operator<<(std::ostream& os, const ioda::Engines::BackendOpenModes& mode)
{
  using namespace ioda::Engines;
  static const std::map<BackendOpenModes, std::string> names {
    {BackendOpenModes::Undefined, "Undefined"},
    {BackendOpenModes::Read_Only, "Read_Only"},
    {BackendOpenModes::Read_Write, "Read_Write"}
  };
  if (names.count(mode) == 0) throw Exception("Unhandled backend open mode", ioda_Here());
  os << "ioda::Engines::BackendOpenModes::" << names.at(mode);
  return os;
}

}  // namespace Engines
}  // namespace ioda

