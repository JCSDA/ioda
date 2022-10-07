/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

#include "ioda/Engines/ReaderBase.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// ReaderParametersBase 
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// ReaderBase 
//---------------------------------------------------------------------

//--------------------------- public functions ---------------------------------------
ReaderBase::ReaderBase(const util::DateTime & winStart, const util::DateTime & winEnd,
                       const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                       const std::vector<std::string> & obsVarNames)
                           : winStart_(winStart), winEnd_(winEnd),
                             comm_(comm), timeComm_(timeComm),
                             obsVarNames_(obsVarNames) {
}


//---------------------------------------------------------------------
// ReaderFactory
//---------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
ReaderFactory::ReaderFactory(const std::string & type) {
  if (getMakers().find(type) != getMakers().end()) {
    throw std::runtime_error(type + " already registered in the ReaderBase factory.");
  }
  getMakers()[type] = this;
}

//-----------------------------------------------------------------------------------------
std::unique_ptr<ReaderBase> ReaderFactory::create(const ReaderParametersBase & params,
                                                  const util::DateTime & winStart,
                                                  const util::DateTime & winEnd,
                                                  const eckit::mpi::Comm & comm,
                                                  const eckit::mpi::Comm & timeComm,
                                                  const std::vector<std::string> & obsVarNames) {
  oops::Log::trace() << "ReaderFactory::create starting" << std::endl;

  const std::string type = params.type;
  std::unique_ptr<ReaderBase> ptr = getMaker(type).make(params, winStart, winEnd,
                                                        comm, timeComm, obsVarNames);
  oops::Log::trace() << "ReaderFactory::create done" << std::endl;
  return ptr;
}

//-----------------------------------------------------------------------------------------
std::unique_ptr<ReaderParametersBase> ReaderFactory::createParameters(
                                                 const std::string & type) {
  oops::Log::trace() << "ReaderFactory::createParameters starting" << std::endl;
  std::unique_ptr<ReaderParametersBase> ptr = getMaker(type).makeParameters();
  oops::Log::trace() << "ReaderFactory::createParameters done" << std::endl;
  return ptr;

}

//-----------------------------------------------------------------------------------------
ReaderFactory& ReaderFactory::getMaker(const std::string & type) {
  typename std::map<std::string, ReaderFactory*>::iterator jloc = getMakers().find(type);

  if (jloc == getMakers().end()) {
    std::string makerNameList;
    for (const auto& makerDetails : getMakers()) makerNameList += "\n  " + makerDetails.first;
    throw eckit::BadParameter(type + " does not exist in ioda::ReaderFactory. "
                                "Possible values:" + makerNameList, Here());
  }

  return *jloc->second;
}

//-----------------------------------------------------------------------------------------
std::vector<std::string> ReaderFactory::getMakerNames() {
  return oops::keys(getMakers());
}

//-----------------------------------------------------------------------------------------
std::map <std::string, ReaderFactory*> & ReaderFactory::getMakers() {
  static std::map <std::string, ReaderFactory*> makers_;
  return makers_;
}

}  // namespace Engines
}  // namespace ioda
