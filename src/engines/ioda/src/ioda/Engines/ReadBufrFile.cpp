/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#include <boost/none.hpp>
#include <boost/optional.hpp>

#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

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

    obs_group_ = Engines::Bufr::openFile(bufrparams, backend);

    oops::Log::trace() << "ioda::Engines::ReadBufrFile end constructor" << std::endl;
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
