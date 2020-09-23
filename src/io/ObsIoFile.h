/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOFILE_H_
#define IO_OBSIOFILE_H_

#include <iostream>

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsIo.h"
#include "ioda/ObsSpaceParameters.h"

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
class ObsIoFile : public ObsIo, private util::ObjectCounter<ObsIoFile> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsIoFile";}

    ObsIoFile(const ObsIoActions action, const ObsIoModes mode,
            const ObsSpaceParameters & params);
    ~ObsIoFile();

 private:
    //-------------------------- private data members -------------------------------

    //-------------------------- private functions ----------------------------------
    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;
};

}  // namespace ioda

#endif  // IO_OBSIOFILE_H_
