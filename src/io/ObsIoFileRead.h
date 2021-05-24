/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOFILEREAD_H_
#define IO_OBSIOFILEREAD_H_

#include <iostream>

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsIo.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"

namespace ioda {

/*! \brief Implementation of ObsIo reading data from a file.
 *
 * \author Stephen Herbener (JCSDA)
 */
class ObsIoFileRead : public ObsIo, private util::ObjectCounter<ObsIoFileRead> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsIoFileRead";}

    /// Type used by ObsIoFactory
    typedef ObsFileInParameters Parameters_;

    explicit ObsIoFileRead(const Parameters_ & ioParams,
                           const ObsSpaceParameters & obsSpaceParams);
    ~ObsIoFileRead();

 private:
    //-------------------------- private data members -------------------------------

    //-------------------------- private functions ----------------------------------
    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;
};

}  // namespace ioda

#endif  // IO_OBSIOFILEREAD_H_
