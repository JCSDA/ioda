/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/util/abor1_cpp.h"

#include "oops/util/Logger.h"

#include "ioda/io/ObsFrameWrite.h"

namespace ioda {

//--------------------------- public functions ---------------------------------------
//------------------------------------------------------------------------------------

  ObsFrameWrite::ObsFrameWrite(const ObsSpaceParameters & params) :
    ObsFrame(params) {
    // Create the ObsIo object
    obs_io_ = ObsIoFactory::create(ObsIoModes::WRITE, params);
    max_frame_size_ = params.top_level_.obsOutFile.value()->maxFrameSize;
    oops::Log::debug() << "ObsFrameWrite: maximum frame size: " << max_frame_size_ << std::endl;
}

ObsFrameWrite::~ObsFrameWrite() {}

//------------------------------------------------------------------------------------
void ObsFrameWrite::frameInit(const VarNameObjectList & varList,
                              const VarNameObjectList & dimVarList,
                              const VarDimMap & varDimMap, const Dimensions_t maxVarSize) {
    frame_start_ = 0;
    max_var_size_ = maxVarSize;

    // create an ObsGroup based frame with an in-memory backend
    createFrameFromObsGroup(varList, dimVarList, varDimMap);

    // copy dimension coordinates from the newly created frame to the ObsIo backend
    copyObsIoDimCoords(obs_frame_.vars, obs_io_->vars(), dimVarList);

    // create variables in the ObsIo backend
    createObsIoVariables(obs_frame_.vars, obs_io_->vars(), varDimMap);
}

//------------------------------------------------------------------------------------
void ObsFrameWrite::frameNext(const VarNameObjectList & varList) {
    // Transfer the current frame variable data to the ObsIo backend.
    Dimensions_t frameStart = this->frameStart();
    for (auto & varNameObject : varList) {
        std::string varName = varNameObject.first;
        Variable sourceVar = obs_frame_.vars.open(varName);
        Dimensions_t frameCount = this->frameCount(varName);
        if (frameCount > 0) {
            // Transfer the variable data for this frame. Do this in two steps:
            //    frame storage --> memory buffer --> ObsIo

            // Selection objects for transfer;
            Variable destVar = obs_io_->vars().open(varName);
            std::vector<Dimensions_t> destVarShape = destVar.getDimensions().dimsCur;
            std::vector<Dimensions_t> sourceVarShape = sourceVar.getDimensions().dimsCur;

            Selection obsFrameSelect = createEntireFrameSelection(sourceVarShape, frameCount);
            Selection memBufferSelect = createMemSelection(sourceVarShape, frameCount);
            Selection obsIoSelect = createObsIoSelection(destVarShape, frameStart, frameCount);

            // Transfer the data
            if (destVar.isA<int>()) {
                std::vector<int> varValues;
                sourceVar.read<int>(varValues, memBufferSelect, obsFrameSelect);
                destVar.write<int>(varValues, memBufferSelect, obsIoSelect);
            } else if (destVar.isA<float>()) {
                std::vector<float> varValues;
                sourceVar.read<float>(varValues, memBufferSelect, obsFrameSelect);
                destVar.write<float>(varValues, memBufferSelect, obsIoSelect);
            } else if (destVar.isA<std::string>()) {
                std::vector<std::string> varValues;
                sourceVar.read<std::string>(varValues, memBufferSelect, obsFrameSelect);
                destVar.write<std::string>(varValues, memBufferSelect, obsIoSelect);
            }
        }
    }

    frame_start_ += max_frame_size_;
}

//------------------------------------------------------------------------------------
bool ObsFrameWrite::frameAvailable() {
    return (frame_start_ < max_var_size_);
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameWrite::frameStart() {
    return frame_start_;
}

//------------------------------------------------------------------------------------
Dimensions_t ObsFrameWrite::frameCount(const std::string & varName) {
    Dimensions_t count;
    Variable var = obs_io_->vars().open(varName);
    Dimensions_t varSize0 = var.getDimensions().dimsCur[0];
    if ((frame_start_ + max_frame_size_) > varSize0) {
        count = varSize0 - frame_start_;
        if (count < 0) { count = 0; }
    } else {
        count = max_frame_size_;
    }
    return count;
}

//-----------------------------------------------------------------------------------
void ObsFrameWrite::writeFrameVar(const std::string & varName,
                                  const std::vector<int> & varData) {
    writeFrameVarHelper<int>(varName, varData);
}
void ObsFrameWrite::writeFrameVar(const std::string & varName,
                                  const std::vector<float> & varData) {
    writeFrameVarHelper<float>(varName, varData);
}
void ObsFrameWrite::writeFrameVar(const std::string & varName,
                                  const std::vector<std::string> & varData) {
    writeFrameVarHelper<std::string>(varName, varData);
}

//--------------------------- private functions --------------------------------------

// -----------------------------------------------------------------------------
void ObsFrameWrite::copyObsIoDimCoords(const Has_Variables & srcVarContainer,
                                       Has_Variables & destVarContainer,
                                       const VarNameObjectList & dimVarList) {
    // fill in dimension coordinate values
    for (auto & dimVarNameObject : dimVarList) {
        std::string dimVarName = dimVarNameObject.first;
        Variable srcDimVar = srcVarContainer.open(dimVarName);
        Variable destDimVar = destVarContainer.open(dimVarName);

        // Set up the dimension selection objects.
        std::vector<Dimensions_t> srcDimShape = srcDimVar.getDimensions().dimsCur;
        std::vector<Dimensions_t> destDimShape = destDimVar.getDimensions().dimsCur;
        Dimensions_t frameCount = srcDimShape[0];
        // Transfer the coordinate values
        if (frameCount > 0) {
            Selection srcSelect = createEntireFrameSelection(srcDimShape, frameCount);
            Selection memSelect = createMemSelection(destDimShape, frameCount);
            Selection destSelect = createObsIoSelection(destDimShape, 0, frameCount);

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
}

// -----------------------------------------------------------------------------
void ObsFrameWrite::createObsIoVariables(const Has_Variables & srcVarContainer,
                                         Has_Variables & destVarContainer,
                                         const VarDimMap & dimsAttachedToVars) {
    // Walk through map to get list of variables to create along with
    // their dimensions. Use the srcVarContainer to get the var data type.
    for (auto & ivar : dimsAttachedToVars) {
        std::string varName = ivar.first;
        std::vector<std::string> varDimNames = ivar.second;

        VariableCreationParameters params;
        params.chunk = true;
        params.compressWithGZIP();

        // Create a vector with dimension scale vector from destination container
        std::vector<Variable> varDims;
        for (auto & dimVarName : varDimNames) {
            varDims.push_back(destVarContainer.open(dimVarName));
        }

        Variable srcVar = srcVarContainer.open(varName);
        if (srcVar.isA<int>()) {
            if (srcVar.hasFillValue()) {
                auto varFillValue = srcVar.getFillValue();
                params.setFillValue<int>(ioda::detail::getFillValue<int>(varFillValue));
            }
            destVarContainer.createWithScales<int>(varName, varDims, params);
        } else if (srcVar.isA<float>()) {
            if (srcVar.hasFillValue()) {
                auto varFillValue = srcVar.getFillValue();
                params.setFillValue<float>(ioda::detail::getFillValue<float>(varFillValue));
            }
            destVarContainer.createWithScales<float>(varName, varDims, params);
        } else if (srcVar.isA<std::string>()) {
            if (srcVar.hasFillValue()) {
                auto varFillValue = srcVar.getFillValue();
                params.setFillValue<std::string>(
                    ioda::detail::getFillValue<std::string>(varFillValue));
            }
            destVarContainer.createWithScales<std::string>(varName, varDims, params);
        } else {
            oops::Log::warning() << "WARNING: ObsWriteFrame::createObsIoVariables: "
                << "Skipping variable due to an unexpected data type for variable: "
                << varName << std::endl;
        }
    }
}

//------------------------------------------------------------------------------------
void ObsFrameWrite::createFrameSelection(const std::string & varName, Selection & feSelect,
                                         Selection & beSelect) {
    // Form the selection objects for this frame. The frontend selection will
    // simply be the current size (according to the variable) of the frame going from
    // 0 to size-1.
    //
    // The backend selection will start at the frame start value (instead of zero) and
    // be determined by the size of the frame for the given variable.

    // Grab the variable dimensions and use this as a template for the selection operators.
    std::vector<Dimensions_t> varDims = obs_io_->vars().open(varName).getDimensions().dimsCur;
    Dimensions_t frameStart = this->frameStart();
    Dimensions_t frameCount = this->frameCount(varName);

    // Substitute the frameCount for the first dimension size of the variable.
    varDims[0] = frameCount;

    // For the frontend, treat the data as a vector with a total size given by the product
    // of the dimension sizes considering the possible adjustment of the fisrt
    // dimension (varDims). Use hyperslab style selection since we will be consolidating
    // the selected locations into a contiguous series.
    Dimensions_t numElements = std::accumulate(
        varDims.begin(), varDims.end(), 1, std::multiplies<Dimensions_t>());
    std::vector<Dimensions_t> feStarts(1, 0);
    std::vector<Dimensions_t> feCounts(1, numElements);
    feSelect.extent(feCounts).select({ SelectionOperator::SET, feStarts, feCounts });

    // For the backend, use a hyperslab style.
    std::vector<Dimensions_t> beStarts(varDims.size(), 0);
    beStarts[0] = frameStart;
    std::vector<Dimensions_t> beCounts = varDims;
    beSelect.select({ SelectionOperator::SET, beStarts, beCounts });
}

//-----------------------------------------------------------------------------------
void ObsFrameWrite::print(std::ostream & os) const {
    os << "ObsFrameWrite: " << std::endl;
}

}  // namespace ioda
