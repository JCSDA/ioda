/*
 * (C) Copyright 2022 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/AssociativeContainers.h"
#include "oops/util/Logger.h"

#include "ioda/Engines/WriterFactory.h"

namespace ioda {
namespace Engines {

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
                                      const WriterCreationParameters & createParams) {
  oops::Log::trace() << "WriterFactory::create starting" << std::endl;

  const std::string type = params.type;
  std::unique_ptr<WriterBase> ptr = getMaker(type).make(params, createParams);
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

//---------------------------------------------------------------------
// WriterProcFactory
//---------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
WriterProcFactory::WriterProcFactory(const std::string & type) {
  if (getMakers().find(type) != getMakers().end()) {
    throw std::runtime_error(type + " already registered in the WriterProcBase factory.");
  }
  getMakers()[type] = this;
}

//-----------------------------------------------------------------------------------------
std::unique_ptr<WriterProcBase> WriterProcFactory::create(const WriterParametersBase & params,
                                      const WriterCreationParameters & createParams) {
  oops::Log::trace() << "WriterProcFactory::create starting" << std::endl;

  const std::string type = params.type;
  std::unique_ptr<WriterProcBase> ptr = getMaker(type).make(params, createParams);
  oops::Log::trace() << "WriterProcFactory::create done" << std::endl;
  return ptr;
}

//-----------------------------------------------------------------------------------------
WriterProcFactory& WriterProcFactory::getMaker(const std::string & type) {
  typename std::map<std::string, WriterProcFactory*>::iterator jloc = getMakers().find(type);

  if (jloc == getMakers().end()) {
    std::string makerNameList;
    for (const auto& makerDetails : getMakers()) makerNameList += "\n  " + makerDetails.first;
    throw eckit::BadParameter(type + " does not exist in ioda::WriterProcFactory. "
                                "Possible values:" + makerNameList, Here());
  }

  return *jloc->second;
}

//-----------------------------------------------------------------------------------------
std::vector<std::string> WriterProcFactory::getMakerNames() {
  return oops::keys(getMakers());
}

//-----------------------------------------------------------------------------------------
std::map <std::string, WriterProcFactory*> & WriterProcFactory::getMakers() {
  static std::map <std::string, WriterProcFactory*> makers_;
  return makers_;
}

//----------------------------------------------------------------------------------------
// Writer factory utilities
//----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
std::unique_ptr<WriterBase> constructFileWriterFromConfig(
                const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                const bool createMultipleFiles, const bool isParallelIo,
                const eckit::LocalConfiguration & config) {
    oops::Log::debug() << "constructFileWriterFromConfig: config: " << config << std::endl;
    // Need two sets of parameters, the engine parameters (built from the eckit config)
    // and the creation parameters (for the reader constructor). 
    WriterParametersWrapper writerParams;          // this will contain the engine parameters
    writerParams.validateAndDeserialize(config.getSubConfiguration("engine"));
    Engines::WriterCreationParameters createParams(comm, timeComm, createMultipleFiles,
                                                   isParallelIo);
    return WriterFactory::create(writerParams.engineParameters, createParams);
}

}  // namespace Engines
}  // namespace ioda
