/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_FILEFORMAT_H_
#define CORE_FILEFORMAT_H_

#include <string>

namespace ioda {

/// \brief Observation file format.
///
/// \internal If you add a new format, be sure to modify FileFormatParameterTraitsHelper in
/// ParameterTraitsFileFormat.h accordingly.
enum class FileFormat {
  /// File format determined automatically from the file name extension (`.odb` -- ODB, everything
  /// else -- HDF5).
  AUTO,
  /// HDF5 file format
  HDF5,
  /// ODB file format
  ODB
};

/// \brief Determine the format of an observation file.
///
/// Returns `hint` unless it's set to FileFormat::AUTO, in which case the function returns
/// FileFormat::ODB if `filePath` ends with `.odb` (irrespective of case) and FileFormat::HDF5
/// otherwise.
FileFormat determineFileFormat(const std::string &filePath, FileFormat hint);

}  // namespace ioda

#endif  // CORE_FILEFORMAT_H_
