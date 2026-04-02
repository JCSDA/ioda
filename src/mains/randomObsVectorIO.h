/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

#include "eckit/config/LocalConfiguration.h"

#include "oops/base/Observations.h"
#include "oops/base/ObsSpaces.h"
#include "oops/mpi/mpi.h"
#include "oops/runs/Application.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

namespace ioda {

template <typename MODEL>
class RandomObsVectorIO : public oops::Application {
  typedef oops::ObsSpaces<MODEL> ObsSpaces_;

 public:
  explicit RandomObsVectorIO(const eckit::mpi::Comm & comm = oops::mpi::world())
    : Application(comm) {}

  virtual ~RandomObsVectorIO() {}

  int execute(const eckit::Configuration & fullConfig) const override {
    // Setup observation window
    const util::TimeWindow timeWindow(fullConfig.getSubConfiguration("time window"));
    oops::Log::info() << "Observation window: " << timeWindow << std::endl;

    // Setup observations
    eckit::LocalConfiguration obsconf(fullConfig, "observations");
    oops::Log::debug() << "Observations configuration is:" << obsconf << std::endl;
    ObsSpaces_ obsdb(obsconf, this->getComm(), timeWindow);

    // Optional settings
    int nfiles = 10000;
    std::string groupPrefix = "RandomObsVector";

    if (fullConfig.has("nfiles")) {
      nfiles = fullConfig.getInt("nfiles");
    }
    if (fullConfig.has("group prefix")) {
      groupPrefix = fullConfig.getString("group prefix");
    }

    for (std::size_t jj = 0; jj < obsdb.size(); ++jj) {
      oops::Log::info() << "ObsSpace: " << obsdb[jj].obsname() << std::endl;
      oops::Log::info() << "  Number of locations: " << obsdb[jj].obsspace().nlocs()
                        << std::endl;
      oops::Log::info() << "  Number of variables: " << obsdb[jj].obsspace().nvars()
                        << std::endl;
      oops::Log::info() << "  Number of records: " << obsdb[jj].obsspace().nrecs()
                        << std::endl;

      ioda::ObsSpace & obspace = obsdb[jj].obsspace();

      for (int i = 0; i < nfiles; ++i) {
        auto t0 = std::chrono::high_resolution_clock::now();
        ioda::ObsVector vec(obspace);
        vec.random();

        std::ostringstream groupname;
        groupname << groupPrefix << "_"
                  << std::setw(5) << std::setfill('0') << i;

        oops::Log::info() << "Saving random ObsVector to group "
                          << groupname.str() << std::endl;

        vec.save(groupname.str());
        auto t1 = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double, std::milli>(t1 - t0).count();
        oops::Log::info() << "Iteration " << i << " took " << dt << " ms" << std::endl;
      }

      // Write output file if obsdataout is configured
      obsdb[jj].save();
    }

    return 0;
  }

// -----------------------------------------------------------------------------
 private:
  std::string appname() const override {
    return "oops::RandomObsVectorIO<" + MODEL::name() + ">";
  }
// -----------------------------------------------------------------------------
};

}  // namespace ioda
