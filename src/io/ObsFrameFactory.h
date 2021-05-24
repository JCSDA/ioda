/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFRAMEFACTORY_H_
#define IO_OBSFRAMEFACTORY_H_

#include <memory>

#include "ioda/io/ObsFrame.h"

namespace ioda {

class Distribution;
class ObsSpaceParameters;

/*!
 * \brief Factory class to instantiate objects of ObsFrame subclasses.
 *
 * \author Stephen Herbener (JCSDA)
 */

class ObsFrameFactory {
 public:
    ObsFrameFactory() { }
    ~ObsFrameFactory() { }

    /// \brief factory function to create an object of an ObsFrame subclass
    /// \param mode Whether to create a frame for reading or writing.
    /// \param params Additional configuration parameters.
    /// \param dist Distributes observations across MPI processes.
    static std::shared_ptr<ObsFrame> create(const ObsIoModes mode,
                                            const ObsSpaceParameters & params,
                                            const std::shared_ptr<Distribution> & dist);
};

}  // namespace ioda

#endif  // IO_OBSFRAMEFACTORY_H_
