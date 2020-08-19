/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIO_H_
#define IO_OBSIO_H_

#include "eckit/config/LocalConfiguration.h"
#include "oops/util/Printable.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for netcdf.
////////////////////////////////////////////////////////////////////////

namespace ioda {

/*! \brief Implementation of ObsIo base class
 *
 * \author Stephen Herbener (JCSDA)
 */
class ObsIo : public util::Printable {
    public:
        ObsIo(const eckit::LocalConfiguration & config) : config_(config) {}
        ~ObsIo();

        /// \brief returns YAML configuration
        eckit::LocalConfiguration config() { return config_; }

    private:
        /// \brief YAML configuration
        const eckit::LocalConfiguration & config_;
};

}  // namespace ioda

#endif  // IO_OBSIO_H_
