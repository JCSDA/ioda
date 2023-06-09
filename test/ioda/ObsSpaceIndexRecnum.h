/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEINDEXRECNUM_H_
#define TEST_IODA_OBSSPACEINDEXRECNUM_H_

#include <cmath>
#include <set>
#include <string>
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
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    std::vector<eckit::LocalConfiguration> conf;
    ::test::TestEnvironment::config().get("observations", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      eckit::LocalConfiguration obsconf(conf[jj], "obs space");
      ioda::ObsTopLevelParameters obsparams;
      obsparams.validateAndDeserialize(obsconf);
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsparams, oops::mpi::world(),
                                                               bgn, end, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

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
    std::size_t GlobalNlocs = odb.globalNumLocs();
    std::size_t GlobalNlocsOutsideTimeWindow = odb.globalNumLocsOutsideTimeWindow();
    bool ObsAreSorted = odb.obsAreSorted();

    // Get the expected nlocs from the obspace object's configuration
    std::size_t ExpectedGlobalNlocs = testConfig.getUnsigned("gnlocs");
    std::size_t ExpectedGlobalNlocsOutsideTimeWindow =
        testConfig.getUnsigned("gnlocs outside time window");
    bool ExpectedObsAreSorted = testConfig.getBool("obs are sorted");

    oops::Log::debug() << "GlobalNlocs, ExpectedGlobalNlocs: " << GlobalNlocs << ", "
                       << ExpectedGlobalNlocs << std::endl;
    oops::Log::debug() << "GlobalNlocsOutsideTimeWindow, ExpectedGlobalNlocsOutsideTimeWindow: "
                       << GlobalNlocsOutsideTimeWindow << ", "
                       << ExpectedGlobalNlocsOutsideTimeWindow << std::endl;
    oops::Log::debug() << "ObsAreSorted, ExpectedObsAreSorted: " << ObsAreSorted << ", "
                       << ExpectedObsAreSorted << std::endl;

    EXPECT(GlobalNlocs == ExpectedGlobalNlocs);
    EXPECT(GlobalNlocsOutsideTimeWindow == ExpectedGlobalNlocsOutsideTimeWindow);
    EXPECT(ObsAreSorted == ExpectedObsAreSorted);

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
      const size_t ExpectedNrecs = odb.nrecs();
      EXPECT_EQUAL(Nrecs, ExpectedNrecs);

      // Calculate the global number of unique records
      std::size_t GlobalNrecs = accumulator->computeResult();
      std::size_t ExpectedGlobalNrecs = testConfig.getUnsigned("nrecs");
      EXPECT_EQUAL(GlobalNrecs, ExpectedGlobalNrecs);
    }
  }
}

// -----------------------------------------------------------------------------

void testIndexRecnum() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test configuration which holds the expected data.
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);
    oops::Log::debug() << "Test data configuration: " << testConfig << std::endl;

    // Get the index and recnum vectors from the obs space
    std::vector<std::size_t> Index = Test_::obspace(jj).index();
    std::vector<std::size_t> Recnum = Test_::obspace(jj).recnum();

    // Get the expected index and recnum vectors from the obspace object's configuration

    int MyMpiSize = Test_::obspace(jj).comm().size();
    int MyMpiRank = Test_::obspace(jj).comm().rank();

    std::string MyPath = "mpi size" + std::to_string(MyMpiSize) +
                               ".rank" + std::to_string(MyMpiRank);
    std::vector<std::size_t> ExpectedIndex = testConfig.getUnsignedVector(MyPath + ".index");
    std::vector<std::size_t> ExpectedRecnum = testConfig.getUnsignedVector(MyPath + ".recnum");
    eckit::LocalConfiguration recidxTestConfig;
    testConfig.get(MyPath + ".recidx", recidxTestConfig);

    oops::Log::debug() << "Index, ExpectedIndex: " << Index << ", "
                       << ExpectedIndex << std::endl;
    oops::Log::debug() << "Recnum, ExpectedRecnum: " << Recnum << ", "
                       << ExpectedRecnum << std::endl;

    EXPECT(Index == ExpectedIndex);
    EXPECT(Recnum == ExpectedRecnum);

    // check that the recidx data structure got initialized properly
    oops::Log::debug() << "recidxTestConfig: " << recidxTestConfig << std::endl;
    std::vector<std::size_t> RecIdxRecNums = Test_::obspace(jj).recidx_all_recnums();
    for (std::size_t i = 0; i < RecIdxRecNums.size(); ++i) {
        std::size_t RecNum = RecIdxRecNums[i];
        std::string TestConfigKey = "rec" + std::to_string(RecNum);
        std::vector<std::size_t> ExpectedRecIdxVector =
            recidxTestConfig.getUnsignedVector(TestConfigKey);
        std::vector<std::size_t> RecIdxVector = Test_::obspace(jj).recidx_vector(RecNum);

        oops::Log::debug() << "RecIdxVector, ExpectedRecIdxVector: "
                           << RecIdxVector << ", " << ExpectedRecIdxVector << std::endl;
        EXPECT(RecIdxVector == ExpectedRecIdxVector);
    }
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

class ObsSpaceIndexRecnum : public oops::Test {
 public:
  ObsSpaceIndexRecnum() {}
  virtual ~ObsSpaceIndexRecnum() {}
 private:
  std::string testid() const override {return "test::ObsSpaceIndexRecnum<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpaceIndexRecnum/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("ioda/ObsSpaceIndexRecnum/testIndexRecnum")
      { testIndexRecnum(); });
    ts.emplace_back(CASE("ioda/ObsSpaceIndexRecnum/testCleanup")
      { testCleanup(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEINDEXRECNUM_H_
