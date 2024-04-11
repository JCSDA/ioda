/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/EngineUtils.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "ioda/defs.h"
#include "ioda/Engines/HH.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsGroup.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------
// Engine Utilities functions
//----------------------------------------------------------------------

// -----------------------------------------------------------------------------
std::string formFileSuffixFromRankNums(const bool createMultipleFiles,
                                       const std::size_t rankNum, const int timeRankNum) {
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
    return ss.str();
}

//------------------------------------------------------------------------------------
std::string formFileWithPath(const std::string & newDirectory, const std::string & fileName) {
    // Get "basename" of the path in inputFileName
    std::string newFileName;
    const std::size_t pos = fileName.find_last_of('/');
    if (pos == std::string::npos) {
        // fileName is just a file name without a directory path, use as is
        newFileName = fileName;
    } else {
        // fileName contains path information, strip off the directory path part
        newFileName = fileName.substr(pos + 1);
    }
    newFileName = newDirectory + std::string("/") + newFileName;
    return newFileName;
}

//------------------------------------------------------------------------------------
std::string formFileWithNewExtension(const std::string & fileName,
                                     const std::string & newExtension) {
    std::string newFileName;
    const std::size_t pos = fileName.find_last_of('.');
    if (pos == std::string::npos) {
        // No file extension, simply add the file extension
        newFileName = fileName + newExtension;
    } else {
        // Replace the extension with the new file extension
        newFileName = fileName.substr(0, pos) + newExtension;
    }
    return newFileName;
}

//------------------------------------------------------------------------------------
std::string formFileWithSuffix(const std::string & fileName, const std::string & fileSuffix) {
    std::string newFileName = fileName;
    const std::size_t pos = newFileName.find_last_of('.');
    if (pos == std::string::npos) {
        // No file extension, simply add the file suffix
        newFileName += fileSuffix;
    } else {
        // Insert the file suffix right before the extension
        newFileName.insert(pos, fileSuffix);
    }
    return newFileName;
}

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

    // Form the file suffix out of the rank numbers
    const std::string fileSuffix =
        formFileSuffixFromRankNums(createMultipleFiles, rankNum, timeRankNum);

    // Construct the output file name
    return uniqueFileName.insert(found, fileSuffix);
}

// -----------------------------------------------------------------------------
void storeGenData(const std::vector<float> & latVals,
                  const std::vector<float> & lonVals,
                  const std::string & vcoordType,
                  const std::vector<float> & vcoordVals,
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
    //        vertical coordinate type
    //        vertical coordinate
    //        datetime
    //
    //     ObsError group
    //        list of simulated variables in obsVarNames
    //
    // Valid values for vcoordType are "pressure" or "height"

    Variable LocationVar = obsGroup.vars["Location"];

    const float missingFloat = util::missingValue<float>();
    const int64_t missingInt64 = util::missingValue<int64_t>();

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
    std::string pressureName("MetaData/pressure");
    std::string heightName("MetaData/height");
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
    if ( vcoordType == "pressure" ) {
        obsGroup.vars.createWithScales<float>(pressureName, { LocationVar }, float_params)
            .write<float>(vcoordVals)
            .atts.add<std::string>("units", std::string("Pa"));
    } else if ( vcoordType == "height" ) {
	obsGroup.vars.createWithScales<float>(heightName, { LocationVar }, float_params)
	    .write<float>(vcoordVals)
            .atts.add<std::string>("units", std::string("m"));
    }

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

eckit::LocalConfiguration constructFileBackendConfig(const std::string & fileType,
                const std::string & fileName, const std::string & mapFileName,
                const std::string & queryFileName, const std::string & odbType) {
    // For now, include just enough for file io:
    //    HDF5 read or write:
    //       engine:
    //           type: H5File
    //           obsfile: <path-to-file>
    //
    //    ODB read or write:
    //       engine:
    //           type: ODB
    //           obsfile: <path-to-file>
    //           mapping file: <path-to-file>
    //           query file: <path-to-file>
    //
    // There are more controls available in the engine configurations, and these can be added
    // later on an as needed basis.
    eckit::LocalConfiguration engineConfig;
    if (fileType == "hdf5") {
        engineConfig.set("engine.type", "H5File");
        engineConfig.set("engine.obsfile", fileName);
    } else if (fileType == "odb") {
        engineConfig.set("engine.type", "ODB");
        engineConfig.set("engine.obsfile", fileName);
        engineConfig.set("engine.mapping file", mapFileName);
        engineConfig.set("engine.query file", queryFileName);
    } else {
        throw Exception("Unknown file type: " + fileType, ioda_Here());
    }

    return engineConfig;
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
  if (name == BackendNames::ObsStore) {
    return ObsStore::createRootGroup();
  }

  // If we get to here, then we have a backend name that is
  // not implemented yet.
  throw Exception("Backend not implemented yet", ioda_Here());
}

bool haveFileReadAccess(const std::string & fileName) {
  // access function returns true if file has read access (R_OK)
  // Want to allow the caller to be able to handle error (access returns non-zero).

  // First make sure fileName is a path to a file. If so then check for read access.
  struct stat sbuf;
  const int status = stat(fileName.c_str(), &sbuf);
  bool haveReadAccess = false;
  if (status == 0) {
      // fileName exists
      if (S_ISREG(sbuf.st_mode)) {
          // fileName is a regular file
          // following is true if fileName has read access (R_OK)
          haveReadAccess = (access(fileName.c_str(), R_OK) == 0);
      }
  }
  return haveReadAccess;
}

bool haveDirRwxAccess(const std::string & dirName) {
  // access function returns true if directory has read write and execute access
  // Want to allow the caller to be able to handle error (access returns non-zero).

  // First make sure dirName is a path to a directory. If so then check for read, write
  // and execute access.
  struct stat sbuf;
  const int status = stat(dirName.c_str(), &sbuf);
  bool haveRwxAccess = false;
  if (status == 0) {
      // dirName exists
      if (S_ISDIR(sbuf.st_mode)) {
          // dirName is a directory
          // following is true if dirName has read (R_OK), write (W_OK) and
          // execute (X_OK) access
          haveRwxAccess = (access(dirName.c_str(), R_OK | W_OK | X_OK) == 0);
      }
  }
  return haveRwxAccess;
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

