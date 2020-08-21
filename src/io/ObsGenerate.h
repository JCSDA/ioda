/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSGENERATE_H_
#define IO_OBSGENERATE_H_

#include <iostream>

#include "ioda/io/ObsIo.h"
#include "ioda/io/ObsIoParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"

////////////////////////////////////////////////////////////////////////
// ObsIo subclass for a YAML generator
////////////////////////////////////////////////////////////////////////

namespace ioda {

/*! \brief Implementation of ObsIo for a YAML generator.
 *
 * \author Stephen Herbener (JCSDA)
 */
class ObsGenerate : public ObsIo, private util::ObjectCounter<ObsGenerate> {
    public:
        /// \brief classname method for object counter
        ///
        /// \details This method is supplied for the ObjectCounter base class.
        ///          It defines a name to identify an object of this class
        ///          for reporting by OOPS.
        static const std::string classname() {return "ioda::ObsFile";}

        ObsGenerate(const ObsIoActions action, const ObsIoModes mode, const ObsIoParameters & params);
        ~ObsGenerate();

    private:
        /// \brief print routine for oops::Printable base class
        void print(std::ostream & os) const override;

        ObsGroup obs_group_;
};

}  // namespace ioda

#endif  // IO_OBSGENERATE_H_
