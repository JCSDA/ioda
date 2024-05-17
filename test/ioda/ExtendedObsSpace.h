/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_EXTENDEDOBSSPACE_H_
#define TEST_IODA_EXTENDEDOBSSPACE_H_

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Expect.h"

#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"

namespace ioda {

namespace test {

void testExtendedObsSpace(const eckit::LocalConfiguration &conf) {
  // Produce and configure ObsSpace object
  const util::TimeWindow timeWindow(conf.getSubConfiguration("time window"));

  const eckit::LocalConfiguration obsSpaceConf(conf, "obs space");

  // Instantiate ObsSpace, allowing for exceptions to be thrown.
  const bool expectThrows = conf.getBool("expectThrows", false);
  if (expectThrows) {
    EXPECT_THROWS(ioda::ObsSpace obsDataThrow(obsSpaceConf,
                                              oops::mpi::world(),
                                              timeWindow, oops::mpi::myself()));
    return;
  }
  ioda::ObsSpace obsdata(obsSpaceConf, oops::mpi::world(), timeWindow, oops::mpi::myself());

  // This test only works for grouped data.
  if (obsdata.obs_group_vars().empty())
    throw eckit::BadValue("Must set 'group variables' configuration option", Here());
  // This test only works if the correct ObsSpace extension options have been supplied.
  if (!obsSpaceConf.has("extension"))
    throw eckit::BadValue("Must set 'extension' configuration option", Here());
  // Number of locations per companion record in the extended ObsSpace.
  const int nlevs = obsSpaceConf.getInt("extension.allocate companion records with length", 0);
  // The extension is not performed if the number of locations is less than or equal to zero.
  if (nlevs <= 0) return;

  const int MPIsize = obsdata.comm().size();
  const int MPIrank = obsdata.comm().rank();

  // Compare number of locations with expected value.
  const int nlocs = static_cast<int>(obsdata.nlocs());
  std::stringstream expectednumber;
  expectednumber << "expected nlocs (" << MPIsize << " PE, rank " << MPIrank << ")";
  const int nlocs_expected = conf.getInt(expectednumber.str());
  EXPECT_EQUAL(nlocs, nlocs_expected);

  // Compare global number of locations with expected value.
  const int gnlocs = static_cast<int>(obsdata.globalNumLocs());
  expectednumber.str("");
  expectednumber << "expected gnlocs (" << MPIsize << " PE, rank " << MPIrank << ")";
  const int gnlocs_expected = conf.getInt(expectednumber.str());
  EXPECT_EQUAL(gnlocs, gnlocs_expected);

  // Compare number of records with expected value.
  const int nrecs = static_cast<int>(obsdata.nrecs());
  expectednumber.str("");
  expectednumber << "expected nrecs (" << MPIsize << " PE, rank " << MPIrank << ")";
  const int nrecs_expected = conf.getInt(expectednumber.str());
  EXPECT_EQUAL(nrecs, nrecs_expected);

  // Given the extended records have nlevs entries each,
  // calculate the corresponding index at which extendedObsSpace switches from 0 to 1.
  std::vector <int> extendedObsSpace(nlocs);
  obsdata.get_db("MetaData", "extendedObsSpace", extendedObsSpace);
  const size_t extendedObsSpaceStart =
    std::find(extendedObsSpace.begin(),
              extendedObsSpace.end(), 1) - extendedObsSpace.begin();
  // Check the index of the start of the extended ObsSpace is
  // a multiple of nlevs from the final index.
  EXPECT_EQUAL((nlocs - extendedObsSpaceStart) % nlevs, 0);
  // Check the values of extendedObsSpace.
  for (size_t iloc = 0; iloc < extendedObsSpaceStart; ++iloc)
    EXPECT_EQUAL(extendedObsSpace[iloc], 0);
  for (int iloc = extendedObsSpaceStart; iloc < nlocs; ++iloc)
    EXPECT_EQUAL(extendedObsSpace[iloc], 1);

  // Get all ObsValue and ObsError vectors that will be simulated.
  // For each vector check that the values in the extended ObsSpace are all missing.
  const float missingValueFloat = util::missingValue<float>();
  std::vector <float> val(nlocs);
  std::vector <float> err(nlocs);
  const oops::ObsVariables& obsvars = obsdata.obsvariables();
  for (size_t ivar = 0; ivar < obsvars.size(); ++ivar) {
    const std::string varname = obsvars[ivar];
    obsdata.get_db("ObsValue", varname, val);
    obsdata.get_db("ObsError", varname, err);
    for (int iloc = extendedObsSpaceStart; iloc < nlocs; ++iloc) {
      EXPECT_EQUAL(val[iloc], missingValueFloat);
      EXPECT_EQUAL(err[iloc], missingValueFloat);
    }
  }

  // Compare record numbers on this processor.
  // There should be an even number of records; the second half should have indices shifted
  // by a constant offset with respect to the first half. This offset should be equal to the
  // original number of records.
  const std::vector<std::size_t> recidx_all_recnums = obsdata.recidx_all_recnums();
  // Determine the original record numbers by dividing the global number of records by two.
  EXPECT_EQUAL(nrecs % 2, 0);
  std::size_t nrecs_original = nrecs / 2;
  const std::vector<std::size_t> original_recnums(recidx_all_recnums.begin(),
                                                  recidx_all_recnums.begin() + nrecs / 2);
  const std::vector<std::size_t> extended_recnums(recidx_all_recnums.begin() + nrecs / 2,
                                                  recidx_all_recnums.end());
  std::size_t gnrecs_original = original_recnums.empty() ? 0 : (original_recnums.back() + 1);
  obsdata.distribution()->max(gnrecs_original);

  std::vector<std::size_t> extended_recnums_expected;
  for (std::size_t i : original_recnums)
    extended_recnums_expected.push_back(i + gnrecs_original);
  EXPECT_EQUAL(extended_recnums, extended_recnums_expected);

  // Compare indices across all processors.
  // Gather all indices, sort them, and produce a vector of unique indices.
  std::vector <size_t> index_processors = obsdata.index();
  obsdata.distribution()->allGatherv(index_processors);
  std::sort(index_processors.begin(), index_processors.end());
  auto it_unique = std::unique(index_processors.begin(), index_processors.end());
  index_processors.erase(it_unique, index_processors.end());
  // Produce expected indices.
  std::vector <size_t> index_processors_expected(index_processors.size());
  std::iota(index_processors_expected.begin(), index_processors_expected.end(), 0);
  // Compare actual and expected indices.
  EXPECT_EQUAL(index_processors, index_processors_expected);

  // Check that values in each averaged profile have been set as desired.

  // User-configured list of variables that should be filled with non-missing values.
  const std::vector <std::string> &nonMissingExtendedVars =
    obsSpaceConf.getStringVector("extension.variables filled with non-missing values",
                                 {"latitude", "longitude", "datetime",
                                     "pressure", "air_pressure_levels", "stationIdentification"});
  // List of variables to check.
  // It is required that these are all floating-point variables in the MetaData group.
  const std::vector <std::string> extendedVarsToCheck =
    {"latitude", "longitude", "pressure"};
  // Retrieve all station IDs in the sample.
  std::vector <std::string> statids(obsdata.nlocs());
  obsdata.get_db("MetaData", "stationIdentification", statids);
  // Unique station IDs are taken from the configuration file.
  // The IDs are loaded in this way in order to guarantee a particular correspondence
  // with the reference vectors.
  const std::vector <std::string> uniqueStatids =
    conf.getStringVector("unique statids");

  // Vector holding values of any variables to check.
  std::vector <float> varToCheck(obsdata.nlocs());
  // Loop over all variables to check.
  for (const auto & extendedVar : extendedVarsToCheck) {
    obsdata.get_db("MetaData", extendedVar, varToCheck);
    // Check whether this variable should have been filled.
    const bool nonMissing =
      std::find(nonMissingExtendedVars.begin(), nonMissingExtendedVars.end(), extendedVar)
      != nonMissingExtendedVars.end();
    // Loop over each original profile in the sample.
    for (std::size_t jprof = 0; jprof < nrecs_original; ++jprof) {
      // Locations corresponding to the original profile.
      const std::vector<std::size_t> locsOriginal =
        obsdata.recidx_vector(recidx_all_recnums[jprof]);
      // Locations corresponding to the averaged profile.
      const std::vector<std::size_t> locsExtended =
        obsdata.recidx_vector(recidx_all_recnums[jprof + nrecs_original]);
      // Obtain comparison value from configuration file.
      float valueToCompare = missingValueFloat;
      if (nonMissing) {
        const std::vector <float> expectedValues = conf.getFloatVector("expected " + extendedVar);
        const std::string statid_prof = statids[locsOriginal.front()];
        auto it_statid_prof = std::find(uniqueStatids.begin(), uniqueStatids.end(), statid_prof);
        valueToCompare = expectedValues[it_statid_prof - uniqueStatids.begin()];
      }
      // Compare values in the averaged profile to the expected value.
      for (const auto jloc : locsExtended)
        EXPECT(varToCheck[jloc] == valueToCompare);
    }
  }

  obsdata.save();
}

class ExtendedObsSpace : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::ExtendedObsSpace";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    for (const std::string & testCaseName : conf.keys())
    {
      const eckit::LocalConfiguration testCaseConf(::test::TestEnvironment::config(), testCaseName);
      ts.emplace_back(CASE("ioda/ExtendedObsSpace/" + testCaseName, testCaseConf)
                      {
                        testExtendedObsSpace(testCaseConf);
                      });
    }
  }

  void clear() const override {}
};

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_EXTENDEDOBSSPACE_H_
