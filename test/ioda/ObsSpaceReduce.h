/*
 * (C) Copyright 2024 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEREDUCE_H_
#define TEST_IODA_OBSSPACEREDUCE_H_

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
    const std::size_t GlobalNlocs = odb.globalNumLocs();
    const std::size_t GlobalNlocsOutsideTimeWindow = odb.globalNumLocsOutsideTimeWindow();
    const bool ObsAreSorted = odb.obsAreSorted();

    // Get the expected nlocs from the obspace object's configuration
    const std::size_t ExpectedGlobalNlocs = testConfig.getUnsigned("gnlocs");
    const std::size_t ExpectedGlobalNlocsOutsideTimeWindow =
        testConfig.getUnsigned("gnlocs outside time window");
    const bool ExpectedObsAreSorted = testConfig.getBool("obs are sorted");

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
      const std::size_t GlobalNrecs = accumulator->computeResult();
      const std::size_t ExpectedGlobalNrecs = testConfig.getUnsigned("nrecs");
      EXPECT_EQUAL(GlobalNrecs, ExpectedGlobalNrecs);
    }
  }
}

// -----------------------------------------------------------------------------

void testReduce() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the test configuration which holds the expected data.
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);
    oops::Log::debug() << "Test data configuration: " << testConfig << std::endl;

    // Get the expected index and recnum vectors from the obspace object's configuration
    const int MyMpiSize = Test_::obspace(jj).comm().size();
    const int MyMpiRank = Test_::obspace(jj).comm().rank();

    // Look up config according to MPI size and rank
    const std::string MyPath = "mpi size" + std::to_string(MyMpiSize) +
                               ".rank" + std::to_string(MyMpiRank);

    // Get the reduce arguments from the test config and call the reduce function
    const std::string reduceActionString = testConfig.getString(MyPath + ".reduce.action");
    ioda::CompareAction reduceAction;
    if (reduceActionString == "equal") {
        reduceAction = ioda::CompareAction::Equal;
    } else if (reduceActionString == "not equal") {
        reduceAction = ioda::CompareAction::NotEqual;
    } else if (reduceActionString == "greater than") {
        reduceAction = ioda::CompareAction::GreaterThan;
    } else if (reduceActionString == "less than") {
        reduceAction = ioda::CompareAction::LessThan;
    } else if (reduceActionString == "greater than or equal") {
        reduceAction = ioda::CompareAction::GreaterThanOrEqual;
    } else if (reduceActionString == "less than or equal") {
        reduceAction = ioda::CompareAction::LessThanOrEqual;
    } else {
        std::string errMsg = std::string("Unrecognized reduce action: ") + reduceActionString +
            std::string("\nMust use one of: 'equal', 'greater than', 'less than', ") +
            std::string("'greater than or equal' or 'less than or equal'");
        throw ioda::Exception(errMsg.c_str(), ioda_Here());
    }
    const int reduceThreshold = testConfig.getInt(MyPath + ".reduce.threshold");
    const std::vector<int> reduceCheckVector =
        testConfig.getIntVector(MyPath + ".reduce.check vector");

    // Test that ObsVectors and ObsDataVectors created prior to reduce are reduced correctly
    ioda::ObsVector vec_pre(Test_::obspace(jj), "ObsValue");
    ioda::ObsVector vec_pre_copy(vec_pre);
    ioda::ObsVector vec_pre_moved(vec_pre);
    ioda::ObsVector vec_pre_move(std::move(vec_pre_moved));
    ioda::ObsDataVector<double> obsvec_pre(Test_::obspace(jj), vec_pre.varnames(), "ObsValue");
    ioda::ObsDataVector<double> obsvec_pre_copy(obsvec_pre);
    ioda::ObsDataVector<double> obsvec_pre_moved(obsvec_pre);
    ioda::ObsDataVector<double> obsvec_pre_move(std::move(obsvec_pre_moved));
    {
      // Test that ObsVectors and ObsDataVectors associated with ObsSpace get de-associated
      // correctly when going out of scope
      ioda::ObsVector vec_pre_local(Test_::obspace(jj), "ObsValue");
      ioda::ObsVector vec_pre_local_copy(vec_pre_copy);
      ioda::ObsVector vec_pre_local_move(std::move(vec_pre_local));
      ioda::ObsDataVector<float> obsvec_pre_local(vec_pre);
      ioda::ObsDataVector<float> obsvec_pre_local_move(std::move(obsvec_pre_local));
    }
    oops::Log::debug() << "ObsVector before reduce: " << vec_pre << std::endl;
    oops::Log::debug() << "ObsDataVector before reduce: " << obsvec_pre << std::endl;
    Test_::obspace(jj).reduce(reduceAction, reduceThreshold, reduceCheckVector);
    // Test that ObsVectors and ObsDataVectors created after reduce use the reduced data
    ioda::ObsVector vec_post(Test_::obspace(jj), "ObsValue");
    ioda::ObsDataVector<double> obsvec_post(Test_::obspace(jj), vec_post.varnames(), "ObsValue");
    // Check that the nlocs and nrecs have been properly adjusted
    const std::size_t ExpectedNlocs = testConfig.getUnsigned(MyPath + ".nlocs");
    const std::size_t ExpectedNrecs = testConfig.getUnsigned(MyPath + ".nrecs");
    const std::size_t ExpectedGnlocs = testConfig.getUnsigned(MyPath + ".gnlocs");
    const std::size_t Nlocs = Test_::obspace(jj).nlocs();
    const std::size_t Nrecs = Test_::obspace(jj).nrecs();
    const std::size_t Gnlocs = Test_::obspace(jj).globalNumLocs();
    oops::Log::debug() << "Nlocs, ExpectedNlocs: " << Nlocs << ", "
                       << ExpectedNlocs << std::endl;
    oops::Log::debug() << "Nrecs, ExpectedNrecs: " << Nrecs << ", "
                       << ExpectedNrecs << std::endl;
    oops::Log::debug() << "Gnlocs, ExpectedGnlocs: " << Gnlocs << ", "
                       << ExpectedGnlocs << std::endl;
    oops::Log::debug() << "ObsVector after reduce (created before reduce): " << vec_pre
                       << std::endl;
    oops::Log::debug() << "ObsVector after reduce (copy-created before reduce): " << vec_pre_copy
                       << std::endl;
    oops::Log::debug() << "ObsVector after reduce (move-created before reduce): " << vec_pre_move
                       << std::endl;
    oops::Log::debug() << "ObsVector after reduce (created after reduce): " << vec_post
                       << std::endl;
    oops::Log::debug() << "ObsDataVector after reduce (created before reduce): " << obsvec_pre
                       << std::endl;
    oops::Log::debug() << "ObsDataVector after reduce (copy-created before reduce): "
                       << obsvec_pre_copy << std::endl;
    oops::Log::debug() << "ObsDataVector after reduce (move-created before reduce): "
                       << obsvec_pre_move << std::endl;
    oops::Log::debug() << "ObsDataVector after reduce (created after reduce): " << obsvec_post
                       << std::endl;
    EXPECT(Nlocs == ExpectedNlocs);
    EXPECT(Nrecs == ExpectedNrecs);
    EXPECT(Gnlocs == ExpectedGnlocs);
    EXPECT(vec_pre.nlocs() == ExpectedNlocs);
    EXPECT(vec_pre_copy.nlocs() == ExpectedNlocs);
    EXPECT(vec_pre_moved.nlocs() == 0);
    EXPECT(vec_pre_move.nlocs() == ExpectedNlocs);
    EXPECT(vec_post.nlocs() == ExpectedNlocs);
    EXPECT(obsvec_pre.nlocs() == ExpectedNlocs);
    EXPECT(obsvec_pre_copy.nlocs() == ExpectedNlocs);
    EXPECT(obsvec_pre_moved.nlocs() == 0);
    EXPECT(obsvec_pre_move.nlocs() == ExpectedNlocs);
    EXPECT(obsvec_post.nlocs() == ExpectedNlocs);
    // Check that the vectors created before and after the reduce are the same.
    vec_pre -= vec_post;
    EXPECT(vec_pre.rms() == 0.0);
    vec_pre_moved = std::move(vec_post);
    obsvec_pre_moved = std::move(obsvec_post);
    oops::Log::debug() << "ObsVector after reduce (move-assigned after reduce): " << vec_pre_moved
                       << std::endl;
    oops::Log::debug() << "ObsDataVector after reduce (move-assigned after reduce): "
                       << obsvec_pre_moved << std::endl;
    EXPECT(vec_post.nlocs() == 0);
    EXPECT(vec_pre_moved.nlocs() == ExpectedNlocs);
    EXPECT(obsvec_post.nlocs() == 0);
    EXPECT(obsvec_pre_moved.nlocs() == ExpectedNlocs);

    // Check that the index and recnum vectors have been properly adjusted
    const std::vector<std::size_t> ExpectedIndex =
        testConfig.getUnsignedVector(MyPath + ".index");
    const std::vector<std::size_t> ExpectedRecnum =
        testConfig.getUnsignedVector(MyPath + ".recnum");
    eckit::LocalConfiguration recidxTestConfig;
    testConfig.get(MyPath + ".recidx", recidxTestConfig);

    // Get the index and recnum vectors from the obs space
    const std::vector<std::size_t> Index = Test_::obspace(jj).index();
    const std::vector<std::size_t> Recnum = Test_::obspace(jj).recnum();

    oops::Log::debug() << "Index, ExpectedIndex: " << Index << ", "
                       << ExpectedIndex << std::endl;
    oops::Log::debug() << "Recnum, ExpectedRecnum: " << Recnum << ", "
                       << ExpectedRecnum << std::endl;

    EXPECT(Index == ExpectedIndex);
    EXPECT(Recnum == ExpectedRecnum);

    // check that the recidx data structure got adjusted properly
    oops::Log::debug() << "recidxTestConfig: " << recidxTestConfig << std::endl;
    const std::vector<std::size_t> RecIdxRecNums = Test_::obspace(jj).recidx_all_recnums();
    for (std::size_t i = 0; i < RecIdxRecNums.size(); ++i) {
        const std::size_t RecNum = RecIdxRecNums[i];
        const std::string TestConfigKey = "rec" + std::to_string(RecNum);
        const std::vector<std::size_t> ExpectedRecIdxVector =
            recidxTestConfig.getUnsignedVector(TestConfigKey);
        const std::vector<std::size_t> RecIdxVector = Test_::obspace(jj).recidx_vector(RecNum);

        oops::Log::debug() << "RecNum -> RecIdxVector, ExpectedRecIdxVector: "
                           << RecNum << " -> " << RecIdxVector << ", "
                           << ExpectedRecIdxVector << std::endl;
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

class ObsSpaceReduce : public oops::Test {
 public:
  ObsSpaceReduce() {}
  virtual ~ObsSpaceReduce() {}
 private:
  std::string testid() const override {return "test::ObsSpaceReduce<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpaceReduce/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("ioda/ObsSpaceReduce/testReduce")
      { testReduce(); });
    ts.emplace_back(CASE("ioda/ObsSpaceReduce/testCleanup")
      { testCleanup(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEREDUCE_H_
