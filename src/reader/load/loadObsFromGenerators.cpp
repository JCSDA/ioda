/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/load/loadObsFromGenerators.hpp"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/GenList.h"
#include "ioda/Engines/GenRandom.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/load/storeGenDataInFrame.hpp"

namespace ioda {
namespace reader {

//--------------------------------------------------------------------------------
void loadOsdfFromGenList(const ObsDataInParameters & dataInParams,
                         const std::vector<std::string> & obsVarNames,
                         std::unique_ptr<osdf::IFrame> & destOsdf,
                         osdf::FrameMetadata & osdfMetadata) {
  const auto & genParams = dynamic_cast<const Engines::GenListParameters &>(
    dataInParams.engine.value().engineParameters.value());

  const Engines::GeneratedObsData data = Engines::generateObsList(genParams, obsVarNames);
  storeGenDataInFrame(data, destOsdf, osdfMetadata);
}

//--------------------------------------------------------------------------------
void loadOsdfFromGenRandom(const ObsDataInParameters & dataInParams,
                           const std::vector<std::string> & obsVarNames,
                           const util::TimeWindow & timeWindow,
                           std::unique_ptr<osdf::IFrame> & destOsdf,
                           osdf::FrameMetadata & osdfMetadata) {
  const auto & genParams = dynamic_cast<const Engines::GenRandomParameters &>(
    dataInParams.engine.value().engineParameters.value());

  const Engines::GeneratedObsData data =
    Engines::generateObsRandom(genParams, obsVarNames, timeWindow);
  storeGenDataInFrame(data, destOsdf, osdfMetadata);
}

}  // namespace reader
}  // namespace ioda
