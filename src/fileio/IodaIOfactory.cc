/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include <string>

#include "oops/util/Logger.h"

#include "fileio/IodaIOfactory.h"
#include "fileio/NetcdfIO.h"

namespace ioda {

//-------------------------------------------------------------------------------------
/*!
 * \brief Instantiate a IodaIO object in read mode
 *
 * \details This method will instantiate an object of a IodaIO subclass. This method is
 *          intended to be used when opening a file in a read mode. The Nlocs, Nobs, Nrecs
 *          and Nvars parameters are set to zero which is okay since these will be set
 *          by reading metadata from the input file.
 */

IodaIO* IodaIOfactory::Create(const std::string & FileName, const std::string & FileMode) {
  return Create(FileName, FileMode, 0, 0, 0, 0);
}

//-------------------------------------------------------------------------------------
/*!
 * \brief Instantiate a IodaIO object in write mode
 *
 * \details This method will instantiate an object of a IodaIO subclass. This method is
 *          intended to be used when opening a file in a write mode. The Nlocs, Nobs, Nrecs
 *          and Nvars parameters are set by the caller in this case. These parameters will
 *          subsequently be used to set metadata in the output file.
 */

IodaIO* IodaIOfactory::Create(const std::string & FileName, const std::string & FileMode,
                              const std::size_t & Nlocs, const std::size_t & Nobs,
                              const std::size_t & Nrecs, const std::size_t & Nvars) {
  std::size_t Spos;
  std::string FileSuffix;

  // Form the suffix by chopping off the string after the last "." in the file name.
  Spos = FileName.find_last_of(".");
  if (Spos == FileName.npos) {
    FileSuffix = "";
    }
  else {
    FileSuffix = FileName.substr(Spos+1);
    }

  // Create the appropriate object depending on the file suffix
  if ((FileSuffix == "nc4") || (FileSuffix == "nc")) {
    return new ioda::NetcdfIO(FileName, FileMode, Nlocs, Nobs, Nrecs, Nvars);
    }
  //else if (FileSuffix == "odb") {
  //  return new ioda::OdbIO(FileName, FileMode);
  //  }
  else {
    oops::Log::error() << "ioda::IodaIO::Create: Unrecognized file suffix: " << FileName << std::endl;
    oops::Log::error() << "ioda::IodaIO::Create:   suffix must be one of: .nc4, .nc" << std::endl;
    return NULL;
    }
}

}  // namespace ioda
