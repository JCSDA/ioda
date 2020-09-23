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
#include "oops/util/Printable.h"

namespace ioda {

/// \brief Implementation of ObsFrameWrite class
/// \author Stephen Herbener (JCSDA)

class ObsFrameWrite : public ObsFrame {
 public:
    ObsFrameWrite(const ObsIoActions action, const ObsIoModes mode,
                  const ObsSpaceParameters & params);

    ~ObsFrameWrite();

 private:

};

}  // namespace ioda

#endif  // IO_OBSFRAMEWRITE_H_
