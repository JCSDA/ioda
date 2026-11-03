/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

#include "ioda/ObsSpace.h"
#include "IodaTestUtils.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// This test checks that ObsSpace::save writes a separate output file for each
// rank of the time communicator.
//
// The world communicator is split the same way oops splits it for parallel
// subwindows: "number of time ranks" subwindows, each holding a contiguous block
// of world.size() / "number of time ranks" tasks. The local communicator holds the
// tasks in one subwindow, and the time communicator links the tasks that have the
// same local rank across the subwindows.
//
// Each observer lists the suffixes of the output files that must exist after the
// save, and the suffixes of the output files that must not exist (eg, "" for the
// file name without the time rank suffix). The output files from a previous run
// are removed by the ioda_obsspace_timecomm_save_clean_files test, which these
// tests depend on.
// -----------------------------------------------------------------------------

inline bool fileExists(const std::string & fileName) {
  std::ifstream inFile(fileName);
  return inFile.good();
}

inline std::string insertFileSuffix(const std::string & fileName, const std::string & suffix) {
  std::string newFileName = fileName;
  std::size_t found = newFileName.find_last_of(".");
  if (found == std::string::npos) found = newFileName.length();
  return newFileName.insert(found, suffix);
}

CASE("ioda/ObsSpaceTimeCommSave") {
  const eckit::Configuration & conf = ::test::TestEnvironment::config();
  const eckit::mpi::Comm & world = oops::mpi::world();

  const int worldSize = static_cast<int>(world.size());
  const int worldRank = static_cast<int>(world.rank());
  const int numTimeRanks = conf.getInt("number of time ranks");
  // The split below needs the MPI task count to be a multiple of the number of
  // time ranks. Check it here so a mismatch between the yaml and the ctest MPI
  // count gives a clear error rather than a divide by zero or an uneven split.
  if (numTimeRanks < 1 || worldSize % numTimeRanks != 0) {
    throw eckit::BadValue("ObsSpaceTimeCommSave: the number of MPI tasks (" +
                          std::to_string(worldSize) + ") must be a multiple of " +
                          "'number of time ranks' (" + std::to_string(numTimeRanks) + ")",
                          Here());
  }
  const int tasksPerSubwindow = worldSize / numTimeRanks;
  const int timeColor = worldRank / tasksPerSubwindow;
  const int localColor = worldRank % tasksPerSubwindow;

  const std::string localCommName = "ioda_test_timecomm_save_local";
  const std::string timeCommName = "ioda_test_timecomm_save_time";
  const eckit::mpi::Comm & localComm = world.split(timeColor, localCommName);
  const eckit::mpi::Comm & timeComm = world.split(localColor, timeCommName);
  EXPECT(static_cast<int>(timeComm.size()) == numTimeRanks);

  const util::TimeWindow timeWindow(conf.getSubConfiguration("time window"));
  const std::vector<eckit::LocalConfiguration> observerConfs =
    conf.getSubConfiguration("observations").getSubConfigurations("observers");
  for (const eckit::LocalConfiguration & observerConf : observerConfs) {
    eckit::LocalConfiguration obsConf(observerConf, "obs space");
    applyContainerDefault(conf, obsConf);
    oops::Log::info() << "Testing obs space: " << obsConf.getString("name") << std::endl;

    // Form the file names to check from the output file name. Read the name back
    // from obsConf rather than from the yaml, since applyContainerDefault (above)
    // may have rewritten "obsdataout.engine.obsfile" to add the "_osdf" tag, when
    // IODA_TEST_CONTAINER=OSDF. The time and io pool rank suffixes from the yaml
    // then go after that tag.
    const std::string outputFileName = obsConf.getString("obsdataout.engine.obsfile");
    std::vector<std::string> expectedFiles;
    for (const auto & suffix : observerConf.getStringVector("expected output file suffixes")) {
      expectedFiles.push_back(insertFileSuffix(outputFileName, suffix));
    }
    std::vector<std::string> unexpectedFiles;
    for (const auto & suffix : observerConf.getStringVector("unexpected output file suffixes")) {
      unexpectedFiles.push_back(insertFileSuffix(outputFileName, suffix));
    }

    {
      ioda::ObsSpace obsSpace(obsConf, localComm, timeWindow, timeComm);
      obsSpace.save();
    }
    world.barrier();

    // Every rank does the checks (rather than rank 0 alone) so that a failed
    // check, which throws, takes all of the ranks out of the test together.
    // Otherwise the other ranks would hang in the barrier for the next obs space.
    for (const auto & fileName : expectedFiles) {
      oops::Log::info() << "  expected file: " << fileName << std::endl;
      EXPECT(fileExists(fileName));
    }
    for (const auto & fileName : unexpectedFiles) {
      oops::Log::info() << "  unexpected file: " << fileName << std::endl;
      EXPECT_NOT(fileExists(fileName));
    }
    world.barrier();
  }

  eckit::mpi::deleteComm(localCommName);
  eckit::mpi::deleteComm(timeCommName);
}

class ObsSpaceTimeCommSave : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::ObsSpaceTimeCommSave";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda
