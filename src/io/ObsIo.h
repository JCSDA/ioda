/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIO_H_
#define IO_OBSIO_H_

#include <iostream>

#include "eckit/config/LocalConfiguration.h"

#include "ioda/io/ObsIoParameters.h"

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
class ObsIo : public util::Printable {
    private:
        // Container for information about frames.
        struct FrameInfoRec {
          std::size_t start;
          std::size_t size;

          // Constructor
          FrameInfoRec(const std::size_t Start, const std::size_t Size) :
              start(Start), size(Size) {}
        };

        /// \brief frame information map
        /// \details This typedef contains information about the frames in the file
        typedef std::vector<FrameInfoRec> FrameInfo;

        /// \brief frame information vector
        FrameInfo frame_info_;

    public:
        /// \brief ObsGroup object representing io source/destination
        ObsGroup obs_group_;

        ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsIoParameters & params);
	virtual ~ObsIo() = 0;

        void frame_info_init(std::size_t maxVarSize);

    protected:
        /// \brief print() for oops::Printable base class
        /// \param ostream output stream
        virtual void print(std::ostream & os) const = 0;

        /// \brief ObsIo action
        ObsIoActions action_;

        /// \brief ObsIo mode
        ObsIoModes mode_;

        /// \brief ObsIo parameter specs
        ObsIoParameters params_;

        /// \brief maximum frame size
        int max_frame_size_;
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
