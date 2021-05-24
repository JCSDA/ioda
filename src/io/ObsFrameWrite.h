/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFRAMEWRITE_H_
#define IO_OBSFRAMEWRITE_H_

#include "eckit/config/LocalConfiguration.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsFrame.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrameWrite class
/// \details This class manages one frame of obs data (subset of locations) when
///          writing data to an ObsIo object. Currently, this is simply a transfer
///          of data, but in the future this will also manage stitching back the
///          data from multiple MPI tasks into one file.
/// \author Stephen Herbener (JCSDA)

class ObsFrameWrite : public ObsFrame, private util::ObjectCounter<ObsFrameWrite> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsFrameWrite";}

    ObsFrameWrite(const ObsSpaceParameters & params,
                  const std::shared_ptr<Distribution> & dist);

    ~ObsFrameWrite();

    /// \brief initialize for walking through the frames
    /// \param varList source ObsGroup list of regular variables
    /// \param dimVarList source ObsGroup list of dimension variable names
    /// \param varDimMap source ObsGroup map showing variables with associated dimensions
    /// \param maxVarSize source ObsGroup maximum variable size along the first dimension
    void frameInit(const VarNameObjectList & varList,
                   const VarNameObjectList & dimVarList,
                   const VarDimMap & varDimMap, const Dimensions_t maxVarSize) override;

    /// \brief move to the next frame
    void frameNext(const VarNameObjectList & varList) override;

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

    /// \brief read a frame variable
    /// \details These are here for completeness. There is no reading of the frame
    ///          for the ObsFrameWrite subclass, but it works a little better to
    ///          have these here so the subclass can be identified in an error message.
    /// \param varName variable name
    /// \param varData varible data
    bool readFrameVar(const std::string & varName, std::vector<int> & varData) override;
    bool readFrameVar(const std::string & varName, std::vector<float> & varData) override;
    bool readFrameVar(const std::string & varName, std::vector<std::string> & varData) override;

    /// \brief write a frame variable
    /// \details This function reuquires the caller to allocate the proper amount of
    ///          memory for the intput vector varData.
    ///          The following signatures are for different variable data types.
    /// \param varName variable name
    /// \param varData varible data
    void writeFrameVar(const std::string & varName,
                       const std::vector<int> & varData) override;
    void writeFrameVar(const std::string & varName,
                       const std::vector<float> & varData) override;
    void writeFrameVar(const std::string & varName,
                       const std::vector<std::string> & varData) override;

 private:
    //------------------ private data members ------------------------------

    //--------------------- private functions ------------------------------

    /// \brief create set of variables from source variables and lists
    /// \param srcVarContainer Has_Variables object from source
    /// \param destVarContainer Has_Variables object from destination
    /// \param dimsAttachedToVars Map containing list of attached dims for each variable
    void createObsIoVariables(const Has_Variables & srcVarContainer,
                              Has_Variables & destVarContainer,
                              const VarDimMap & dimsAttachedToVars);

    /// \brief set up frontend and backend selection objects for the given variable
    /// \param varName ObsGroup variable name
    /// \param feSelect Front end selection object
    /// \param beSelect Back end selection object
    void createFrameSelection(const std::string & varName, Selection & feSelect,
                              Selection & beSelect);

    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;

    /// \brief write variable data into frame helper function
    /// \param varName variable name
    /// \param varData varible data
    template<typename DataType>
    void writeFrameVarHelper(const std::string & varName,
                             const std::vector<DataType> & varData) {
        Dimensions_t frameCount = this->frameCount(varName);
        if (frameCount > 0) {
            Variable frameVar = obs_frame_.vars.open(varName);
            std::vector<Dimensions_t> varShape = frameVar.getDimensions().dimsCur;

            // Form the selection objects for this variable
            Selection varDataSelect = createMemSelection(varShape, frameCount);
            Selection frameSelect = createEntireFrameSelection(varShape, frameCount);

            // Write the data into the frame
            frameVar.write<DataType>(varData, varDataSelect, frameSelect);
        }
    }
};

}  // namespace ioda

#endif  // IO_OBSFRAMEWRITE_H_
