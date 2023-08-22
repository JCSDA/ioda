/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/ioPool/WriterPoolFactory.h"

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace IoPool {

//---------------------------------------------------------------------
// WriterPoolFactory
//---------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
WriterPoolFactory::WriterPoolFactory(const std::string & name) {
  if (getMakers().find(name) != getMakers().end()) {
    throw std::runtime_error(name + " already registered in the WriterPoolBase factory.");
  }
  getMakers()[name] = this;
}

//-----------------------------------------------------------------------------------------
std::unique_ptr<WriterPoolBase> WriterPoolFactory::create(const IoPoolParameters & configParams,
                                      const WriterPoolCreationParameters & createParams) {
  oops::Log::trace() << "WriterPoolFactory::create starting" << std::endl;

  const std::string name = configParams.writerPoolName;
  std::unique_ptr<WriterPoolBase> ptr = getMaker(name).make(configParams, createParams);
  oops::Log::trace() << "WriterPoolFactory::create done" << std::endl;
  return ptr;
}

//-----------------------------------------------------------------------------------------
WriterPoolFactory& WriterPoolFactory::getMaker(const std::string & name) {
  typename std::map<std::string, WriterPoolFactory*>::iterator jloc = getMakers().find(name);

  if (jloc == getMakers().end()) {
    std::string makerNameList;
    for (const auto& makerDetails : getMakers()) makerNameList += "\n  " + makerDetails.first;
    throw eckit::BadParameter(name + " does not exist in ioda::WriterPoolFactory. "
                                "Possible values:" + makerNameList, Here());
  }

  return *jloc->second;
}

//-----------------------------------------------------------------------------------------
std::vector<std::string> WriterPoolFactory::getMakerNames() {
  return oops::keys(getMakers());
}

//-----------------------------------------------------------------------------------------
std::map <std::string, WriterPoolFactory*> & WriterPoolFactory::getMakers() {
  static std::map <std::string, WriterPoolFactory*> makers_;
  return makers_;
}

}  // namespace IoPool
}  // namespace ioda
