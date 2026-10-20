/*
 *  (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/FrameRows.h"
#include "ioda/containers/IFrame.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/reader/load/loadObs.hpp"

#include "oops/mpi/mpi.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Helpers shared by the tests that apply reader::loadObs to a given input file
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
/// \brief the obsdatain configuration naming one of the files
inline eckit::LocalConfiguration dataInConfigFor(const std::string & fileName) {
  eckit::LocalConfiguration config;
  config.set("engine.type", "H5File");
  config.set("engine.obsfile", "Data/testinput_tier_1/" + fileName);
  return config;
}

// -----------------------------------------------------------------------------
/// \brief apply reader to the given input file, load into container
/// \details The load is driven through reader::loadObs so that the input checks are
/// reached the way an obs space reaches them.
inline void loadObsFromFile(const eckit::LocalConfiguration & dataInConfig,
                            std::unique_ptr<osdf::IFrame> & destOsdf,
                            osdf::FrameMetadata & osdfMetadata) {
  ioda::ObsDataInParameters dataInParams;
  dataInParams.deserialize(dataInConfig);

  eckit::LocalConfiguration ioPoolConfig;
  ioPoolConfig.set("max pool size", 1);
  ioda::IoPool::IoPoolParameters ioPoolParams;
  ioPoolParams.validateAndDeserialize(ioPoolConfig);

  const std::vector<std::string> obsVarNames;
  eckit::LocalConfiguration timeWindowConfig;
  timeWindowConfig.set("begin", "2018-01-01T00:00:00Z");
  timeWindowConfig.set("end", "2018-01-01T06:00:00Z");
  const util::TimeWindow timeWindow(timeWindowConfig);

  reader::loadObs(dataInParams, ioPoolParams, oops::mpi::world(), obsVarNames, timeWindow,
                  "Reader test", destOsdf, osdfMetadata);
}

// -----------------------------------------------------------------------------
/// \brief the load must fail with an eckit::BadValue naming every expected string and no
/// unexpected one
/// \details The messaging is what makes the input checks worth having, so it is tested
/// as well as the exception type.
inline void expectLoadThrowsBadValue(const eckit::LocalConfiguration & dataInConfig,
                                     const std::vector<std::string> & expectedInMessage,
                                     const std::vector<std::string> & unexpectedInMessage = {}) {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  bool threwBadValue = false;
  std::string errMsg;
  try {
    loadObsFromFile(dataInConfig, testOsdf, osdfMetadata);
  } catch (const eckit::BadValue & e) {
    threwBadValue = true;
    errMsg = e.what();
  }
  EXPECT(threwBadValue);
  if (!threwBadValue) {
    return;
  }
  for (const auto & expected : expectedInMessage) {
    const bool found = (errMsg.find(expected) != std::string::npos);
    if (!found) {
      oops::Log::info() << "Reader test: error message is missing: " << expected
                        << std::endl << "    message was: " << errMsg << std::endl;
    }
    EXPECT(found);
  }
  for (const auto & unexpected : unexpectedInMessage) {
    const bool absent = (errMsg.find(unexpected) == std::string::npos);
    if (!absent) {
      oops::Log::info() << "Reader test: error message should not name: " << unexpected
                        << std::endl << "    message was: " << errMsg << std::endl;
    }
    EXPECT(absent);
  }
}

// -----------------------------------------------------------------------------
/// \brief the load must succeed, each of the named columns must be there and none of the
/// absent columns may be
inline void expectLoadSucceeds(const eckit::LocalConfiguration & dataInConfig,
                               const std::vector<std::string> & expectedColumns,
                               const std::vector<std::string> & absentColumns = {}) {
  std::unique_ptr<osdf::IFrame> testOsdf = std::make_unique<osdf::FrameRows>();
  osdf::FrameMetadata osdfMetadata;
  EXPECT_NO_THROW((loadObsFromFile(dataInConfig, testOsdf, osdfMetadata)));
  for (const auto & columnName : expectedColumns) {
    const bool found = testOsdf->hasColumn(columnName);
    if (!found) {
      oops::Log::info() << "Reader test: expected column is missing: " << columnName
                        << std::endl;
    }
    EXPECT(found);
  }
  for (const auto & columnName : absentColumns) {
    const bool absent = !testOsdf->hasColumn(columnName);
    if (!absent) {
      oops::Log::info() << "Reader test: column should not exist: " << columnName << std::endl;
    }
    EXPECT(absent);
  }
}

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
