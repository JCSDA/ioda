/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/io/ObsFrameFactory.h"

#include "ioda/io/ObsFrameRead.h"
#include "ioda/io/ObsFrameWrite.h"

namespace ioda {

//-------------------------------------------------------------------------------------
std::shared_ptr<ObsFrame> ObsFrameFactory::create(const ObsIoModes mode,
                                                  const ObsSpaceParameters & params,
                                                  const std::shared_ptr<Distribution> & dist) {
    switch (mode) {
    case ObsIoModes::READ:
        return std::make_shared<ObsFrameRead>(params, dist);
    case ObsIoModes::WRITE:
        return std::make_shared<ObsFrameWrite>(params, dist);
    default:
        throw eckit::BadValue("Unknown mode", Here());
    }
}

}  // namespace ioda
