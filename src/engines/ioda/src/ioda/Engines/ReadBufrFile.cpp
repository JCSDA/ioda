/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#include <boost/none.hpp>
#include <boost/optional.hpp>

#include "oops/util/Logger.h"

#include "ioda/Engines/ReadBufrFile.h"

namespace ioda {
namespace Engines {
  //---------------------------------------------------------------------
  // ReadBufrFile
  //---------------------------------------------------------------------

  static ReaderMaker<ReadBufrFile> maker("bufr");

  ReadBufrFile::ReadBufrFile(const Parameters_ & params,
                             const ReaderCreationParameters & createParams)
      : ReaderBase(createParams), fileName_(params.fileName)
  {
    oops::Log::trace() << "ioda::Engines::ReadBufrFile start constructor" << std::endl;
    if (haveFileReadAccess(fileName_)) {
    	// Create an in-memory backend
    	Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
    	Engines::BackendCreationParameters backendParams;
    	Group backend = constructBackend(backendName, backendParams);

    	// Load the BUFR file into the backend
    	Engines::Bufr::Bufr_Parameters bufrparams;
    	bufrparams.filename = params.fileName;
    	bufrparams.mappingFile = params.mappingFile;

    	if (params.tablePath.value())
    	{
      	  bufrparams.tablePath = params.tablePath.value().get();
    	}

    	if (params.category.value())
    	{
      	  bufrparams.category = params.category.value().get();
    	}

    	if (params.cacheCategories.value())
    	{
      	  bufrparams.cacheCategories = params.cacheCategories.value().get();
    	}

        obs_group_ = Engines::Bufr::openFile(bufrparams, createParams, backend);

    	oops::Log::trace() << "ioda::Engines::ReadBufrFile end constructor" << std::endl;
  } else{
        // Input file does not exist (is not readable actually)
        if (params.missingFileAction.value() == "warn") {
            oops::Log::info() << "WARNING: input file is not readable, "
               << "will continue with empty file representation" << std::endl
               << "WARNING:     file: " << fileName_ << std::endl;
            // Create a memory backend as a placehold for the missing file. Make the
            // memory backend look like an empty file (Location == 0).
            Engines::BackendNames backendName = Engines::BackendNames::ObsStore;
            Engines::BackendCreationParameters backendParams;
            Group backend = constructBackend(backendName, backendParams);

            // Create the ObsGroup and attach the backend.
    	    // Load the BUFR file into the backend
    	    Engines::Bufr::Bufr_Parameters bufrparams;
    	    bufrparams.filename = params.fileName;
     	    bufrparams.mappingFile = params.mappingFile;

            obs_group_ = ObsGroup::generate(backend, {});

            // Create the Location dimension and set its size to zero.
            obs_group_.vars.create<int64_t>("Location", { 0 })
                .setIsDimensionScale("Location");
        } else if (params.missingFileAction.value() == "error") {
            std::string ErrMsg = std::string("Input file is not readable, ") +
                std::string("will stop execution. File: ") + fileName_ + std::string("\n");
            throw eckit::Exception(ErrMsg, Here());
        } else {
            std::string ErrMsg = std::string("Unrecognized input file missing action: ") +
                params.missingFileAction.value();
            throw eckit::Exception(ErrMsg, Here());
        }
  
    }
  }
  void ReadBufrFile::print(std::ostream & os) const
  {
    os << fileName_;
  }

  std::string ReadBufrFile::fileName() const
  {
    return fileName_;
  }
}  // namespace Engines
}  // namespace ioda
