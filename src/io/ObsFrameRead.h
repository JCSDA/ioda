/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFRAMEREAD_H_
#define IO_OBSFRAMEREAD_H_

#include "eckit/config/LocalConfiguration.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrameRead class
/// \author Stephen Herbener (JCSDA)

class ObsFrameRead : public ObsFrame, private util::ObjectCounter<ObsFrameRead> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsFrameRead";}

    ObsFrameRead(const ObsIoActions action, const ObsIoModes mode,
                 const ObsSpaceParameters & params);

    ~ObsFrameRead();

    /// \brief return list of indices indicating which locations were selected from ObsIo
    std::vector<std::size_t> index() const override {return indx_;}

    /// \brief return list of record numbers from ObsIo
    std::vector<std::size_t> recnums() const override {return recnums_;}

    /// \brief initialize for walking through the frames
    void frameInit() override;

    /// \brief move to the next frame
    void frameNext() override;

    /// \brief true if a frame is available (not past end of frames)
    bool frameAvailable() override;

    /// \brief return current frame starting index
    /// \param varName name of variable
    Dimensions_t frameStart() override;

    /// \brief return current frame count for variable
    /// \details Variables can be of different sizes so it's possible that the
    /// frame has moved past the end of some variables but not so for other
    /// variables. When the frame is past the end of the given variable, this
    /// routine returns a zero to indicate that we're done with this variable.
    /// \param varName variable name
    Dimensions_t frameCount(const std::string & varName) override;

    /// \brief return adjusted nlocs frame start
    Dimensions_t adjNlocsFrameStart() const override {return adjusted_nlocs_frame_start_;}

    /// \brief return adjusted nlocs frame count
    Dimensions_t adjNlocsFrameCount() const override {return adjusted_nlocs_frame_count_;}

    /// \brief generate frame indices and corresponding record numbers
    /// \details This method generates a list of indices with their corresponding
    ///  record numbers, where the indices denote which locations are to be
    ///  read into this process element.
    void genFrameIndexRecNums(std::shared_ptr<Distribution> & dist) override;

    /// \brief set up frontend and backend selection objects for the given variable
    /// \param varName ObsGroup variable name
    /// \param feSelect Front end selection object
    /// \param beSelect Back end selection object
    void createFrameSelection(const std::string & varName, Selection & feSelect,
                              Selection & beSelect) override;

 private:
    //------------------ private data members ------------------------------
    /// \brief current frame start for variable dimensioned along nlocs
    /// \details This data member is keeping track of the frame start for
    /// the contiguous storage where the obs source data will be moved to.
    /// Note that the start_ data member is keeping track of the frame start
    /// for the obs source itself.
    Dimensions_t adjusted_nlocs_frame_start_;

    /// \brief current frame count for variable dimensioned along nlocs
    Dimensions_t adjusted_nlocs_frame_count_;

    /// \brief maps for obs grouping via integer, float or string values
    std::map<int, std::size_t> int_obs_grouping_;
    std::map<float, std::size_t> float_obs_grouping_;
    std::map<std::string, std::size_t> string_obs_grouping_;

    /// \brief indexes of locations to extract from the input obs file
    std::vector<std::size_t> indx_;

    /// \brief record numbers associated with the location indexes
    std::vector<std::size_t> recnums_;

    /// \brief next available record number
    std::size_t next_rec_num_;

    /// \brief unique record numbers
    std::set<std::size_t> unique_rec_nums_;

    /// \brief location indices for current frame
    std::vector<Dimensions_t> frame_loc_index_;

    //--------------------- private functions ------------------------------
    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;

    /// \brief return current frame count for variable
    /// \details Variables can be of different sizes so it's possible that the
    /// frame has moved past the end of some variables but not so for other
    /// variables. When the frame is past the end of the given variable, this
    /// routine returns a zero to indicate that we're done with this variable.
    /// \param var variable
    Dimensions_t basicFrameCount(const Variable & var);

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

    /// \details return true if observation is inside the DA timing window.
    /// \param obsDt Observation date time object
    bool insideTimingWindow(const util::DateTime & ObsDt);
};

}  // namespace ioda

#endif  // IO_OBSFRAMEREAD_H_
