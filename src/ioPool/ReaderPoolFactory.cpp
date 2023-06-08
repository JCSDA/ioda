/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/ioPool/ReaderPoolFactory.h"

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

namespace ioda {

//---------------------------------------------------------------------
// ReaderPoolFactory
//---------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
ReaderPoolFactory::ReaderPoolFactory(const std::string & name) {
  if (getMakers().find(name) != getMakers().end()) {
    throw std::runtime_error(name + " already registered in the ReaderPoolBase factory.");
  }
  getMakers()[name] = this;
}

//-----------------------------------------------------------------------------------------
std::unique_ptr<ReaderPoolBase> ReaderPoolFactory::create(const IoPoolParameters & configParams,
                                             const ReaderPoolCreationParameters & createParams) {
  oops::Log::trace() << "ReaderPoolFactory::create starting" << std::endl;

  const std::string name = configParams.readerPoolName;
  std::unique_ptr<ReaderPoolBase> ptr = getMaker(name).make(configParams, createParams);
  oops::Log::trace() << "ReaderPoolFactory::create done" << std::endl;
  return ptr;
}

//-----------------------------------------------------------------------------------------
ReaderPoolFactory& ReaderPoolFactory::getMaker(const std::string & name) {
  typename std::map<std::string, ReaderPoolFactory*>::iterator jloc = getMakers().find(name);

  if (jloc == getMakers().end()) {
    std::string makerNameList;
    for (const auto& makerDetails : getMakers()) makerNameList += "\n  " + makerDetails.first;
    throw eckit::BadParameter(name + " does not exist in ioda::ReaderPoolFactory. "
                                "Possible values:" + makerNameList, Here());
  }

  return *jloc->second;
}

//-----------------------------------------------------------------------------------------
std::vector<std::string> ReaderPoolFactory::getMakerNames() {
  return oops::keys(getMakers());
}

//-----------------------------------------------------------------------------------------
std::map <std::string, ReaderPoolFactory*> & ReaderPoolFactory::getMakers() {
  static std::map <std::string, ReaderPoolFactory*> makers_;
  return makers_;
}

}  // namespace ioda
