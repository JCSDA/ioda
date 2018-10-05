/*
 * (C) Copyright 2017-2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */


#include "eckit/config/Configuration.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Logger.h"
#include "oops/util/DateTime.h"
#include "ioda/ObsSpaceFactory.h"

namespace ioda {

// -----------------------------------------------------------------------------

ObsSpaceFactory::ObsSpaceFactory(const std::string & name) {
  if (getMakers().find(name) != getMakers().end()) {
    oops::Log::error() << name << " already registered in ufo::ObsSpaceFactory." << std::endl;
    ABORT("Element already registered in ufo::ObsSpaceFactory.");
  }
  getMakers()[name] = this;
}

// -----------------------------------------------------------------------------

oops::ObsSpaceBase * ObsSpaceFactory::create(const eckit::Configuration & conf,
                        const util::DateTime & bgn, const util::DateTime & end ) {
  oops::Log::trace() << "ObsSpaceBase::create starting" << std::endl;
  const std::string id = conf.getString("ObsType");
  typename std::map<std::string, ObsSpaceFactory*>::iterator jloc = getMakers().find(id);
  if (jloc == getMakers().end()) {
    oops::Log::error() << id << " does not exist in ufo::ObsSpaceFactory." << std::endl;
    ABORT("Element does not exist in ufo::ObsSpaceFactory.");
  }
  oops::ObsSpaceBase * ptr = jloc->second->make(conf, bgn, end);
  oops::Log::trace() << "ObsSpaceBase::create done" << std::endl;
  return ptr;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
