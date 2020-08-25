/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_IODAIOFACTORY_H_
#define IO_IODAIOFACTORY_H_

#include <string>

#include "oops/util/DateTime.h"

#include "ioda/io/IodaIO.h"

namespace ioda {

/*! \brief Constant to be used for default maximum frame size */
const std::size_t IODAIO_DEFAULT_FRAME_SIZE = 10000;

/*!
 * \brief Factory class to instantiate objects of IodaIO subclasses.
 *
 * \details This class provides a Create method to open a file in a particular mode.
 *
 *          Currently, the subclass from which to instantiate an object from is chosen
 *          based on the suffix in the file name. ".nc4" and ".nc" are recognized as
 *          netcdf files, and .odb is recognized as an ODB2 file. This isn't necessarily
 *          the best way to identify the file format, so this should be revisited in
 *          the future.
 *
 * \author Stephen Herbener (JCSDA)
 */

class IodaIOfactory {
 public:
  IodaIOfactory() { }
  ~IodaIOfactory() { }

  // Factory methods
  static ioda::IodaIO* Create(const std::string & FileName, const std::string & FileMode,
                              const std::size_t MaxFrameSize);
};

}  // namespace ioda

#endif  // IO_IODAIOFACTORY_H_
