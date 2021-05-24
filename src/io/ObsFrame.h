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
#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsIo.h"
#include "ioda/ObsGroup.h"
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
    ObsFrame(const ObsSpaceParameters & params, const std::shared_ptr<Distribution> & dist);
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
    const VarNameObjectList & ioVarList() const {return obs_io_->varList();}

    /// \brief return list of dimension scale variables from ObsIo
    const VarNameObjectList & ioDimVarList() const {return obs_io_->dimVarList();}

    /// \brief return map from variables to their attached dimension scales
    VarDimMap ioVarDimMap() const {return obs_io_->varDimMap();}

    /// \brief update variable, dimension info in the ObsIo object
    void ioUpdateVarDimInfo() const {obs_io_->updateVarDimInfo();}

    /// \brief return true if variable is dimensioned by nlocs
    bool ioIsVarDimByNlocs(const std::string & varName) const {
        return obs_io_->isVarDimByNlocs(varName);
    }

    /// \brief return number of locations
    virtual std::size_t frameNumLocs() const {return nlocs_;}

    /// \brief return number of records
    virtual std::size_t frameNumRecs() const {return nrecs_;}

    /// \brief return number of locations that were selected from ObsIo
    Dimensions_t globalNumLocs() const {return gnlocs_;}

    /// \brief return number of locations from obs source that were outside the time window
    Dimensions_t globalNumLocsOutsideTimeWindow() const {return gnlocs_outside_timewindow_;}

    /// \brief return list of indices indicating which locations were selected from ObsIo
    virtual std::vector<std::size_t> index() const {return std::vector<std::size_t>{};}

    /// \brief return list of record numbers from ObsIo
    virtual std::vector<std::size_t> recnums() const {return std::vector<std::size_t>{};}

    /// \brief initialize frame for a read frame object
    virtual void frameInit() {}

    /// \brief initialize for a write frame object
    /// \param varList source ObsGroup list of regular variables
    /// \param dimVarList source ObsGroup list of dimension variables
    /// \param varDimMap source ObsGroup map showing variables with associated dimensions
    /// \param maxVarSize source ObsGroup maximum variable size along the first dimension
    virtual void frameInit(const VarNameObjectList & varList,
                           const VarNameObjectList & varDimList,
                           const VarDimMap & varDimMap, const Dimensions_t maxVarSize) {}

    /// \brief move to the next frame for a read frame object
    virtual void frameNext() {}

    /// \brief move to the next frame for a write frame object
    /// \param varList source ObsGroup list variable names
    virtual void frameNext(const VarNameObjectList & varList) {}

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

    /// \brief read a frame variable
    /// \details It's possible for some variables to not be included in the
    ///          read because the frame has gone past their ending index.
    ///          Therefore, this function will return true when there exists
    ///          more data available for the variable in the frame.
    ///          This function will allocate the proper amount of memory for the
    ///          output vector varData.
    ///          The following signatures are for different variable data types.
    /// \param varName variable name
    /// \param varData varible data
    virtual bool readFrameVar(const std::string & varName, std::vector<int> & varData) = 0;
    virtual bool readFrameVar(const std::string & varName, std::vector<float> & varData) = 0;
    virtual bool readFrameVar(const std::string & varName,
                              std::vector<std::string> & varData) = 0;

    /// \brief write a frame variable
    /// \details This function reuquires the caller to allocate the proper amount of
    ///          memory for the intput vector varData.
    ///          The following signatures are for different variable data types.
    /// \param varName variable name
    /// \param varData varible data
    virtual void writeFrameVar(const std::string & varName,
                               const std::vector<int> & varData) = 0;
    virtual void writeFrameVar(const std::string & varName,
                               const std::vector<float> & varData) = 0;
    virtual void writeFrameVar(const std::string & varName,
                               const std::vector<std::string> & varData) = 0;

 protected:
    //------------------ protected data members ------------------------------
    /// \brief ObsIo object
    std::shared_ptr<ObsIo> obs_io_;

    /// \brief ObsGroup object (temporary storage for a single frame)
    ObsGroup obs_frame_;

    /// \brief MPI distribution object
    std::shared_ptr<Distribution> dist_;

    /// \brief number of records from source (file or generator)
    Dimensions_t nrecs_;

    /// \brief number of locations from source (file or generator)
    Dimensions_t nlocs_;

    /// \brief total number of locations from source (file or generator) that were
    ///        selected after the timing window filtering
    Dimensions_t gnlocs_;

    /// \brief number of nlocs from the file (gnlocs) that are outside the time window
    Dimensions_t gnlocs_outside_timewindow_;

    /// \brief ObsIo parameter specs
    ObsSpaceParameters params_;

    /// \brief maximum frame size
    Dimensions_t max_frame_size_;

    /// \brief maximum variable size
    Dimensions_t max_var_size_;

    /// \brief current frame starting index
    Dimensions_t frame_start_;

    //------------------ protected functions ----------------------------------

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
    ///          be created with IodaUtils::collectVarDimInfo() in IodaUtils.h.
    /// \param varList source ObsGroup list of regular variables
    /// \param dimVarList source ObsGroup list of dimension variable names
    /// \param varDimMap source ObsGroup map showing variables with associated dimensions
    void createFrameFromObsGroup(const VarNameObjectList & varList,
                                 const VarNameObjectList & dimVarList,
                                 const VarDimMap & varDimMap);

    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;
};

}  // namespace ioda

#endif  // IO_OBSFRAME_H_
