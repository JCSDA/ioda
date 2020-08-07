/*
 * (C) Crown copyright 2020, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_DESCENDINGSORT_H_
#define TEST_IODA_DESCENDINGSORT_H_

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Expect.h"

#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"

namespace ioda {

namespace test {

void testDescendingSort(const eckit::LocalConfiguration &conf) {
  // Produce and configure ObsData object
  util::DateTime bgn(conf.getString("window begin"));
  util::DateTime end(conf.getString("window end"));
  const eckit::LocalConfiguration obsSpaceConf(conf, "obs space");
  ioda::ObsData obsdata(obsSpaceConf, oops::mpi::comm(), bgn, end);

  // This test only works for grouped data with descending sort order
  if (obsdata.obs_sort_order() != "descending") {
    throw eckit::BadValue("Must set sort_order to descending", Here());
  }
  if (obsdata.obs_group_var().empty()) {
    throw eckit::BadValue("Must set group_variable", Here());
  }

  // Number of locations
  const size_t nlocs = obsdata.nlocs();

  // All expected sort indices, obtained from input file
  std::vector <int> expectedIndicesAll;
  expectedIndicesAll.assign(nlocs, 0);
  obsdata.get_db("MetaData", "expected_indices", expectedIndicesAll);

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
}

class DescendingSort : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::DescendingSort";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    for (const std::string & testCaseName : conf.keys())
    {
      const eckit::LocalConfiguration testCaseConf(::test::TestEnvironment::config(), testCaseName);
      ts.emplace_back(CASE("ioda/DescendingSort/" + testCaseName, testCaseConf)
                      {
                        testDescendingSort(testCaseConf);
                      });
    }
  }
};

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_DESCENDINGSORT_H_
