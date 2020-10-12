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

/// \details The ObsFrame class along with its subclasses are responsible for appying
/// the timing window filtering, record number assignment (according to the obsgrouping
/// specification) and the application of MPI distribution when transferring data from a
/// source (file or generator) into memory. For data transfer in both directions (source to
/// memory, and memory to destination), this class also provides a means for transferring
/// data in chunks (first n locations, followed by second n locations, etc.) which for
/// a transfer from a file to memory can be utilized to avoid reading in the whole file
/// into memory before applying the filtering and distribution.
///
/// \author Stephen Herbener (JCSDA)

class ObsFrame : public util::Printable {
 public:
    ObsFrame(const ObsIoActions action, const ObsIoModes mode,
             const ObsSpaceParameters & params);
    ~ObsFrame() {}

    /// \brief return number of maximum variable size (along first dimension) from ObsIo
    Dimensions_t ioMaxVarSize() const {return obs_io_->maxVarSize();}

    /// \brief return number of locations from ObsIo
    Dimensions_t ioNumLocs() const {return obs_io_->numLocs();}

    /// \brief return number of regular variables from ObsIo
    Dimensions_t ioNumVars() const {return obs_io_->numVars();}

    /// \brief return number of dimension scale variables from ObsIo
    Dimensions_t ioNumDimVars() const {return obs_io_->numDimVars();}

    /// \brief return variables container from ObsIo
    Has_Variables & vars() const {return obs_io_->vars();}

    /// \brief return attributes container from ObsIo
    Has_Attributes & atts() const {return obs_io_->atts();}

    /// \brief return list of regular variables from ObsIo
    std::vector<std::string> ioVarList() const {return obs_io_->varList();}

    /// \brief return list of dimension scale variables from ObsIo
    std::vector<std::string> ioDimVarList() const {return obs_io_->dimVarList();}

    /// \brief return map from variables to their attached dimension scales
    VarDimMap ioVarDimMap() const {return obs_io_->varDimMap();}

    /// \brief return true if variable is dimensioned by nlocs
    bool ioIsVarDimByNlocs(const std::string & varName) const {
        return obs_io_->isVarDimByNlocs(varName);
    }

    /// \brief reset lists of regular and dimension scale variables from ObsIo
    void ioResetVarLists() const {return obs_io_->resetVarLists();}

    /// \brief reset map from variables to their attached dimension scales
    void ioResetVarDimMap() const {return obs_io_->resetVarDimMap();}

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
    /// \param varName variable name
    virtual Dimensions_t frameCount(const std::string & varName) = 0;

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
    /// \param varName ObsGroup variable name
    /// \param feSelect Front end selection object
    /// \param beSelect Back end selection object
    virtual void createFrameSelection(const std::string & varName, Selection & feSelect,
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