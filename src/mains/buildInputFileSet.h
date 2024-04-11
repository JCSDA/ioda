/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef MAINS_TIMEIODAIO_H_
#define MAINS_TIMEIODAIO_H_

#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "ioda/distribution/DistributionFactory.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/ReaderPoolBase.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ObsSpaceParameters.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Application.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

// This application will create a set of ioda input files corresponding to a given
// single ioda obs file and specs on the target io pool and mpi configuration.

namespace ioda {

class BuildInputFileSet : public oops::Application {
 public:
  explicit BuildInputFileSet(const eckit::mpi::Comm & comm = oops::mpi::world())
                                 : Application(comm) {}
  virtual ~BuildInputFileSet() {}

  // -----------------------------------------------------------------------------
  int execute(const eckit::Configuration & fullConfig, bool /* validate */) const {
      // Get the observation time window
      const util::TimeWindow timeWindow(fullConfig.getSubConfiguration("time window"));
      oops::Log::info() << "Observation window: " << timeWindow << std::endl;

      // Form the ObsSpaceParameters object. This will be used to instantiate a reader pool
      // using the ReaderPrepInputFiles subclass. Make sure the configuration is specifying
      // the proper subclass.
      eckit::LocalConfiguration obsSpaceConfig = fullConfig.getSubConfiguration("obs space");
      const std::string readerName = obsSpaceConfig.getString("io pool.reader name");
      if (readerName != "PrepInputFiles") {
          const std::string errMsg = std::string("Must use the io pool reader name: ") +
                                     std::string("PrepInputFiles for this application");
          throw eckit::BadParameter(errMsg.c_str(), Here());
      }
      ObsSpaceParameters obsSpaceParams(obsSpaceConfig, timeWindow,
                                        this->getComm(), oops::mpi::myself());

      // Create an MPI distribution object
      const auto & distParams = obsSpaceParams.top_level_.distribution.value().params.value();
      std::shared_ptr<Distribution> distribution =
          DistributionFactory::create(obsSpaceParams.comm(), distParams);

      // Create the reader pool, and then just call the initialization step to get the
      // input file set built.
      IoPool::ReaderPoolCreationParameters createParams(
          obsSpaceParams.comm(), obsSpaceParams.timeComm(),
          obsSpaceParams.top_level_.obsDataIn.value().engine.value().engineParameters,
          obsSpaceParams.timeWindow(),
          obsSpaceParams.top_level_.simVars.value().variables(), distribution,
          obsSpaceParams.top_level_.obsDataIn.value().obsGrouping.value().obsGroupVars,
          obsSpaceParams.top_level_.obsDataIn.value().prepType);

      std::unique_ptr<IoPool::ReaderPoolBase> readPool =
          IoPool::ReaderPoolFactory::create(obsSpaceParams.top_level_.ioPool, createParams);

      // This builds the input files
      readPool->initialize();
      return 0;
  }

// -----------------------------------------------------------------------------
 private:
  std::string appname() const {
    return "ioda::BuildInputFileSet";
  }
// -----------------------------------------------------------------------------
};

}  // namespace ioda

#endif  // MAINS_TIMEIODAIO_H_
