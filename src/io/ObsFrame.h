/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFRAME_H_
#define IO_OBSFRAME_H_

#include <iostream>
#include <typeindex>
#include <typeinfo>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrame class
/// \author Stephen Herbener (JCSDA)

class ObsFrame {
 public:
    ObsFrame(const ObsIoActions action, const ObsIoModes mode,
             const ObsSpaceParameters & params);
    virtual ~ObsFrame() = 0;

    /// \brief initialize for walking through the frames
    void frameInit(const Dimensions_t maxVarSize, const Dimensions_t maxFrameSize);

    /// \brief move to the next frame
    void frameNext();

    /// \brief true if a frame is available (not past end of frames)
    bool frameAvailable();

    /// \brief return current frame starting index
    /// \param varName name of variable
    Dimensions_t frameStart();

    /// \brief return current frame count for variable
    /// \details Variables can be of different sizes so it's possible that the
    /// frame has moved past the end of some variables but not so for other
    /// variables. When the frame is past the end of the given variable, this
    /// routine returns a zero to indicate that we're done with this variable.
    /// \param var variable
    Dimensions_t frameCount(const Variable & var);

 protected:
    /// \brief ObsIo action
    ObsIoActions action_;

    /// \brief ObsIo mode
    ObsIoModes mode_;

    /// \brief ObsIo parameter specs
    ObsSpaceParameters params_;

    /// \brief maximum frame size
    Dimensions_t max_size_;

    /// \brief maximum variable size
    Dimensions_t max_var_size_;

    /// \brief current frame starting index
    Dimensions_t start_;
};

}  // namespace ioda

#endif  // IO_OBSFRAME_H_
