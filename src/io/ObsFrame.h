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
#include "ioda/io/ObsIo.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrame class
/// \author Stephen Herbener (JCSDA)

class ObsFrame : public util::Printable {
 public:
    ObsFrame(const ObsIoActions action, const ObsIoModes mode,
             const ObsSpaceParameters & params);
    virtual ~ObsFrame() = 0;

    /// \brief return number of maximum variable size (along first dimension) from the source
    Dimensions_t maxSrcVarSize() const {return obs_io_->maxVarSize();}

    /// \brief return number of locations from the source
    Dimensions_t numSrcLocs() const {return obs_io_->numLocs();}

    /// \brief return number of regular variables from the source
    Dimensions_t numSrcVars() const {return obs_io_->numVars();}

    /// \brief return number of dimension scale variables from the source
    Dimensions_t numSrcDimVars() const {return obs_io_->numDimVars();}

    /// \brief initialize for walking through the frames
    virtual void frameInit() = 0;

    /// \brief move to the next frame
    virtual void frameNext() = 0;

    /// \brief true if a frame is available (not past end of frames)
    virtual bool frameAvailable() = 0;

    /// \brief return current frame starting index
    /// \param varName name of variable
    virtual Dimensions_t frameStart() = 0;

    /// \brief return current frame count for variable
    /// \details Variables can be of different sizes so it's possible that the
    /// frame has moved past the end of some variables but not so for other
    /// variables. When the frame is past the end of the given variable, this
    /// routine returns a zero to indicate that we're done with this variable.
    /// \param var variable
    virtual Dimensions_t frameCount(const Variable & var) = 0;

 protected:
    //------------------ protected data members ------------------------------
    /// \brief ObsIo object
    std::shared_ptr<ObsIo> obs_io_;

    /// \brief ObsIo action
    ObsIoActions action_;

    /// \brief ObsIo mode
    ObsIoModes mode_;

    /// \brief number of records from source (file or generator)
    Dimensions_t nrecs_;

    /// \brief number of locations from source (file or generator)
    Dimensions_t nlocs_;

    /// \brief ObsIo parameter specs
    ObsSpaceParameters params_;

    /// \brief maximum frame size
    Dimensions_t max_frame_size_;

    /// \brief maximum variable size
    Dimensions_t max_var_size_;

    /// \brief current frame starting index
    Dimensions_t frame_start_;

    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;
};

}  // namespace ioda

#endif  // IO_OBSFRAME_H_
