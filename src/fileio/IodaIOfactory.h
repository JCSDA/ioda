/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef FILEIO_IODAIOFACTORY_H_
#define FILEIO_IODAIOFACTORY_H_

#include <string>

#include "fileio/IodaIO.h"
#include "oops/util/DateTime.h"

namespace ioda {

/*!
 * \brief Factory class to instantiate objects of IodaIO subclasses.
 *
 * \details This class provides two Create methods. The Create method with two arguements
 *          Is intended for use when opening a file in read mode. The Create method with
 *          six arguments is intended for use when opening a file in write mode.
 *
 *          Currently, the subclass from which to instantiate an object from is chosen
 *          based on the suffix in the file name. ".nc4" and ".nc" are recognized as
 *          netcdf files, and .odb is recognized as an ODB file. This isn't necessarily
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
                              const util::DateTime & bgn, const util::DateTime & end,
                              const eckit::mpi::Comm & comm);
  static ioda::IodaIO* Create(const std::string & FileName, const std::string & FileMode,
                              const util::DateTime & bgn, const util::DateTime & end,
                              const eckit::mpi::Comm & comm,
                              const std::size_t & Nlocs, const std::size_t & Nrecs,
                              const std::size_t & Nvars);
};

}  // namespace ioda

#endif  // FILEIO_IODAIOFACTORY_H_
