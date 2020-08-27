/*
 * (C) Copyright 2017-2019 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/io/ObsIo.h"

////////////////////////////////////////////////////////////////////////
// Implementation of ObsIo base class
////////////////////////////////////////////////////////////////////////

namespace ioda {

//------------------------------------------------------------------------------------
ObsIo::ObsIo(const ObsIoActions action, const ObsIoModes mode,
             const ObsIoParameters & params) : action_(action), mode_(mode), params_(params) {
    oops::Log::trace() << "Constructing ObsIo" << std::endl;
}

ObsIo::~ObsIo() {}

//------------------------------------------------------------------------------------
void ObsIo::frame_info_init(std::size_t maxVarSize) {
  // Chop the maxVarSize into max_frame_size_ pieces. Make sure the total
  // of the sizes of all frames adds up to maxVarSize.
  std::size_t frameStart = 0;
  while (frameStart < maxVarSize) {
    std::size_t frameSize = max_frame_size_;
    if ((frameStart + frameSize) > maxVarSize) {
      frameSize = maxVarSize - frameStart;
    }
    ObsIo::FrameInfoRec finfo(frameStart, frameSize);
    frame_info_.push_back(finfo);

    frameStart += max_frame_size_;
  }
}

//------------------------------------------------------------------------------------
void ObsIo::print(std::ostream & os) const {}

}  // namespace ioda
