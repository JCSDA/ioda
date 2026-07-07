/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/load/storeGenDataInFrame.hpp"

#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"
#include "ioda/Engines/EngineUtils.h"

namespace ioda {
namespace reader {

//--------------------------------------------------------------------------------
void storeGenDataInFrame(const Engines::GeneratedObsData & data,
                         std::unique_ptr<osdf::IFrame> & destOsdf,
                         osdf::FrameMetadata & osdfMetadata) {
  const std::size_t numLocs = data.latVals.size();

  // MetaData columns. The latitude/longitude unit strings match the engine-side storeGenData
  // routine, so that GenList/GenRandom produce identical metadata in the OSDF and ObsGroup paths.
  destOsdf->appendNewColumn("MetaData/latitude", data.latVals, std::string("degrees_north"));
  destOsdf->appendNewColumn("MetaData/longitude", data.lonVals, std::string("degrees_east"));
  destOsdf->appendNewColumn("MetaData/dateTime", data.dts, data.epoch);

  if (data.vcoordType == "pressure") {
    destOsdf->appendNewColumn("MetaData/pressure", data.vcoordVals, std::string("Pa"));
  } else if (data.vcoordType == "height") {
    destOsdf->appendNewColumn("MetaData/height", data.vcoordVals, std::string("m"));
  }

  // ObsError (always) and ObsValue (when provided) columns, one per simulated variable.
  // Each value is replicated across all locations. Count ObsValue columns in the frame
  // metadata variable count, matching how the file readers count variables.
  for (std::size_t ivar = 0; ivar < data.obsVarNames.size(); ++ivar) {
    const std::string errName = std::string("ObsError/") + data.obsVarNames[ivar];
    const std::vector<float> errVals(numLocs, data.obsErrors[ivar]);
    destOsdf->appendNewColumn(errName, errVals);

    if (!data.obsValues.empty()) {
      const std::string valName = std::string("ObsValue/") + data.obsVarNames[ivar];
      const std::vector<float> obsVals(numLocs, data.obsValues[ivar]);
      destOsdf->appendNewColumn(valName, obsVals);
      osdfMetadata.incrNumVars();
    }
  }

  // Create the sourceLocationIndices column needed downstream (e.g. UFO geoval test
  // synchronization). Generated data is not filtered during load, so the indices are
  // simply 0..numLocs-1.
  std::vector<int> locationIndices(numLocs);
  std::iota(locationIndices.begin(), locationIndices.end(), 0);
  destOsdf->appendNewColumn("sourceLocationIndices", locationIndices);
}

}  // namespace reader
}  // namespace ioda
