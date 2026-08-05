/*
* (C) Copyright 2024 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

/*! \addtogroup ioda_cxx_engines_pub_Bufr
 *
 * @{
 * \file Bufr.cpp
 * \brief Bufr engine bindings
 */

#include <ostream>

#include "ioda/Engines/Bufr.h"
#include "ioda/Group.h"
#include "ioda/config.h"  // Auto-generated. Defines *_FOUND.
#include "ioda/ObsGroup.h"
#include "oops/util/Logger.h"

# include "eckit/config/YAMLConfiguration.h"
# include "eckit/filesystem/PathName.h"

#if bufr_query_FOUND
#include "bufr/DataCache.h"
#include "bufr/DataContainer.h"
#include "bufr/BufrParser.h"

#include "ioda/Engines/Bufr/Encoder.h"
#else
#endif


namespace ioda {
namespace Engines {
namespace Bufr {

#if bufr_query_FOUND
#else
/// @brief Standard message when the BUFR API is unavailable.
const char bufrMissingMessage[] {
  "The Bufr engine is disabled."};
#endif

ObsGroup openFile(const Bufr_Parameters& bufrParams,
                  const ioda::Engines::ReaderCreationParameters& readerParams,
                  Group emptyStorageGroup)
{
#if bufr_query_FOUND
  oops::Log::debug() << "BUFR called with " << bufrParams.mappingFile << std::endl;

  if (bufrParams.mappingFile.find(".yaml") == std::string::npos)
  {
    throw eckit::Exception("Unknown file type for BUFR mapping file.", Here());
  }

  eckit::YAMLConfiguration yaml(eckit::PathName(bufrParams.mappingFile));

  if (!yaml.has("bufr"))
  {
    throw eckit::Exception("No section named \"bufr\"", Here());
  }

  if (!yaml.has("encoder"))
  {
    throw eckit::Exception("No section named \"encoder\"", Here());
  }

  std::shared_ptr<bufr::DataContainer> data;

  if (bufr::DataCache::has(bufrParams.filename, bufrParams.mappingFile) &&
      !bufrParams.cacheCategories.empty())
  {
    if (bufrParams.category.empty())
    {
      throw eckit::Exception("Must provide category if BUFR file is split.", Here());
    }

    oops::Log::debug() << "Using cached data for " << bufrParams.filename << std::endl;

    data = bufr::DataCache::get(bufrParams.filename, bufrParams.mappingFile);
  }
  else
  {
    auto bufrParser = bufr::BufrParser(bufrParams.filename,
                                       yaml.getSubConfiguration("bufr"),
                                       bufrParams.tablePath);

    oops::Log::info() << "Parsing BUFR file: " << bufrParams.filename << std::endl;

    if (readerParams.comm.size() > 0)
    {
      oops::Log::info() << "Parsing in parallel" << std::endl;
      data = bufrParser.parse(readerParams.comm);

      // time the time it takes to run allGather
      data->allGather(readerParams.comm);
    }
    else
    {
      oops::Log::info() << "Parsing in serial" << std::endl;
      data = bufrParser.parse();
    }

    if (!bufrParams.cacheCategories.empty())
    {
      bufr::DataCache::add(bufrParams.filename,
                           bufrParams.mappingFile,
                           bufrParams.cacheCategories,
                           data);
    }
  }

  auto dataMap = Encoder(yaml.getSubConfiguration("encoder")).\
                  encode(data->getSubContainer(bufrParams.category));

  ObsGroup result;
  if (!bufrParams.category.empty())
  {
    if (dataMap.find(bufrParams.category) == dataMap.end())
    {
      std::stringstream errStr;
      errStr << "Category (";
      for (const auto& cat : bufrParams.category)
      {
        errStr << cat;

        if (cat != bufrParams.category.back())
        {
          errStr << ", ";
        }
      }

      errStr << ") was not read by BufrParser.";
      throw eckit::Exception(errStr.str(), Here());
    }
    else
    {
      result = dataMap[bufrParams.category];
    }
  }
  else
  {
    if (dataMap.size() > 1)
    {
      throw eckit::Exception("Must provide category if BUFR file is split.", Here());
    }

    result = dataMap.begin()->second;
  }

  if (!bufrParams.cacheCategories.empty())
  {
    bufr::DataCache::markFinished(bufrParams.filename, bufrParams.mappingFile, bufrParams.category);
  }

  return result;
#else
  throw eckit::Exception(bufrMissingMessage, Here());
#endif
}
}  // namespace Bufr
}  // namespace Engines
}  // namespace ioda
