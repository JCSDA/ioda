/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef MAINS_TIMEIODAIO_H_
#define MAINS_TIMEIODAIO_H_

#include<string>
#include<vector>

#include "eckit/config/LocalConfiguration.h"

#include "oops/base/Observations.h"
#include "oops/base/ObsSpaces.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Application.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

#include "fileio/IodaIO.h"
#include "fileio/IodaIOfactory.h"
#include "utils/IodaUtils.h"

namespace ioda {

template <typename MODEL> class TimeIodaIO : public oops::Application {
  typedef oops::ObsSpaces<MODEL>         ObsSpaces_;

 public:
// -----------------------------------------------------------------------------
  explicit TimeIodaIO(const eckit::mpi::Comm & comm = oops::mpi::comm()) : Application(comm) {}
// -----------------------------------------------------------------------------
  virtual ~TimeIodaIO() {}
// -----------------------------------------------------------------------------
  int execute(const eckit::Configuration & fullConfig) const {
//  Setup observation window
    const util::DateTime winbgn(fullConfig.getString("window_begin"));
    const util::DateTime winend(fullConfig.getString("window_end"));
    oops::Log::info() << "Observation window begin:" << winbgn << std::endl;
    oops::Log::info() << "Observation window end:" << winend << std::endl;

//  Setup observations
    eckit::LocalConfiguration obsconf(fullConfig, "Observations");
    oops::Log::debug() << "Observations configuration is:" << obsconf << std::endl;
    ObsSpaces_ obsdb(obsconf, this->getComm(), winbgn, winend);

    std::vector<eckit::LocalConfiguration> conf;
    obsconf.get("ObsTypes", conf);

    for (std::size_t jj = 0; jj < obsdb.size(); ++jj) {
      oops::Log::info() << "ObsSpace: " << obsdb[jj].obsname() << std::endl;
      oops::Log::info() << "  Number of locations: " << obsdb[jj].obsspace().nlocs()
                        << std::endl;
      oops::Log::info() << "  Number of variables: " << obsdb[jj].obsspace().nvars()
                        << std::endl;
      oops::Log::info() << "  Number of records: " << obsdb[jj].obsspace().nrecs()
                        << std::endl;
    }
  return 0;
  }

// -----------------------------------------------------------------------------
 private:
  std::string appname() const {
    return "oops::TimeIodaIO<" + MODEL::name() + ">";
  }
// -----------------------------------------------------------------------------
};

}  // namespace ioda

#endif  // MAINS_TIMEIODAIO_H_
