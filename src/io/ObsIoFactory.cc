/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/io/ObsIo.h"
#include "ioda/io/ObsIoFactory.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

namespace ioda {

ObsIoFactory::ObsIoFactory(const std::string & name) {
  if (getMakers().find(name) != getMakers().end()) {
    throw std::runtime_error(name + " already registered in the ObsIo factory.");
  }
  getMakers()[name] = this;
}

std::shared_ptr<ObsIo> ObsIoFactory::create(ObsIoModes mode,
                                            const ObsSpaceParameters & parameters) {
  oops::Log::trace() << "ObsIoFactory::create starting" << std::endl;

  std::string name;
  const ObsIoParametersBase *ioParameters = nullptr;

  switch (mode) {
  case ObsIoModes::READ:
    name = parameters.top_level_.obsIoInParameters().type.value().value();
    ioParameters = &parameters.top_level_.obsIoInParameters();
    break;
  case ObsIoModes::WRITE:
    if (parameters.top_level_.obsOutFile.value() == boost::none)
      throw eckit::BadValue("Cannot create output file: the 'obsdataout' option has not been set",
                            Here());
    name = "FileCreate";
    ioParameters = &parameters.top_level_.obsOutFile.value().value();
    break;
  default:
    throw eckit::BadValue("Unknown mode", Here());
    break;
  }

  std::shared_ptr<ObsIo> ptr = getMaker(name).make(*ioParameters, parameters);
  oops::Log::trace() << "ObsIoFactory::create done" << std::endl;
  return ptr;
}

std::unique_ptr<ObsIoParametersBase> ObsIoFactory::createParameters(const std::string &name) {
  return getMaker(name).makeParameters();
}

ObsIoFactory& ObsIoFactory::getMaker(const std::string &name) {
  typename std::map<std::string, ObsIoFactory*>::iterator jloc = getMakers().find(name);

  if (jloc == getMakers().end()) {
    std::string makerNameList;
    for (const auto& makerDetails : getMakers()) makerNameList += "\n  " + makerDetails.first;
    throw eckit::BadParameter(name + " does not exist in ioda::ObsIoFactory. "
                                "Possible values:" + makerNameList, Here());
  }

  return *jloc->second;
}

std::vector<std::string> ObsIoFactory::getMakerNames() {
  return oops::keys(getMakers());
}

std::map <std::string, ObsIoFactory*> & ObsIoFactory::getMakers() {
  static std::map <std::string, ObsIoFactory*> makers_;
  return makers_;
}

}  // namespace ioda
