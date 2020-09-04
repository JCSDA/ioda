/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOFACTORY_H_
#define IO_OBSIOFACTORY_H_

#include "eckit/mpi/Comm.h"

#include "ioda/io/ObsIo.h"
#include "ioda/ObsSpaceParameters.h"

namespace ioda {

/*!
 * \brief Factory class to instantiate objects of ObsIo subclasses.
 *
 * \author Stephen Herbener (JCSDA)
 */

class ObsIoFactory {
 public:
    ObsIoFactory() { }
    ~ObsIoFactory() { }

    /// \brief factory function to create an object of an ObsIo subclass
    /// \param config Eckit local configuration ojbect containing specs for construction
    static std::shared_ptr<ObsIo> create(const ObsIoActions action, const ObsIoModes mode,
                                         const ObsSpaceParameters & params);
};

}  // namespace ioda

#endif  // IO_OBSIOFACTORY_H_
