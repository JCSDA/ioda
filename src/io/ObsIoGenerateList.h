/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOGENERATELIST_H_
#define IO_OBSIOGENERATELIST_H_

#include <iostream>

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsIo.h"
#include "ioda/ObsSpaceParameters.h"

#include "eckit/mpi/Comm.h"

#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"

namespace ioda {

/*! \brief Implementation of ObsIo generating observations at locations specified in the
 *  input YAML file (parsed earlier into the ObsGenerateListParameters object passed
 *  to the constructor).
 *
 * \author Stephen Herbener (JCSDA)
 */
class ObsIoGenerateList : public ObsIo, private util::ObjectCounter<ObsIoGenerateList> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsIoGenerateList";}

    /// Type used by ObsIoFactory
    typedef ObsGenerateListParameters Parameters_;

    explicit ObsIoGenerateList(const Parameters_ &ioParams,
                               const ObsSpaceParameters & obsSpaceParams);
    ~ObsIoGenerateList();

    bool applyTimingWindow() const override { return false; }

 private:
    //-------------------------- private data members -------------------------------

    //-------------------------- private functions ----------------------------------
    /// \brief generate observation locations using the list method
    /// \details This method will generate a set of latitudes and longitudes of which
    ///          can be used for testing without reading in an obs file. The values
    ///          are simply read from lists in the configuration file. The purpose of
    ///          this method is to allow the user to exactly specify obs locations.
    ///          these data are intended for use with the MakeObs functionality.
    /// \param params Parameters structure specific to the generate list method
    /// \param obsErrors list of error estimates for each assimilated variable
    /// \param simVarNames list of names of variables to be assimilated
    void genDistList(const EmbeddedObsGenerateListParameters & params,
                     const std::vector<float> & obsErrors,
                     const std::vector<std::string> & simVarNames);

    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;
};

}  // namespace ioda

#endif  // IO_OBSIOGENERATELIST_H_
