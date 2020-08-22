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
class ObsGenerate : public ObsIo, private util::ObjectCounter<ObsGenerate> {
    public:
        /// \brief classname method for object counter
        ///
        /// \details This method is supplied for the ObjectCounter base class.
        ///          It defines a name to identify an object of this class
        ///          for reporting by OOPS.
        static const std::string classname() {return "ioda::ObsFile";}

        ObsGenerate(const ObsIoActions action, const ObsIoModes mode,
                    const ObsIoParameters & params);
        ~ObsGenerate();

    private:
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
        void genDistRandom(const ObsGenerateRandomParameters & params,
                           const util::DateTime & winbgn, const util::DateTime & winend,
                           const eckit::mpi::Comm & comm);

        /// \brief generate observation locations using the list method
        /// \details This method will generate a set of latitudes and longitudes of which
        ///          can be used for testing without reading in an obs file. The values
        ///          are simply read from lists in the configuration file. The purpose of
        ///          this method is to allow the user to exactly specify obs locations.
        ///          these data are intended for use with the MakeObs functionality.
        /// \param params Parameters structure specific to the generate list method
        void genDistList(const ObsGenerateListParameters & params);

        /// \brief print routine for oops::Printable base class
        /// \param ostream output stream
        void print(std::ostream & os) const override;

        ObsGroup obs_group_;
};

}  // namespace ioda

#endif  // IO_OBSGENERATE_H_
