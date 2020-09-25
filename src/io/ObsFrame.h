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

    /// \brief return number of maximum variable size (along first dimension) from ObsIo
    Dimensions_t ioMaxVarSize() const {return obs_io_->maxVarSize();}

    /// \brief return number of locations from ObsIo
    Dimensions_t ioNumLocs() const {return obs_io_->numLocs();}

    /// \brief return number of regular variables from ObsIo
    Dimensions_t ioNumVars() const {return obs_io_->numVars();}

    /// \brief return number of dimension scale variables from ObsIo
    Dimensions_t ioNumDimVars() const {return obs_io_->numDimVars();}

    /// \brief return number of dimension scale variables from ObsIo
    Has_Variables & vars() const {return obs_io_->vars();}

    /// \brief return list of regular variables from ObsIo
    std::vector<std::string> ioVarList() const {return obs_io_->varList();}

    /// \brief return list of dimension scale variables from ObsIo
    std::vector<std::string> ioDimVarList() const {return obs_io_->dimVarList();}

    /// \brief return list of regular variables from ObsIo
    void ioResetVarList() const {return obs_io_->resetVarList();}

    /// \brief return list of dimension scale variables from ObsIo
    void ioResetDimVarList() const {return obs_io_->resetDimVarList();}

    /// \brief return number of locations
    virtual std::size_t frameNumLocs() const {return nlocs_;}

    /// \brief return number of records
    virtual std::size_t frameNumRecs() const {return nrecs_;}

    /// \brief return list of indices indicating which locations were selected from ObsIo
    virtual std::vector<std::size_t> index() const {return std::vector<std::size_t>{};}

    /// \brief return list of record numbers from ObsIo
    virtual std::vector<std::size_t> recnums() const {return std::vector<std::size_t>{};}

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

    /// \brief return adjusted nlocs frame start
    virtual Dimensions_t adjNlocsFrameStart() const {return 0;}

    /// \brief return adjusted nlocs frame count
    virtual Dimensions_t adjNlocsFrameCount() const {return 0;}

    /// \brief generate frame indices and corresponding record numbers
    /// \details This method generates a list of indices with their corresponding
    ///  record numbers, where the indices denote which locations are to be
    ///  read into this process element.
    virtual void genFrameIndexRecNums(std::shared_ptr<Distribution> & dist) {}

    /// \brief set up frontend and backend selection objects for the given variable
    /// \param var ObsGroup variable
    /// \param feSelect Front end selection object
    /// \param beSelect Back end selection object
    virtual void createFrameSelection(const Variable & var, Selection & feSelect,
                                      Selection & beSelect) {}

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
