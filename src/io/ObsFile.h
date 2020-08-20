/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSFILE_H_
#define IO_OBSFILE_H_

#include <iostream>

#include "ioda/io/ObsIo.h"
#include "ioda/io/ObsIoParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"

////////////////////////////////////////////////////////////////////////
// ObsIo subclass for a file
////////////////////////////////////////////////////////////////////////

namespace ioda {

/*! \brief Implementation of ObsIo for a file.
 *
 * \author Stephen Herbener (JCSDA)
 */
class ObsFile : public ObsIo, private util::ObjectCounter<ObsFile> {
    public:
        /// \brief classname method for object counter
        ///
        /// \details This method is supplied for the ObjectCounter base class.
        ///          It defines a name to identify an object of this class
        ///          for reporting by OOPS.
        static const std::string classname() {return "ioda::ObsFile";}

        ObsFile(const ObsIoActions action, const ObsIoModes mode, const ObsIoParameters & params);
        ~ObsFile();

    private:
        /// \brief print routine for oops::Printable base class
        void print(std::ostream & os) const override;
};

}  // namespace ioda

#endif  // IO_OBSFILE_H_
