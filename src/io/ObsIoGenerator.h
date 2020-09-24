/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOGENERATOR_H_
#define IO_OBSIOGENERATOR_H_

#include <iostream>

#include "ioda/distribution/Distribution.h"
#include "ioda/io/ObsIo.h"
#include "ioda/ObsSpaceParameters.h"

#include "eckit/mpi/Comm.h"

#include "oops/util/DateTime.h"
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
class ObsIoGenerator : public ObsIo, private util::ObjectCounter<ObsIoGenerator> {
 public:
    /// \brief classname method for object counter
    ///
    /// \details This method is supplied for the ObjectCounter base class.
    ///          It defines a name to identify an object of this class
    ///          for reporting by OOPS.
    static const std::string classname() {return "ioda::ObsIoGenerator";}

    ObsIoGenerator(const ObsIoActions action, const ObsIoModes mode,
                const ObsSpaceParameters & params);
    ~ObsIoGenerator();

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
    void genDistRandom(const ObsGenerateRandomParameters & params,
                       const util::DateTime & winStart, const util::DateTime & winEnd,
                       const eckit::mpi::Comm & comm,
                       const std::vector<float> & obsErrors,
                       const std::vector<std::string> & simVarNames);

    /// \brief generate observation locations using the list method
    /// \details This method will generate a set of latitudes and longitudes of which
    ///          can be used for testing without reading in an obs file. The values
    ///          are simply read from lists in the configuration file. The purpose of
    ///          this method is to allow the user to exactly specify obs locations.
    ///          these data are intended for use with the MakeObs functionality.
    /// \param params Parameters structure specific to the generate list method
    /// \param obsErrors list of error estimates for each assimilated variable
    /// \param simVarNames list of names of variables to be assimilated
    void genDistList(const ObsGenerateListParameters & params,
                     const std::vector<float> & obsErrors,
                     const std::vector<std::string> & simVarNames);

    /// \brief store generated data into an ObsGroup
    /// \param latVals vector of latitude values
    /// \param lonVals vector of longitude values
    /// \param dtStrings vector of datetime (ISO 8601 string) values
    /// \param obsVarNames vector (string) of simulated variable names
    /// \param obsErrors vector of obs error estimates
    void storeGenData(const std::vector<float> & latVals,
                      const std::vector<float> & lonVals,
                      const std::vector<std::string> & dtStrings,
                      const std::vector<std::string> & obsVarNames,
                      const std::vector<float> & obsErrors);

    /// \brief print routine for oops::Printable base class
    /// \param ostream output stream
    void print(std::ostream & os) const override;
};

}  // namespace ioda

#endif  // IO_OBSIOGENERATOR_H_
