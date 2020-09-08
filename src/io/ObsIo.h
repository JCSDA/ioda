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

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

////////////////////////////////////////////////////////////////////////
// Base class for observation data IO
////////////////////////////////////////////////////////////////////////

namespace ioda {

/// \brief Implementation of ObsIo base class
/// \author Stephen Herbener (JCSDA)

class ObsIo : public util::Printable {
 public:
    /// \brief ObsGroup object representing io source/destination
    ObsGroup obs_group_;

    ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsSpaceParameters & params);
    virtual ~ObsIo() = 0;

    /// \brief return number of locations from the source
    std::size_t nlocs() const {return nlocs_;}

    /// \brief return number of locations from the source
    std::size_t nrecs() const {return unique_rec_nums_.size();}

    /// \brief return adjusted nlocs frame start
    std::size_t adjNlocsFrameStart() const {return adjusted_nlocs_frame_start_;}

    /// \brief return adjusted nlocs frame count
    std::size_t adjNlocsFrameCount() const {return adjusted_nlocs_frame_count_;}

    /// \brief return vector with results of timing window filtering and MPI distribution
    std::vector<std::size_t> index() const {return indx_;}

    /// \brief return vector with record numbering
    std::vector<std::size_t> recnums() const {return recnums_;}

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

    /// \brief generate frame indices and corresponding record numbers
    /// \details This method generates a list of indices with their corresponding
    ///  record numbers, where the indices denote which locations are to be
    ///  read into this process element.
    virtual void genFrameIndexRecNums(std::shared_ptr<Distribution> & dist) = 0;

    /// \details return true if observation is inside the DA timing window.
    /// \param obsDt Observation date time object
    bool insideTimingWindow(const util::DateTime & ObsDt);

 protected:
    //------------------ protected data members ------------------------------
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

    /// \brief number of records from source (file or generator)
    std::size_t nrecs_;

    /// \brief ObsFrame object for generating variable selection objects
    ObsFrame obs_frame_;

    /// \brief current frame start for variable dimensioned along nlocs
    /// \details This data member is keeping track of the frame start for
    /// the contiguous storage where the obs source data will be moved to.
    /// Note that the start_ data member is keeping track of the frame start
    /// for the obs source itself.
    Dimensions_t adjusted_nlocs_frame_start_;

    /// \brief current frame count for variable dimensioned along nlocs
    Dimensions_t adjusted_nlocs_frame_count_;

    /// \brief indexes of locations to extract from the input obs file
    std::vector<std::size_t> indx_;

    /// \brief record numbers associated with the location indexes
    std::vector<std::size_t> recnums_;

    /// \brief maps for obs grouping via integer, float or string values
    std::map<int, std::size_t> int_obs_grouping_;
    std::map<float, std::size_t> float_obs_grouping_;
    std::map<std::string, std::size_t> string_obs_grouping_;

    /// \brief next available record number
    std::size_t next_rec_num_;

    /// \brief unique record numbers
    std::set<std::size_t> unique_rec_nums_;

    /// \brief location indices for current frame
    std::vector<Dimensions_t> frame_loc_index_;

    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;

    /// \brief generate indices for all locations in current frame
    /// \param locIndex vector of location indices relative to entire obs source
    /// \param frameIndex vector of location indices relative to current frame
    void genFrameLocationsAll(std::vector<Dimensions_t> & locIndex,
                              std::vector<Dimensions_t> & frameIndex);

    /// \brief generate indices for locations in current frame after filtering out
    ///  obs outside DA timing window
    /// \param locIndex vector of location indices relative to entire obs source
    /// \param frameIndex vector of location indices relative to current frame
    void genFrameLocationsTimeWindow(std::vector<Dimensions_t> & locIndex,
                                     std::vector<Dimensions_t> & frameIndex);

    /// \brief generate record numbers where each location is a unique record (no grouping)
    /// \param locIndex vector containing location indices
    /// \param records vector indexed by location containing the record numbers
    void genRecordNumbersAll(const std::vector<Dimensions_t> & locIndex,
                             std::vector<Dimensions_t> & records);

    /// \brief generate record numbers considering obs grouping
    /// \param obsGroupVarName variable controlling the grouping function
    /// \param frameIndex vector containing frame location indices
    /// \param records vector indexed by location containing the record numbers
    void genRecordNumbersGrouping(const std::string & obsGroupVarName,
                                  const std::vector<Dimensions_t> & frameIndex,
                                  std::vector<Dimensions_t> & records);

    /// \brief apply MPI distribution
    /// \param dist ioda::Distribution object
    /// \param records vector indexed by location containing the record numbers
    void applyMpiDistribution(const std::shared_ptr<Distribution> & dist,
                              const std::vector<Dimensions_t> & locIndex,
                              const std::vector<Dimensions_t> & records);
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
