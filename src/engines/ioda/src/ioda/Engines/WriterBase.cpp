/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

#include "ioda/Engines/WriterBase.h"

namespace ioda {
namespace Engines {

//---------------------------------------------------------------------
// WriterParametersBase 
//---------------------------------------------------------------------

//---------------------------------------------------------------------
// WriterBase 
//---------------------------------------------------------------------

//--------------------------- public functions ---------------------------------------
WriterBase::WriterBase(const util::DateTime & winStart, const util::DateTime & winEnd,
                       const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                       const std::vector<std::string> & obsVarNames)
                           : winStart_(winStart), winEnd_(winEnd),
                             comm_(comm), timeComm_(timeComm),
                             obsVarNames_(obsVarNames) {
}


//---------------------------------------------------------------------
// WriterFactory
//---------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
WriterFactory::WriterFactory(const std::string & type) {
  if (getMakers().find(type) != getMakers().end()) {
    throw std::runtime_error(type + " already registered in the WriterBase factory.");
  }
  getMakers()[type] = this;
}

//-----------------------------------------------------------------------------------------
std::unique_ptr<WriterBase> WriterFactory::create(const WriterParametersBase & params,
                                                  const util::DateTime & winStart,
                                                  const util::DateTime & winEnd,
                                                  const eckit::mpi::Comm & comm,
                                                  const eckit::mpi::Comm & timeComm,
                                                  const std::vector<std::string> & obsVarNames) {
  oops::Log::trace() << "WriterFactory::create starting" << std::endl;

  const std::string type = params.type;
  std::unique_ptr<WriterBase> ptr = getMaker(type).make(params, winStart, winEnd,
                                                        comm, timeComm, obsVarNames);
  oops::Log::trace() << "WriterFactory::create done" << std::endl;
  return ptr;
}

//-----------------------------------------------------------------------------------------
std::unique_ptr<WriterParametersBase> WriterFactory::createParameters(
                                                 const std::string & type) {
  oops::Log::trace() << "WriterFactory::createParameters starting" << std::endl;
  std::unique_ptr<WriterParametersBase> ptr = getMaker(type).makeParameters();
  oops::Log::trace() << "WriterFactory::createParameters done" << std::endl;
  return ptr;

}

//-----------------------------------------------------------------------------------------
WriterFactory& WriterFactory::getMaker(const std::string & type) {
  typename std::map<std::string, WriterFactory*>::iterator jloc = getMakers().find(type);

  if (jloc == getMakers().end()) {
    std::string makerNameList;
    for (const auto& makerDetails : getMakers()) makerNameList += "\n  " + makerDetails.first;
    throw eckit::BadParameter(type + " does not exist in ioda::WriterFactory. "
                                "Possible values:" + makerNameList, Here());
  }

  return *jloc->second;
}

//-----------------------------------------------------------------------------------------
std::vector<std::string> WriterFactory::getMakerNames() {
  return oops::keys(getMakers());
}

//-----------------------------------------------------------------------------------------
std::map <std::string, WriterFactory*> & WriterFactory::getMakers() {
  static std::map <std::string, WriterFactory*> makers_;
  return makers_;
}

}  // namespace Engines
}  // namespace ioda
