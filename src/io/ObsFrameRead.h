/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFRAMEREAD_H_
#define IO_OBSFRAMEREAD_H_

#include "eckit/config/LocalConfiguration.h"

#include "ioda/io/ObsFrame.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrameRead class
/// \author Stephen Herbener (JCSDA)

class ObsFrameRead : public ObsFrame {
 public:
    ObsFrameRead(const ObsIoActions action, const ObsIoModes mode,
                 const ObsSpaceParameters & params);

    ~ObsFrameRead();

 private:

};

}  // namespace ioda

#endif  // IO_OBSFRAMEREAD_H_
