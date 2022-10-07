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
#include "ioda/core/IodaUtils.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/VarUtils.h"

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
    explicit ObsFrame(const ObsSpaceParameters & params);
    ~ObsFrame() {}

    /// \brief return the ObsGroup that stores the frame data
    inline ObsGroup getObsGroup() { return obs_frame_; }

    /// \brief return the ObsGroup that stores the frame data
    inline const ObsGroup getObsGroup() const { return obs_frame_; }

    /// \brief return the list of variables with their associated dimensions from the frame
    VarUtils::VarDimMap varDimMap() const {return dims_attached_to_vars_;}

    /// \brief return the list of variables from the backend
    VarUtils::Vec_Named_Variable backendDimVarList() const {return backend_dim_var_list_;}

    /// \brief return list of regular variables
    const VarUtils::Vec_Named_Variable & varList() const {return var_list_; }

    /// \brief return number of maximum variable size (along first dimension)
    Dimensions_t maxVarSize() const {return max_var_size_;}

    /// \brief return true if variable's first dimension is nlocs
    /// \param varName variable name to check
    bool isVarDimByNlocs(const std::string & varName) const;

    /// \brief return number of locations
    std::size_t frameNumLocs() const {return nlocs_;}

    /// \brief return number of records
    std::size_t frameNumRecs() const {return nrecs_;}

    /// \brief return number of locations that were selected from ObsIo
    Dimensions_t globalNumLocs() const {return gnlocs_;}

    /// \brief return number of locations from obs source that were outside the time window
    Dimensions_t globalNumLocsOutsideTimeWindow() const {return gnlocs_outside_timewindow_;}

    /// \brief return number of locations from backend
    std::size_t backendNumLocs() const {return backend_nlocs_;}

    /// \brief return number of variables from backend
    std::size_t backendNumVars() const {return backend_var_list_.size();}

    /// \brief return number of dimension variables from backend
    std::size_t backendNumDimVars() const {return backend_dim_var_list_.size();}

    /// \brief return number of maximum variable size (along first dimension) from backend
    Dimensions_t backendMaxVarSize() const {return backend_max_var_size_;}

    /// \brief return the backend (not the frame) obs group
    ObsGroup backendObsGroup() const {return obs_data_in_->getObsGroup();}

    /// \brief return list of indices indicating which locations were selected from ObsIo
    virtual std::vector<std::size_t> index() const {return std::vector<std::size_t>{};}

    /// \brief return list of record numbers from ObsIo
    virtual std::vector<std::size_t> recnums() const {return std::vector<std::size_t>{};}

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

    /// \brief create selection object for accessing a memory buffer
    /// \param varShape dimension sizes for variable being transferred
    /// \param frameCount size of current frame
    Selection createMemSelection(const std::vector<Dimensions_t> & varShape,
                                 const Dimensions_t frameCount);

    /// \brief create selection object for accessing the entire frame variable
    /// \param varShape dimension sizes for variable being transferred
    /// \param frameCount size of current frame
    Selection createEntireFrameSelection(const std::vector<Dimensions_t> & varShape,
                                         const Dimensions_t frameCount);

    /// \brief create selection object for accessing a frame from a whole variable
    /// \param varShape dimension sizes for variable being transferred
    /// \param frameStart start of current frame for current variable
    /// \param frameCount size of current frame
    Selection createVarSelection(const std::vector<Dimensions_t> & varShape,
                                 const Dimensions_t frameStart,
                                 const Dimensions_t frameCount);

 protected:
    //------------------ protected data members ------------------------------
    /// \brief ioda engines backend for reading obs data
    std::unique_ptr<Engines::ReaderBase> obs_data_in_;

    /// \brief ObsGroup object (temporary storage for a single frame)
    ObsGroup obs_frame_;

    /// \brief number of records from frame
    Dimensions_t nrecs_;

    /// \brief number of locations from frame
    Dimensions_t nlocs_;

    /// \brief total number of locations from source (file or generator) that were
    ///        selected after the timing window filtering
    Dimensions_t gnlocs_;

    /// \brief number of nlocs from the file (gnlocs) that are outside the time window
    Dimensions_t gnlocs_outside_timewindow_;

    /// \brief number of locations from backend
    Dimensions_t backend_nlocs_;

    /// \brief ObsIo parameter specs
    ObsSpaceParameters params_;

    /// \brief maximum frame size
    Dimensions_t max_frame_size_;

    /// \brief maximum variable size
    Dimensions_t max_var_size_;

    /// \brief maximum variable size
    Dimensions_t backend_max_var_size_;

    /// \brief current frame starting index
    Dimensions_t frame_start_;

    /// \brief true if the backend contains an epoch style datetime variable
    bool use_epoch_datetime_;

    /// \brief true if the backend contains a string style datetime variable
    bool use_string_datetime_;

    /// \brief true if the backend contains an offset style datetime variable
    bool use_offset_datetime_;

    /// \brief list of regular variables from the frame
    VarUtils::Vec_Named_Variable var_list_;

    /// \brief list of dimension scale variables from the frame
    VarUtils::Vec_Named_Variable dim_var_list_;

    /// \brief map containing variables with their attached dimension scales from the frame
    VarUtils::VarDimMap dims_attached_to_vars_;

    /// \brief list of regular variables from the backend (file or generator)
    VarUtils::Vec_Named_Variable backend_var_list_;

    /// \brief list of dimension scale variables from the backend
    VarUtils::Vec_Named_Variable backend_dim_var_list_;

    /// \brief map containing variables with their attached dimension scales from the backend
    VarUtils::VarDimMap backend_dims_attached_to_vars_;

    /// \brief names of variables to be used to group observations into records
    std::vector<std::string> obs_grouping_vars_;

    //------------------ protected functions ----------------------------------

    /// \brief internal implementation of isVarDimByNlocs
    /// \details This function checks to see if the first dimension of the given variable
    /// (varName) is the nlocs dimension, and if so returns true.
    /// \param varName Name of variable to check
    /// \param varDimMap Map with each variable and its associated dimensions
    bool isVarDimByNlocs_Impl(const std::string & varName,
                              const VarUtils::VarDimMap & varDimMap) const;

    /// \brief create selection object for accessing an ObsIo variable
    /// \param varShape dimension sizes for variable being transferred
    /// \param frameStart starting index number for current frame
    /// \param frameCount size of current frame
    Selection createObsIoSelection(const std::vector<Dimensions_t> & varShape,
                                   const Dimensions_t frameStart,
                                   const Dimensions_t frameCount);

    /// \brief create a frame object based on dimensions and variables from a source ObsGroup
    /// \details This function is used to set up a temprary ObsGroup based frame in memory
    ///          which is to be used for processing and transferring data between ObsIo
    ///          and ObsSpace variables. The two parameters dimVarList and varDimMap can
    ///          be created with VarUtils::collectVarDimInfo() in VarUtils.h.
    /// \param varList source ObsGroup list of regular variables
    /// \param dimVarList source ObsGroup list of dimension variable names
    /// \param varDimMap source ObsGroup map showing variables with associated dimensions
    void createFrameFromObsGroup(const VarUtils::Vec_Named_Variable & varList,
                                 const VarUtils::Vec_Named_Variable & dimVarList,
                                 const VarUtils::VarDimMap & varDimMap);

    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;
};

}  // namespace ioda

#endif  // IO_OBSFRAME_H_
