/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIO_H_
#define IO_OBSIO_H_

#include <iostream>
#include <typeindex>
#include <typeinfo>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

////////////////////////////////////////////////////////////////////////
// Base class for observation data IO
////////////////////////////////////////////////////////////////////////

namespace ioda {

/*! \brief Implementation of ObsIo base class
 *
 * \author Stephen Herbener (JCSDA)
 */

class ObsFrame {
 public:
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

    /// \brief set up frontend and backend selection objects for the given variable
    /// \param var Variable associated with the selection objects
    /// \param feSelect Front end selection object
    /// \param beSelect Back end selection object
    void createFrameSelection(const Variable & var, Selection & feSelect,
                              Selection & beSelect);

 private:
    /// \brief maximum frame size
    Dimensions_t max_size_;

    /// \brief maximum variable size
    Dimensions_t max_var_size_;

    /// \brief current frame starting index
    Dimensions_t start_;
};

class ObsIo : public util::Printable {
 public:
    /// \brief ObsGroup object representing io source/destination
    ObsGroup obs_group_;

    ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsSpaceParameters & params);
    virtual ~ObsIo() = 0;

    /// \brief return number of locations from the source
    std::size_t nlocs() const {return nlocs_;}

    //----------------- Access to frame selection -------------------
    /// \brief initialize for walking through the frames
    void frameInit();

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
    /// \param var ObsGroup variable
    Dimensions_t frameCount(const Variable & var);

    /// \brief set up frontend and backend selection objects for the given variable
    /// \param var ObsGroup variable
    /// \param feSelect Front end selection object
    /// \param beSelect Back end selection object
    void createFrameSelection(const Variable & var, Selection & feSelect, Selection & beSelect);

 protected:
    //------------------ data members ----------------------------------
    /// \brief ObsIo action
    ObsIoActions action_;

    /// \brief ObsIo mode
    ObsIoModes mode_;

    /// \brief ObsIo parameter specs
    ObsSpaceParameters params_;

    /// \brief maximum variable size (ie, first dimension size)
    Dimensions_t max_var_size_;

    /// \brief maximum frame size
    Dimensions_t max_frame_size_;

    /// \brief number of locations from source (file or generator)
    std::size_t nlocs_;

    //------------------ functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;

 private:
    /// \brief ObsFrame object for generating variable selection objects
    ObsFrame obs_frame_;
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
