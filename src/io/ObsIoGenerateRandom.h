/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOGENERATERANDOM_H_
#define IO_OBSIOGENERATERANDOM_H_

#include <iostream>

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsIo.h"
#include "ioda/ObsSpaceParameters.h"

#include "eckit/mpi/Comm.h"

#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/ObjectCounter.h"

namespace ioda {

/*! \brief Implementation of ObsIo generating observations at random locations.
 *
 * \author Stephen Herbener (JCSDA)
 */
class ObsIoGenerateRandom : public ObsIo, private util::ObjectCounter<ObsIoGenerateRandom> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsIoGenerateRandom";}

    /// Type used by ObsIoFactory
    typedef ObsGenerateRandomParameters Parameters_;

    explicit ObsIoGenerateRandom(const Parameters_ &ioParams,
                                 const ObsSpaceParameters & obsSpaceParams);
    ~ObsIoGenerateRandom();

    bool applyLocationsCheck() const override { return false; }

 private:
    //-------------------------- private data members -------------------------------

    //-------------------------- private functions ----------------------------------
    /// \brief generate observation locations using the random method
    /// \details This method will generate a set of latitudes and longitudes of which
    ///          can be used for testing without reading in an obs file. Two latitude
    ///          values, two longitude values, the number of locations (nobs keyword)
    ///          and an optional random seed are specified in the configuration given
    ///          by the conf parameter. Random locations between the two latitudes and
    ///          two longitudes are generated and stored in the obs container as meta data.
    ///          Random time stamps that fall inside the given timing window (which is
    ///          specified in the configuration file) are also generated and stored
    ///          in the obs container as meta data. These data are intended for use
    ///          with the MakeObs functionality.
    /// \param params Parameters structure specific to the generate random method
    /// \param winStart DateTime object marking DA window start
    /// \param winEnd DateTime object marking DA window end
    /// \param comm MPI communicator group
    /// \param obsErrors list of error estimates for each assimilated variable
    /// \param simVarNames list of names of variables to be assimilated
    void genDistRandom(const EmbeddedObsGenerateRandomParameters & params,
                       const util::DateTime & winStart, const util::DateTime & winEnd,
                       const eckit::mpi::Comm & comm,
                       const std::vector<float> & obsErrors,
                       const std::vector<std::string> & simVarNames);

    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;
};

}  // namespace ioda

#endif  // IO_OBSIOGENERATERANDOM_H_
