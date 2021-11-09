/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/Engines/HH.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/Layout.h"
#include "ioda/Variables/Variable.h"

namespace ioda {

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------
  ObsFrame::ObsFrame(const ObsSpaceParameters & params) :
      params_(params), gnlocs_(0), gnlocs_outside_timewindow_(0) {
    oops::Log::trace() << "Constructing ObsFrame" << std::endl;
}

//------------------------------------------------------------------------------------
Selection ObsFrame::createMemSelection(const std::vector<Dimensions_t> & varShape,
                                           const Dimensions_t frameCount) {
    // Use hyperslab selection on the memory side to make sure a slab selected
    // from a backend with a start greater than zero ends up in the memory
    // starting at zero. If the memory is left with a default selection ("ALL")
    // ioda-engines will allocate the memory to match the entire size of the backend variable
    // and place the selected slab from the backend in the exact same position in the memory.
    std::vector<Dimensions_t> memShape = varShape;
    memShape[0] = frameCount;

    // Treat the memory side as a buffer (vector) so make sure any extra dimensions
    // are accounted for in the size.
    Dimensions_t numElements = std::accumulate(
        memShape.begin(), memShape.end(), 1, std::multiplies<Dimensions_t>());

    std::vector<Dimensions_t> memStarts(1, 0);
    std::vector<Dimensions_t> memCounts(1, numElements);
    Selection memSelect;
    memSelect.extent(memCounts).select({ SelectionOperator::SET, memStarts, memCounts });
    return memSelect;
}

//------------------------------------------------------------------------------------
Selection ObsFrame::createEntireFrameSelection(const std::vector<Dimensions_t> & varShape,
                                                   const Dimensions_t frameCount) {
    // In this case we want the memory select to go from zero to frameCount in the
    // first dimension of the variable.
    std::vector<Dimensions_t> entireFrameCounts = varShape;
    entireFrameCounts[0] = frameCount;

    // The frame extent will go from zero to frameCount in the first dimension
    // of the varShape. varShape is the shape from the storage containing the
    // entire variable which might be larger than one frame.
    std::vector<Dimensions_t> frameVarShape = varShape;
    frameVarShape[0] = frameCount;

    // Treat the frame side as multi-dimensioned storage. Take the entire range for
    // the second dimension, third dimension, etc.
    std::vector<Dimensions_t> entireFrameStarts(entireFrameCounts.size(), 0);
    Selection entireFrameSelect;
    entireFrameSelect.extent(frameVarShape)
        .select({ SelectionOperator::SET, entireFrameStarts, entireFrameCounts });
    return entireFrameSelect;
}

//------------------------------------------------------------------------------------
Selection ObsFrame::createVarSelection(const std::vector<Dimensions_t> & varShape,
                                       const Dimensions_t frameStart,
                                       const Dimensions_t frameCount) {
    // In this case we want the memory select to go from frameStart and be of size frameCount
    // in the first dimension of the variable.
    std::vector<Dimensions_t> varCounts = varShape;
    varCounts[0] = frameCount;

    // Treat the frame size as multi-dimensioned storage. Take the entire range for
    // the second dimension, third dimension, etc.
    std::vector<Dimensions_t> varStarts(varCounts.size(), 0);
    varStarts[0] = frameStart;
    Selection varSelect;
    varSelect.extent(varShape).select({ SelectionOperator::SET, varStarts, varCounts });
    return varSelect;
}

//--------------------------- protected functions ---------------------------------------
//------------------------------------------------------------------------------------
Selection ObsFrame::createObsIoSelection(const std::vector<Dimensions_t> & varShape,
                                             const Dimensions_t frameStart,
                                             const Dimensions_t frameCount) {
    // We want ObsIo selection to go from frameStart to frameCount in the first
    // dimension of the variable shape.
    std::vector<Dimensions_t> obsIoCounts = varShape;
    obsIoCounts[0] = frameCount;

    // Treat the ObsIo side as multi-dimensioned storage. Take the entire range for
    // the second dimension, third dimension, etc.
    std::vector<Dimensions_t> obsIoStarts(obsIoCounts.size(), 0);
    obsIoStarts[0] = frameStart;
    Selection obsIoSelect;
    obsIoSelect.extent(varShape).select({ SelectionOperator::SET, obsIoStarts, obsIoCounts });
    return obsIoSelect;
}

//------------------------------------------------------------------------------------
void ObsFrame::createFrameFromObsGroup(const VarNameObjectList & varList,
                                       const VarNameObjectList & dimVarList,
                                       const VarDimMap & varDimMap) {
    // create an ObsGroup with an in-memory backend
    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    backendParams.fileName = ioda::Engines::HH::genUniqueName();
    backendParams.allocBytes = 1024*1024*50;
    backendParams.flush = false;

    backendName = Engines::BackendNames::ObsStore;  // Hdf5Mem;  ObsStore;
    Group backend = constructBackend(backendName, backendParams);

    // create dimensions for frame
    NewDimensionScales_t newDims;
    for (auto & dimNameObject : dimVarList) {
        std::string dimName = dimNameObject.first;
        Variable srcDimVar = dimNameObject.second;
        Dimensions_t dimSize = dimNameObject.second.getDimensions().dimsCur[0];
        // Don't allow nchans to be limited by the frame size since nchans is
        // the second dimension (and we are only limiting the frame size on
        // the first dimension, typically nlocs).
        if (dimName != "nchans") {
            if (dimSize > max_frame_size_) {
                dimSize = max_frame_size_;
            }
        }
        Dimensions_t maxDimSize = dimSize;
        Dimensions_t chunkSize = dimSize;

        if (srcDimVar.isA<int>()) {
            newDims.push_back(
                ioda::NewDimensionScale<int>(
                    dimName, dimSize, maxDimSize, chunkSize));
        } else if (srcDimVar.isA<float>()) {
            newDims.push_back(
                ioda::NewDimensionScale<float>(
                    dimName, dimSize, maxDimSize, chunkSize));
        }
    }
    obs_frame_ = ObsGroup::generate(backend, newDims);

    // fill in dimension coordinate values
    for (auto & dimVarNameObject : dimVarList) {
        std::string dimVarName = dimVarNameObject.first;
        Variable srcDimVar = dimVarNameObject.second;
        Variable destDimVar = obs_frame_.vars.open(dimVarName);

        // Set up the dimension selection objects. The prior loop declared the
        // sizes of all the dimensions in the frame so use that as a guide, and
        // transfer the first frame's worth of coordinate values accordingly.
        std::vector<Dimensions_t> srcDimShape = srcDimVar.getDimensions().dimsCur;
        std::vector<Dimensions_t> destDimShape = destDimVar.getDimensions().dimsCur;
        Dimensions_t frameCount = destDimShape[0];
        // Transfer the coordinate values
        if (frameCount > 0) {
            Selection srcSelect = createObsIoSelection(srcDimShape, 0, frameCount);
            Selection memSelect = createMemSelection(destDimShape, frameCount);
            Selection destSelect = createEntireFrameSelection(destDimShape, frameCount);

            if (srcDimVar.isA<int>()) {
                std::vector<int> dimCoords;
                srcDimVar.read<int>(dimCoords, memSelect, srcSelect);
                destDimVar.write<int>(dimCoords, memSelect, destSelect);
            } else if (srcDimVar.isA<float>()) {
                std::vector<float> dimCoords;
                srcDimVar.read<float>(dimCoords, memSelect, srcSelect);
                destDimVar.write<float>(dimCoords, memSelect, destSelect);
            }
        }
    }

    // create variables for frame
    for (auto & varNameObject : varList) {
        std::string varName = varNameObject.first;

        // get the dimensions attached to this variable
        std::vector<std::string> dimVarNames = varDimMap.at(varName);
        std::vector<Variable> dimVars;
        for (auto & dimVarName : dimVarNames) {
          dimVars.push_back(obs_frame_.vars.open(dimVarName));
        }

        Variable sourceVar = varNameObject.second;
        forAnySupportedVariableType(
              sourceVar,
              [&](auto typeDiscriminator) {
                  typedef decltype(typeDiscriminator) T;
                  VariableCreationParameters params;
                  if (sourceVar.hasFillValue()) {
                      auto varFillValue = sourceVar.getFillValue();
                      params.setFillValue<T>(ioda::detail::getFillValue<T>(varFillValue));
                  }
                  Variable destVar = obs_frame_.vars.createWithScales<T>(
                      varName, dimVars, params);
                  copyAttributes(sourceVar.atts, destVar.atts);
              },
              ThrowIfVariableIsOfUnsupportedType(varName));
    }
}

}  // namespace ioda
