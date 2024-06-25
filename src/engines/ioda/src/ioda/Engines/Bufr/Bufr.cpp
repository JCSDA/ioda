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

#include "eckit/io/MemoryHandle.h"
#include "ioda/Engines/Bufr.h"
#include "ioda/Exception.h"
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

ObsGroup openFile(const Bufr_Parameters& bufrParams, Group emptyStorageGroup)
{
#if bufr_query_FOUND
  oops::Log::debug() << "BUFR called with " << bufrParams.mappingFile << std::endl;

  if (bufrParams.mappingFile.find(".yaml") == std::string::npos)
  {
    throw Exception("Unknown file type for BUFR mapping file.", ioda_Here());
  }

  eckit::YAMLConfiguration yaml(eckit::PathName(bufrParams.mappingFile));

  if (!yaml.has("bufr"))
  {
    throw Exception("No section named \"bufr\"", ioda_Here());
  }

  if (!yaml.has("encoder"))
  {
    throw Exception("No section named \"encoder\"", ioda_Here());
  }

  std::shared_ptr<bufr::DataContainer> data;

  if (bufr::DataCache::has(bufrParams.filename, bufrParams.mappingFile) &&
      !bufrParams.cacheCategories.empty())
  {
    if (bufrParams.category.empty())
    {
      throw Exception("Must provide category if BUFR file is split.", ioda_Here());
    }

    oops::Log::debug() << "Using cached data for " << bufrParams.filename << std::endl;

    data = bufr::DataCache::get(bufrParams.filename, bufrParams.mappingFile);
  }
  else
  {
    data = bufr::BufrParser(bufrParams.filename,
                            yaml.getSubConfiguration("bufr"),
                            bufrParams.tablePath).parse();

    if (!bufrParams.cacheCategories.empty())
    {
      bufr::DataCache::add(bufrParams.filename,
                           bufrParams.mappingFile,
                           bufrParams.cacheCategories,
                           data);
    }
  }

  auto dataMap = Encoder(yaml.getSubConfiguration("encoder")).encode(data);

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
      throw Exception(errStr.str(), ioda_Here());
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
      throw Exception("Must provide category if BUFR file is split.", ioda_Here());
    }

    result = dataMap.begin()->second;
  }

  if (!bufrParams.cacheCategories.empty())
  {
    bufr::DataCache::markFinished(bufrParams.filename, bufrParams.mappingFile, bufrParams.category);
  }

  return result;
#else
  throw Exception(bufrMissingMessage, ioda_Here());
#endif
}
}  // namespace Bufr
}  // namespace Engines
}  // namespace ioda