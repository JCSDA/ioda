/*
 * (C) Copyright 2024 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEAPPEND_H_
#define TEST_IODA_OBSSPACEAPPEND_H_

#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/distribution/Accumulator.h"
#include "ioda/Exception.h"
#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

class ObsSpaceTestFixture : private boost::noncopyable {
 public:
  static ioda::ObsSpace & obspace(const std::size_t ii) {
    return *getInstance().ospaces_.at(ii);
  }
  static std::size_t size() {return getInstance().ospaces_.size();}
  static void cleanup() {
    auto &spaces = getInstance().ospaces_;
    for (auto &space : spaces) {
      space->save();
      space.reset();
    }
  }

 private:
  static ObsSpaceTestFixture & getInstance() {
    static ObsSpaceTestFixture theObsSpaceTestFixture;
    return theObsSpaceTestFixture;
  }

  ObsSpaceTestFixture(): ospaces_() {
    const util::TimeWindow timeWindow
      (::test::TestEnvironment::config().getSubConfiguration("time window"));
    std::vector<eckit::LocalConfiguration> conf;
    ::test::TestEnvironment::config().get("observations", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      eckit::LocalConfiguration obsconf(conf[jj], "obs space");
      ioda::ObsTopLevelParameters obsparams;
      obsparams.validateAndDeserialize(obsconf);
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, oops::mpi::world(),
                                                               timeWindow, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------
void checkNrecs(const ObsSpace & odb, const size_t expectedGlobalNrecs) {
  // records are ambigious and not implemented for halo distribution
  if (odb.distribution()->name() != "Halo") {
    std::size_t Nlocs = odb.nlocs();
    std::size_t Nrecs = 0;
    std::set<std::size_t> recIndices;
    auto accumulator = odb.distribution()->createAccumulator<std::size_t>();
    for (std::size_t loc = 0; loc < Nlocs; ++loc) {
      if (recIndices.insert(odb.recnum()[loc]).second) {
        accumulator->addTerm(loc, 1);
        ++Nrecs;
      }
    }
    const size_t expectedNrecs = odb.nrecs();
    EXPECT_EQUAL(Nrecs, expectedNrecs);

    // Calculate the global number of unique records
    const std::size_t GlobalNrecs = accumulator->computeResult();
    EXPECT_EQUAL(GlobalNrecs, expectedGlobalNrecs);
  }
}

// -----------------------------------------------------------------------------
void updateDerivedObsError(ObsSpace & odb,
                           const eckit::LocalConfiguration & testConfig,
                           const std::string & updateConfigName) {
  // Get the configuration containing the list of variables to update
  std::vector<eckit::LocalConfiguration> updateObsErrorConfig;
  testConfig.get(updateConfigName, updateObsErrorConfig);
  for (std::size_t i = 0; i < updateObsErrorConfig.size(); ++i) {
    std::string group = updateObsErrorConfig[i].getString("group");
    std::string name = updateObsErrorConfig[i].getString("name");
    std::vector<float> values = updateObsErrorConfig[i].getFloatVector("values");

    std::vector<float> obsErrors;
    odb.get_db(group, name, obsErrors);
    std::size_t indx = obsErrors.size() - values.size();
    for (std::size_t i = 0; i < values.size(); ++i) {
      obsErrors[indx] = values[i];
      ++indx;
    }
    odb.put_db(group, name, obsErrors);
  }
}

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test configuration which holds the expected data.
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);
    oops::Log::debug() << "Test data configuration: " << testConfig << std::endl;

    const ObsSpace &odb = Test_::obspace(jj);

    // Get the global numbers of locations and vars from the ObsSpace object
    // These values are not expected to change whether running with a single process
    // or multiple MPI tasks. There are other tests that check local stats according to
    // the MPI distribution.
    const std::size_t globalNlocs = odb.globalNumLocs();
    const std::size_t globalNlocsOutsideTimeWindow = odb.globalNumLocsOutsideTimeWindow();
    const bool ObsAreSorted = odb.obsAreSorted();

    // Get the expected nlocs from the obspace object's configuration
    const std::size_t expectedGlobalNlocs = testConfig.getUnsigned("after constructor.gnlocs");
    const std::size_t expectedGlobalNlocsOutsideTimeWindow =
        testConfig.getUnsigned("after constructor.gnlocs outside time window");
    const bool expectedObsAreSorted = testConfig.getBool("after constructor.obs are sorted");

    oops::Log::debug() << "globalNlocs, expectedGlobalNlocs: " << globalNlocs << ", "
                       << expectedGlobalNlocs << std::endl;
    oops::Log::debug() << "globalNlocsOutsideTimeWindow, expectedGlobalNlocsOutsideTimeWindow: "
                       << globalNlocsOutsideTimeWindow << ", "
                       << expectedGlobalNlocsOutsideTimeWindow << std::endl;
    oops::Log::debug() << "ObsAreSorted, expectedObsAreSorted: " << ObsAreSorted << ", "
                       << expectedObsAreSorted << std::endl;

    EXPECT(globalNlocs == expectedGlobalNlocs);
    EXPECT(globalNlocsOutsideTimeWindow == expectedGlobalNlocsOutsideTimeWindow);
    EXPECT(ObsAreSorted == expectedObsAreSorted);

    const std::size_t expectedGlobalNrecs = testConfig.getUnsigned("after constructor.nrecs");
    checkNrecs(odb, expectedGlobalNrecs);
  }
}

// -----------------------------------------------------------------------------

void testAppend() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test configuration which holds the expected data.
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);
    oops::Log::debug() << "Test data configuration: " << testConfig << std::endl;

    ObsSpace &odb = Test_::obspace(jj);
    ioda::ObsVector vec(Test_::obspace(jj), "ObsValue");
    vec *= 2.0;
    std::vector<double> vec_data = vec.data();    // all variables
    ioda::ObsDataVector<double> obsvec(Test_::obspace(jj), vec.varnames(), "ObsValue");
    std::vector<double> obsvec_data = obsvec[0];  // just one variable

    // First, update the variables in derived obs error group (if any)
    updateDerivedObsError(odb, testConfig, "after constructor.update derived obs error");

    // Walk through the append sequence
    std::vector<eckit::LocalConfiguration> appendSequence;
    testConfig.get("append sequence", appendSequence);
    for (std::size_t iapp = 0; iapp < appendSequence.size(); ++iapp) {
      // get the expected append directory and call the append function
      const std::string appendDir = appendSequence[iapp].getString("append directory");
      odb.append(appendDir);

      // get the expected values and compare
      const std::size_t expectedGlobalNlocs = appendSequence[iapp].getUnsigned("gnlocs");
      const std::size_t globalNlocs = odb.globalNumLocs();
      EXPECT_EQUAL(globalNlocs, expectedGlobalNlocs);

      const std::size_t expectedGlobalNlocsOutsideTimeWindow =
                            appendSequence[iapp].getUnsigned("gnlocs outside time window");
      const std::size_t globalNlocsOutsideTimeWindow = odb.globalNumLocsOutsideTimeWindow();
      EXPECT_EQUAL(globalNlocsOutsideTimeWindow, expectedGlobalNlocsOutsideTimeWindow);

      const bool expectedObsAreSorted = appendSequence[iapp].getBool("obs are sorted");
      const bool obsAreSorted = odb.obsAreSorted();
      EXPECT_EQUAL(obsAreSorted, expectedObsAreSorted);

      const std::size_t expectedGlobalNrecs = appendSequence[iapp].getUnsigned("nrecs");
      checkNrecs(odb, expectedGlobalNrecs);

      // Update the variables in derived obs error group (if any)
      updateDerivedObsError(odb, appendSequence[iapp], "update derived obs error");

      // Test appending ObsVector and ObsDataVector
      std::vector<double> vec_data_appended = vec.data();
      std::vector<double> obsvec_data_appended = obsvec[0];
      // expect the first parts of vectors to be the same, and the last to have
      // missing values
      EXPECT(std::equal(vec_data.begin(), vec_data.end(), vec_data_appended.begin()));
      EXPECT(std::equal(obsvec_data.begin(), obsvec_data.end(), obsvec_data_appended.begin()));
      double missing = util::missingValue<double>();
      EXPECT(std::all_of(vec_data_appended.begin() + vec_data.size(), vec_data_appended.end(),
                         [missing](double elem) {return elem == missing;}));
      EXPECT(std::all_of(obsvec_data_appended.begin() + obsvec_data.size(),
                         obsvec_data_appended.end(),
                         [missing](double elem) {return elem == missing;}));
      // get a new ObsVector from the appended ObsSpace and read ObsValue in it
      ioda::ObsVector vec_afterappend(Test_::obspace(jj), "ObsValue");
      std::vector<double> vec_afterappend_data = vec_afterappend.data();
      oops::Log::info() << "Vector created before append with doubled values: "
                        << vec << std::endl;
      oops::Log::info() << "Vector created after append: "
                        << vec_afterappend << std::endl;
      // read values only in the appended part of the ObsVector
      vec.readAppended("ObsValue");
      oops::Log::info() << "Vector created before append with doubled values after "
                        << "reading values in the appended part: " << vec << std::endl;
      vec_data_appended = vec.data();
      // expect the first part of vector to be the same as before (2 * ObsValue), and
      // the rest to have ObsValue
      EXPECT(std::equal(vec_data.begin(), vec_data.end(), vec_data_appended.begin()));
      EXPECT(std::equal(vec_data_appended.begin() + vec_data.size(), vec_data_appended.end(),
                        vec_afterappend_data.begin() + vec_data.size()));
      // now read values in all of the ObsVector
      // expect the whole vector to have ObsValue
      vec.read("ObsValue");
      oops::Log::info() << "Vector created before append after reading all values: "
                        << vec << std::endl;
      vec_data_appended = vec.data();
      EXPECT(std::equal(vec_data_appended.begin(), vec_data_appended.end(),
                        vec_afterappend_data.begin()));
      // update data vectors to compare after the next append call
      vec *= 2.0;
      vec_data = vec.data();
      obsvec_data = obsvec[0];
    }

    // Get the expected index and recnum vectors from the obspace object's configuration
    const int MyMpiSize = odb.comm().size();
    const int MyMpiRank = odb.comm().rank();

    // Look up config according to MPI size and rank
    const std::string MyPath = "mpi size" + std::to_string(MyMpiSize) +
                               ".rank" + std::to_string(MyMpiRank);

    // Check that the index and recnum vectors have been properly adjusted
    const std::vector<std::size_t> ExpectedIndex =
      testConfig.getUnsignedVector(MyPath + ".index");
    const std::vector<std::size_t> ExpectedRecnum =
      testConfig.getUnsignedVector(MyPath + ".recnum");
    eckit::LocalConfiguration recidxTestConfig;
    testConfig.get(MyPath + ".recidx", recidxTestConfig);

    // Get the index and recnum vectors from the obs space
    const std::vector<std::size_t> Index = odb.index();
    const std::vector<std::size_t> Recnum = odb.recnum();

    oops::Log::debug() << "Index, ExpectedIndex: " << Index << ", "
                       << ExpectedIndex << std::endl;
    oops::Log::debug() << "Recnum, ExpectedRecnum: " << Recnum << ", "
                       << ExpectedRecnum << std::endl;

    EXPECT(Index == ExpectedIndex);
    EXPECT(Recnum == ExpectedRecnum);

    // check that the recidx data structure got adjusted properly
    oops::Log::debug() << "recidxTestConfig: " << recidxTestConfig << std::endl;
    const std::vector<std::size_t> RecIdxRecNums = odb.recidx_all_recnums();
    for (std::size_t i = 0; i < RecIdxRecNums.size(); ++i) {
      const std::size_t RecNum = RecIdxRecNums[i];
      const std::string TestConfigKey = "rec" + std::to_string(RecNum);
      const std::vector<std::size_t> ExpectedRecIdxVector =
        recidxTestConfig.getUnsignedVector(TestConfigKey);
      const std::vector<std::size_t> RecIdxVector = odb.recidx_vector(RecNum);

      oops::Log::debug() << "RecNum -> RecIdxVector, ExpectedRecIdxVector: "
                         << RecNum << " -> " << RecIdxVector << ", "
                         << ExpectedRecIdxVector << std::endl;
      EXPECT(RecIdxVector == ExpectedRecIdxVector);
    }

    // check that the patchObs from the associated distribution got adjusted properly
    std::vector<bool> patchObs(odb.nlocs());
    odb.distribution()->patchObs(patchObs);
    std::vector<int> patchObsInt(patchObs.size());
    for (std::size_t i = 0; i < patchObs.size(); ++i) {
      patchObsInt[i] = patchObs[i] ? 1 : 0;
    }
    std::vector<int> expectedPatchObs = testConfig.getIntVector(MyPath + ".patch obs");
    oops::Log::debug() << "patchObsInt, expectedPatchObs: " << patchObsInt << ", "
                       << expectedPatchObs << std::endl;
    EXPECT(patchObsInt == expectedPatchObs);

    const size_t expectedNlocs = testConfig.getInt(MyPath + ".nlocs");
    EXPECT_EQUAL(vec.nlocs(), expectedNlocs);
    EXPECT_EQUAL(obsvec.nlocs(), expectedNlocs);
  }
}

// -----------------------------------------------------------------------------

void testCleanup() {
  // This test removes the obsspaces and ensures that they evict their contents
  // to disk successfully.
  typedef ObsSpaceTestFixture Test_;

  Test_::cleanup();
}

// -----------------------------------------------------------------------------

class ObsSpaceAppend : public oops::Test {
 public:
  ObsSpaceAppend() {}
  virtual ~ObsSpaceAppend() {}
 private:
  std::string testid() const override {return "test::ObsSpaceAppend<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpaceAppend/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("ioda/ObsSpaceAppend/testAppend")
      { testAppend(); });
    ts.emplace_back(CASE("ioda/ObsSpaceAppend/testCleanup")
      { testCleanup(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEAPPEND_H_
