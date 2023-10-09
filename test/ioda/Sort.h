/*
 * (C) Crown copyright 2020, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_SORT_H_
#define TEST_IODA_SORT_H_

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

void testSort(const eckit::LocalConfiguration &conf) {
  // Produce and configure ObsSpace object
  util::DateTime bgn(conf.getString("window begin"));
  util::DateTime end(conf.getString("window end"));
  const util::TimeWindow timeWindow(bgn, end);

  const eckit::LocalConfiguration obsSpaceConf(conf, "obs space");
  ioda::ObsTopLevelParameters obsParams;
  obsParams.validateAndDeserialize(obsSpaceConf);
  ioda::ObsSpace obsdata(obsParams, oops::mpi::world(), timeWindow, oops::mpi::myself());

  // This test only works for grouped data
  if (obsdata.obs_group_vars().empty()) {
    throw eckit::BadValue("Must set group_variable", Here());
  }

  // Number of locations
  const size_t nlocs = obsdata.nlocs();

  // All expected sort indices, obtained from input file
  std::vector <int> expectedIndicesAll;
  expectedIndicesAll.assign(nlocs, 0);
  const std::string expected_indices_name = conf.getString("expected indices name");
  obsdata.get_db("MetaData", expected_indices_name, expectedIndicesAll, { });

  // Record index for each location
  const std::vector <size_t> recnums = obsdata.recnum();

  // List of unique record indices
  const std::vector <size_t> recnumList = obsdata.recidx_all_recnums();

  for (size_t rn = 0; rn < recnumList.size(); ++rn) {
    // Expected record indices for this recnum
    std::vector <size_t> expectedRecordIndices;
    for (size_t rnindex = 0; rnindex < nlocs; ++rnindex)
      if (recnums[rnindex] == rn)
        expectedRecordIndices.emplace_back(expectedIndicesAll[rnindex]);

    // Actual record indices for this recnum
    const std::vector<size_t> recordIndices = obsdata.recidx_vector(rn);

    // Compare the two vectors and throw an exception if they differ
    const bool equal = std::equal(recordIndices.begin(),
                                  recordIndices.end(),
                                  expectedRecordIndices.begin());
    EXPECT(equal);
  }
  obsdata.save();
}

class Sort : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::Sort";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    for (const std::string & testCaseName : conf.keys())
    {
      const eckit::LocalConfiguration testCaseConf(::test::TestEnvironment::config(), testCaseName);
      ts.emplace_back(CASE("ioda/Sort/" + testCaseName, testCaseConf)
                      {
                        testSort(testCaseConf);
                      });
    }
  }

  void clear() const override {}
};

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_SORT_H_
