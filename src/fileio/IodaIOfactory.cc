/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <string>

#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"

#include "fileio/IodaIOfactory.h"
#include "fileio/NetcdfIO.h"

namespace ioda {

//-------------------------------------------------------------------------------------
/*!
 * \brief Instantiate a IodaIO object
 *
 * \param[in] FileName Path to the obs file
 * \param[in] FileMode Mode in which to open the obs file, "r" for read, "w" for overwrite
 *            and existing file and "W" for create and write to a new file
 * \param[in] MaxFrameSize  Maximum number of "rows" in a frame
 */

IodaIO* IodaIOfactory::Create(const std::string & FileName, const std::string & FileMode,
                              const std::size_t MaxFrameSize) {
  std::size_t Spos;
  std::string FileSuffix;

  // Form the suffix by chopping off the string after the last "." in the file name.
  Spos = FileName.find_last_of(".");
  if (Spos == FileName.npos) {
    FileSuffix = "";
  } else {
    FileSuffix = FileName.substr(Spos+1);
  }

  // Create the appropriate object depending on the file suffix
  if ((FileSuffix == "nc4") || (FileSuffix == "nc")) {
    return new ioda::NetcdfIO(FileName, FileMode, MaxFrameSize);
  } else {
    oops::Log::error() << "ioda::IodaIO::Create: Unrecognized file suffix: "
                       << FileName << std::endl;
    oops::Log::error() << "ioda::IodaIO::Create:   suffix must be one of: .nc4, .nc"
                       << std::endl;
    ABORT("ioda::Ioda::Create: Unrecognized file suffix");
    return NULL;
  }
}

}  // namespace ioda
