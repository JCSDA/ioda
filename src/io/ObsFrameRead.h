/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFRAMEREAD_H_
#define IO_OBSFRAMEREAD_H_

#include <vector>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/VarUtils.h"

#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrameRead class
/// \details This class manages one frame of obs data (subset of locations) when
///          reading data from an ObsIo object. This includes reading the frame,
///          filtering out obs that are outside the DA timing window, generating record
///          numbers, applying obs grouping (optional) and applying the MPI distribution.
/// \author Stephen Herbener (JCSDA)

class ObsFrameRead : public ObsFrame, private util::ObjectCounter<ObsFrameRead> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsFrameRead";}

    explicit ObsFrameRead(const ObsSpaceParameters & params);

    ~ObsFrameRead();

    /// \brief return list of indices indicating which locations were selected from ObsIo
    std::vector<std::size_t> index() const override {return indx_;}

    /// \brief return list of record numbers from ObsIo
    std::vector<std::size_t> recnums() const override {return recnums_;}

    /// \brief initialize for walking through the frames
    /// \param destAttrs destination ObsGroup global attributes container
    void frameInit(Has_Attributes & destAttrs);

    /// \brief move to the next frame
    void frameNext();

    /// \brief true if a frame is available (not past end of frames)
    bool frameAvailable();

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
    /// \param varDataSelect selection information for the selection in memory
    /// \param frameSelect selection information for the selection in frame
    bool readFrameVar(const std::string & varName, std::vector<int> & varData);
    bool readFrameVar(const std::string & varName, std::vector<int64_t> & varData);
    bool readFrameVar(const std::string & varName, std::vector<float> & varData);
    bool readFrameVar(const std::string & varName, std::vector<std::string> & varData);
    bool readFrameVar(const std::string & varName, std::vector<char> & varData);

    /// \brief return the MPI distribution
    std::shared_ptr<const Distribution> distribution() {return dist_;}

 private:
    //------------------ private data members ------------------------------

    /// \brief MPI distribution object
    std::shared_ptr<Distribution> dist_;

    /// \brief true if the backend produces a different series of observations on each process,
    /// false if they are all the same
    bool each_process_reads_separate_obs_;

    /// \Brief Distribution Name
    std::string distname_;

    /// \brief current frame start for variable dimensioned along nlocs
    /// \details This data member is keeping track of the frame start for
    /// the contiguous storage where the obs source data will be moved to.
    /// Note that the start_ data member is keeping track of the frame start
    /// for the obs source itself.
    Dimensions_t adjusted_nlocs_frame_start_;

    /// \brief current frame count for variable dimensioned along nlocs
    Dimensions_t adjusted_nlocs_frame_count_;

    /// \brief map for obs grouping via string keys
    std::map<std::string, std::size_t> obs_grouping_;

    /// \brief indexes of locations to extract from the input obs file
    std::vector<std::size_t> indx_;

    /// \brief record numbers associated with the location indexes
    std::vector<std::size_t> recnums_;

    /// \brief next available record number
    std::size_t next_rec_num_;

    /// \brief spacing between record numbers assigned on this process.
    ///
    /// Normally 1, but if each process reads observations from a different file, then set to
    /// the size of the MPI communicator to ensure record numbers assigned by different processes
    /// are distinct.
    std::size_t rec_num_increment_;

    /// \brief unique record numbers
    std::set<std::size_t> unique_rec_nums_;

    /// \brief location indices for current frame
    std::vector<Dimensions_t> frame_loc_index_;

    /// \brief cache for frame selection
    std::map<VarUtils::Vec_Named_Variable, Selection> known_frame_selections_;

    /// \brief cache for memory buffer selection
    std::map<VarUtils::Vec_Named_Variable, Selection> known_mem_selections_;

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

    /// \brief set up frontend and backend selection objects for the given variable
    /// \param varShape dimension sizes for variable being transferred
    Selection createIndexedFrameSelection(const std::vector<Dimensions_t> & varShape);

    /// \brief generate frame indices and corresponding record numbers
    /// \details This method generates a list of indices with their corresponding
    ///  record numbers, where the indices denote which locations are to be
    ///  read into this process element.
    void genFrameIndexRecNums(std::shared_ptr<Distribution> & dist);

    /// \brief generate indices for all locations in current frame
    /// \param locIndex vector of location indices relative to entire obs source
    /// \param frameIndex vector of location indices relative to current frame
    void genFrameLocationsAll(std::vector<Dimensions_t> & locIndex,
                              std::vector<Dimensions_t> & frameIndex);

    /// \brief generate indices for locations in current frame after filtering out
    ///  obs failing a quality check
    /// \details For now, variables being checked are:
    ///    MetaData/latitude
    ///    MetaData/longitude
    ///    MetaData/datetime
    ///
    /// and the quality checks include:
    ///    locations outside the DA timing window
    ///    locations with missing values
    ///
    /// \param locIndex vector of location indices relative to entire obs source
    /// \param frameIndex vector of location indices relative to current frame
    void genFrameLocationsWithQcheck(std::vector<Dimensions_t> & locIndex,
                                     std::vector<Dimensions_t> & frameIndex);

    /// \brief generate record numbers where each location is a unique record (no grouping)
    /// \param locIndex vector containing location indices
    /// \param records vector indexed by location containing the record numbers
    void genRecordNumbersAll(const std::vector<Dimensions_t> & locIndex,
                             std::vector<Dimensions_t> & records);

    /// \brief generate record numbers considering obs grouping
    /// \param obsGroupVarList list of variables controlling the grouping function
    /// \param frameIndex vector containing frame location indices
    /// \param records vector indexed by location containing the record numbers
    void genRecordNumbersGrouping(const std::vector<std::string> & obsGroupVarList,
                                  const std::vector<Dimensions_t> & frameIndex,
                                  std::vector<Dimensions_t> & records);

    /// \brief generate string keys for record number assignment
    /// \param obsGroupVarList list of variables controlling the grouping function
    /// \param frameIndex vector containing frame location indices
    /// \param groupingKeys vector of keys for the obs grouping map
    void buildObsGroupingKeys(const std::vector<std::string> & obsGroupVarList,
                              const std::vector<Dimensions_t> & frameIndex,
                              std::vector<std::string> & groupingKeys);

    /// \brief apply MPI distribution
    /// \param dist ioda::Distribution object
    /// \param records vector indexed by location containing the record numbers
    void applyMpiDistribution(const std::shared_ptr<Distribution> & dist,
                              const std::vector<Dimensions_t> & locIndex,
                              const std::vector<Dimensions_t> & records);

    /// \details return true if observation is inside the DA timing window.
    /// \param obsDt Observation date time object
    bool insideTimingWindow(const util::DateTime & ObsDt);

    /// \brief read variable data from frame helper function
    /// \param varName variable name
    /// \param varData varible data
    template<typename DataType>
    bool readFrameVarHelper(const std::string & varName, std::vector<DataType> & varData) {
        bool frameVarAvailable;
        Dimensions_t frameCount = this->frameCount(varName);
        if (frameCount > 0) {
            Variable frameVar = obs_frame_.vars.open(varName);
            std::vector<Dimensions_t> varShape = frameVar.getDimensions().dimsCur;

            // Form the selection objects for this variable

            // Check the cache for the selection
            VarUtils::Vec_Named_Variable dims;
            for (auto & ivar : dims_attached_to_vars_) {
                if (ivar.first.name == varName) {
                    dims = ivar.second;
                    break;
                }
            }
            if (!known_mem_selections_.count(dims)) {
                known_mem_selections_[dims] = createMemSelection(varShape, frameCount);
                if (isVarDimByNlocs(varName)) {
                  known_frame_selections_[dims] =
                      createIndexedFrameSelection(varShape);
                } else {
                  known_frame_selections_[dims] =
                      createEntireFrameSelection(varShape, frameCount);
                }
            }
            Selection & memSelect = known_mem_selections_[dims];
            Selection & frameSelect = known_frame_selections_[dims];

            // Read the data into the output varData
            frameVar.read<DataType>(varData, memSelect, frameSelect);

            frameVarAvailable = true;
        } else {
            frameVarAvailable = false;
        }
        return frameVarAvailable;
    }
};

}  // namespace ioda

#endif  // IO_OBSFRAMEREAD_H_
