/*
 * * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "eckit/utils/StringTools.h"
#include "ioda/core/FileFormat.h"

namespace ioda {

FileFormat determineFileFormat(const std::string &filePath, FileFormat hint) {
  if (hint != FileFormat::AUTO)
    return hint;
  if (eckit::StringTools::endsWith(eckit::StringTools::lower(filePath), ".odb"))
    return FileFormat::ODB;
  return FileFormat::HDF5;
}

}  // namespace ioda
