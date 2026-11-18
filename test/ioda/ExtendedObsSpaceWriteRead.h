/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_EXTENDEDOBSSPACEWRITEREAD_H_
#define TEST_IODA_EXTENDEDOBSSPACEWRITEREAD_H_

#include <cstddef>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Expect.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

#include "ioda/ObsSpace.h"

#include "IodaTestUtils.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
/// \brief Format a vector of ints as a space separated list, for log messages.
inline std::string intsToString(const std::vector<int> & values) {
  std::string result;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) result += " ";
    result += std::to_string(values[i]);
  }
  return result;
}

// -----------------------------------------------------------------------------
/// \brief Write an extended ObsSpace to a file, read it back, and check that each record's
/// locations kept their order.
///
/// \details
/// Write phase: the ObsSpace is extended with companion records, and a MetaData variable
/// holding each location's position within its in-memory record is added (0..n-1 for an
/// original record, 0..nlevs-1 for a companion record). The ObsSpace is then saved.
///
/// Read phase: the file is opened again without extension, grouped by the same variables
/// but with no sort variable, so each record holds its locations in the order they appear
/// in the file. An original record and its companion record share the grouping variable
/// values, so they come back as one record. That record must hold the original locations,
/// with their position markers in ascending order, followed by exactly nlevs companion
/// locations, with their position markers in ascending order.
///
/// The container is selected for the write obs space by applyContainerDefault (normally
/// from IODA_TEST_CONTAINER), and the read obs space is given the same container and the
/// same file as the write obs space. The read obs space is deliberately not passed to
/// applyContainerDefault: its input file is under testoutput/, which that function
/// rejects because the file would normally belong to another test. Here the file is the
/// one this test has just written, under its final (tagged) name.
void testExtendedObsSpaceWriteRead(const eckit::LocalConfiguration & conf) {
  const util::TimeWindow timeWindow(conf.getSubConfiguration("time window"));
  const eckit::mpi::Comm & comm = oops::mpi::world();
  const std::string markerGroup = "MetaData";
  const std::string markerName = "levelMarker";
  const std::string containerKey = "use data frame container";
  const std::string outputFileKey = "obsdataout.engine.obsfile";

  // Write phase
  eckit::LocalConfiguration writeConf =
    conf.getSubConfiguration("write").getSubConfiguration("obs space");
  applyContainerDefault(::test::TestEnvironment::config(), writeConf);
  // The same YAML is run with different numbers of MPI tasks. Tag the output file name
  // with the task count to keep those runs from overwriting each other's files.
  tagObsOutputFile(writeConf, "_mpi" + std::to_string(comm.size()));
  const std::string outputFile = writeConf.getString(outputFileKey);
  const int nlevs = writeConf.getInt("extension.allocate companion records with length");
  ASSERT(nlevs > 0);
  {
    ObsSpace obsdata(writeConf, comm, timeWindow, oops::mpi::myself());
    ASSERT(!obsdata.obs_group_vars().empty());

    // Number each location by its position within its record, in memory order. The
    // companion records have their own record numbers, so their markers restart at 0.
    const std::vector<std::size_t> & recNums = obsdata.recnum();
    std::unordered_map<std::size_t, int> locsSeenInRecord;
    std::vector<int> marker(obsdata.nlocs());
    for (std::size_t iloc = 0; iloc < marker.size(); ++iloc) {
      marker[iloc] = locsSeenInRecord[recNums[iloc]]++;
    }
    obsdata.put_db(markerGroup, markerName, marker);
    obsdata.save();
  }
  comm.barrier();

  // Read phase
  eckit::LocalConfiguration readConf =
    conf.getSubConfiguration("read").getSubConfiguration("obs space");
  ASSERT_MSG(!readConf.has("obsdatain.engine.obsfile"),
             "The read obs space reads the file written by the write obs space, so its "
             "'obsdatain.engine.obsfile' must not be set in the YAML");
  ASSERT_MSG(!readConf.has(containerKey),
             "The read obs space uses the container of the write obs space, so its '" +
             containerKey + "' must not be set in the YAML");
  readConf.set("obsdatain.engine.obsfile", outputFile);
  if (writeConf.has(containerKey)) {
    readConf.set(containerKey, writeConf.getBool(containerKey));
  }
  ObsSpace readback(readConf, comm, timeWindow, oops::mpi::myself());
  ASSERT(!readback.obs_group_vars().empty());
  ASSERT(!readback.obsAreSorted());

  // Every location written must have been read back.
  EXPECT_EQUAL(readback.globalNumLocs(), conf.getUnsigned("expected gnlocs"));
  std::size_t gnrecs = readback.nrecs();
  comm.allReduceInPlace(gnrecs, eckit::mpi::sum());
  EXPECT_EQUAL(gnrecs, conf.getUnsigned("expected gnrecs"));

  std::vector<int> extendedFlag(readback.nlocs());
  readback.get_db("MetaData", "extendedObsSpace", extendedFlag);
  std::vector<int> marker(readback.nlocs());
  readback.get_db(markerGroup, markerName, marker);
  std::vector<std::string> stationIds(readback.nlocs());
  readback.get_db("MetaData", "stationIdentification", stationIds);

  // Check the layout of each record. Log the details of the first few records that fail,
  // so that the output of a failing run shows how the locations were rearranged.
  const std::size_t maxRecordsToReport = 3;
  std::size_t numBadRecords = 0;
  for (const std::size_t recNum : readback.recidx_all_recnums()) {
    const std::vector<std::size_t> & locs = readback.recidx_vector(recNum);
    std::vector<int> recFlags;
    std::vector<int> originalMarkers;
    std::vector<int> companionMarkers;
    // Set once a companion location has been seen. An original location after that point
    // means the original and companion locations are interleaved.
    bool companionSeen = false;
    bool interleaved = false;
    for (const std::size_t iloc : locs) {
      recFlags.push_back(extendedFlag[iloc]);
      if (extendedFlag[iloc] == 0) {
        if (companionSeen) interleaved = true;
        originalMarkers.push_back(marker[iloc]);
      } else {
        companionSeen = true;
        companionMarkers.push_back(marker[iloc]);
      }
    }

    std::vector<int> expectedOriginalMarkers(originalMarkers.size());
    std::iota(expectedOriginalMarkers.begin(), expectedOriginalMarkers.end(), 0);
    std::vector<int> expectedCompanionMarkers(nlevs);
    std::iota(expectedCompanionMarkers.begin(), expectedCompanionMarkers.end(), 0);

    const bool recordOk = !interleaved &&
                          !originalMarkers.empty() &&
                          originalMarkers == expectedOriginalMarkers &&
                          companionMarkers == expectedCompanionMarkers;
    if (!recordOk) {
      if (numBadRecords < maxRecordsToReport) {
        oops::Log::info() << "rank " << comm.rank() << ": station "
                          << stationIds[locs.front()] << " has " << originalMarkers.size()
                          << " original and " << companionMarkers.size()
                          << " companion locations (expected " << nlevs << " companion)"
                          << (interleaved ? ", interleaved" : "") << std::endl
                          << "  extendedObsSpace: " << intsToString(recFlags) << std::endl
                          << "  original markers: " << intsToString(originalMarkers)
                          << std::endl
                          << "  companion markers: " << intsToString(companionMarkers)
                          << std::endl;
      }
      ++numBadRecords;
    }
  }
  comm.allReduceInPlace(numBadRecords, eckit::mpi::sum());
  oops::Log::info() << "Number of records whose locations were rearranged: "
                    << numBadRecords << std::endl;
  EXPECT_EQUAL(numBadRecords, 0);
}

// -----------------------------------------------------------------------------
class ExtendedObsSpaceWriteRead : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::ExtendedObsSpaceWriteRead";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    for (const std::string & testCaseName : conf.keys()) {
      const eckit::LocalConfiguration testCaseConf(::test::TestEnvironment::config(),
                                                   testCaseName);
      ts.emplace_back(CASE("ioda/ExtendedObsSpaceWriteRead/" + testCaseName, testCaseConf)
                      {
                        testExtendedObsSpaceWriteRead(testCaseConf);
                      });
    }
  }

  void clear() const override {}
};

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_EXTENDEDOBSSPACEWRITEREAD_H_
