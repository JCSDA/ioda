/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIO_H_
#define IO_OBSIO_H_

#include <iostream>

#include "ioda/io/ObsIoParameters.h"
#include "eckit/config/LocalConfiguration.h"
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
    public:
        ObsIo(const ObsIoActions action, const ObsIoModes mode, const ObsIoParameters & params);
	virtual ~ObsIo() = 0;

    protected:
        /// \brief print() for oops::Printable base class
        virtual void print(std::ostream & os) const = 0;

        /// \brief ObsIo action
        ObsIoActions action_;

        /// \brief ObsIo mode
        ObsIoModes mode_;

        /// \brief ObsIo parameter specs
        ObsIoParameters params_;
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
