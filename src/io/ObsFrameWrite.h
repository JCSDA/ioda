/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFRAMEWRITE_H_
#define IO_OBSFRAMEWRITE_H_

#include "eckit/config/LocalConfiguration.h"

#include "ioda/io/ObsFrame.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrameWrite class
/// \author Stephen Herbener (JCSDA)

class ObsFrameWrite : public ObsFrame, private util::ObjectCounter<ObsFrameWrite> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsFrameWrite";}

    ObsFrameWrite(const ObsIoActions action, const ObsIoModes mode,
                  const ObsSpaceParameters & params);

    ~ObsFrameWrite();

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
    /// \param var variable
    Dimensions_t frameCount(const Variable & var) override;

    /// \brief set up frontend and backend selection objects for the given variable
    /// \param var ObsGroup variable
    /// \param feSelect Front end selection object
    /// \param beSelect Back end selection object
    void createFrameSelection(const Variable & var, Selection & feSelect,
                              Selection & beSelect) override;

 private:
    //------------------ private data members ------------------------------

    //--------------------- private functions ------------------------------
    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;
};

}  // namespace ioda

#endif  // IO_OBSFRAMEWRITE_H_
