/*
* (C) Copyright 2024 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#pragma once

#include <string>
#include <vector>

#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/ReaderFactory.h"
#include "ioda/Engines/Bufr.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// ReadOdbFile
//----------------------------------------------------------------------------------------

// Parameters

class ReadBufrFileParameters : public ReaderParametersBase {
  OOPS_CONCRETE_PARAMETERS(ReadBufrFileParameters, ReaderParametersBase)

public:
  /// \brief Path to input file
  oops::Parameter<std::string> fileName{"obsfile", "", this};

  /// \brief Paths to multiple input file
  oops::Parameter<std::vector<std::string>> fileNames{"obsfiles", { }, this};

  /// \brief Path to BUFR query specs
  oops::RequiredParameter<std::string> mappingFile{"mapping file", this};

  /// \brief Path to BUFR table files used to decode WMO files
  oops::OptionalParameter<std::string> tablePath{"table path", this};

  /// \brief Category to read from the BUFR file
  oops::OptionalParameter<std::vector<std::string>> category{"category", this};

  /// \brief Categories to cache to DataCache tracks to figure out when to dump the cache
  oops::OptionalParameter<std::vector<std::vector<std::string>>>
    cacheCategories{"cache categories", this};

  bool isFileBackend() const override { return true; }

  std::string getFileName() const override { return fileName.value(); }
};

// Classes

class ReadBufrFile: public ReaderBase {
public:
  typedef ReadBufrFileParameters Parameters_;

  // Constructor via parameters
  ReadBufrFile(const Parameters_ & params, const ReaderCreationParameters & createParams);

  void print(std::ostream & os) const final;

  std::string fileName() const final;

private:
  std::string fileName_;
};

}  // namespace Engines
}  // namespace ioda
