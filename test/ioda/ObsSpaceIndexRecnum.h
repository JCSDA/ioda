/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEINDEXRECNUM_H_
#define TEST_IODA_OBSSPACEINDEXRECNUM_H_

#include <cmath>
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
      std::string distname = obsconf.getString("distribution", "RoundRobin");
      // if Halo distribution, need to prescribe center and radius for each patch
      if (distname == "Halo") {
        const eckit::mpi::Comm & MpiComm = oops::mpi::world();
        std::size_t MyRank = MpiComm.rank();
        std::size_t nprocs = MpiComm.size();
        double centerLongitude = static_cast<double>(MyRank)*
                                 (360.0/static_cast<double>(nprocs));
        double radius = 50000000.0;  // this is big enough to hold all points on each PE
        std::vector<double> centerd{centerLongitude, 0.0};
        obsconf.set("center", centerd);
        obsconf.set("radius", radius);
      }
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, oops::mpi::world(),
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
    // The observations are distributed across processors, so Nlocs is local to a
    // processor. To get the total number of observations,
    // use ObsSpace.distribution().sum(Nlocs) to sum accoss all processors.

    // Get the numbers of locations (nlocs) from the ObsSpace object
    std::size_t Nlocs = Test_::obspace(jj).nlocspatch();
    std::size_t Nrecs = Test_::obspace(jj).nrecs();
    std::size_t Nvars = Test_::obspace(jj).nvars();
    Test_::obspace(jj).distribution().sum(Nlocs);
    Test_::obspace(jj).distribution().sum(Nrecs);
    bool ObsAreSorted = Test_::obspace(jj).obsAreSorted();

    // Get the expected nlocs from the obspace object's configuration
    std::size_t ExpectedNlocs = conf[jj].getUnsigned("obs space.test data.nlocs");
    std::size_t ExpectedNrecs = conf[jj].getUnsigned("obs space.test data.nrecs");
    std::size_t ExpectedNvars = conf[jj].getUnsigned("obs space.test data.nvars");
    bool ExpectedObsAreSorted = conf[jj].getBool("obs space.test data.obs are sorted");

    oops::Log::debug() << "Nlocs, ExpectedNlocs: " << Nlocs << ", "
                       << ExpectedNlocs << std::endl;
    oops::Log::debug() << "Nvars, ExpectedNvars: " << Nvars << ", "
                       << ExpectedNvars << std::endl;
    oops::Log::debug() << "Nrecs, ExpectedNrecs: " << Nrecs << ", "
                       << ExpectedNrecs << std::endl;
    oops::Log::debug() << "ObsAreSorted, ExpectedObsAreSorted: " << ObsAreSorted << ", "
                       << ExpectedObsAreSorted << std::endl;

    EXPECT(Nlocs == ExpectedNlocs);
    EXPECT(Nrecs == ExpectedNrecs);
    EXPECT(Nvars == ExpectedNvars);
    EXPECT(ObsAreSorted == ExpectedObsAreSorted);
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
    conf[jj].get("obs space.test data", testConfig);
    oops::Log::debug() << "Test data configuration: " << testConfig << std::endl;

    // Get the index and recnum vectors from the obs space
    std::vector<std::size_t> Index = Test_::obspace(jj).index();
    std::vector<std::size_t> Recnum = Test_::obspace(jj).recnum();

    // Get the expected index and recnum vectors from the obspace object's configuration
    // Allow number of MPI tasks to be 1, 2 or 4. Look for corresponding YAML configuration.
    eckit::LocalConfiguration recidxTestConfig;
    std::vector<std::size_t> ExpectedIndex;
    std::vector<std::size_t> ExpectedRecnum;
    int MyMpiSize = Test_::obspace(jj).comm().size();
    int MyMpiRank = Test_::obspace(jj).comm().rank();
    if (MyMpiSize == 1) {
      ExpectedIndex = testConfig.getUnsignedVector("mpi size1.rank0.index");
      ExpectedRecnum = testConfig.getUnsignedVector("mpi size1.rank0.recnum");
      testConfig.get("mpi size1.rank0.recidx", recidxTestConfig);
    } else if (MyMpiSize == 2) {
      if (MyMpiRank == 0) {
        ExpectedIndex = testConfig.getUnsignedVector("mpi size2.rank0.index");
        ExpectedRecnum = testConfig.getUnsignedVector("mpi size2.rank0.recnum");
        testConfig.get("mpi size2.rank0.recidx", recidxTestConfig);
      } else {
        ExpectedIndex = testConfig.getUnsignedVector("mpi size2.rank1.index");
        ExpectedRecnum = testConfig.getUnsignedVector("mpi size2.rank1.recnum");
        testConfig.get("mpi size2.rank1.recidx", recidxTestConfig);
      }
    } else if (MyMpiSize == 4) {
      if (MyMpiRank == 0) {
        ExpectedIndex = testConfig.getUnsignedVector("mpi size4.rank0.index");
        ExpectedRecnum = testConfig.getUnsignedVector("mpi size4.rank0.recnum");
        testConfig.get("mpi size4.rank0.recidx", recidxTestConfig);
      } else if (MyMpiRank == 1) {
        ExpectedIndex = testConfig.getUnsignedVector("mpi size4.rank1.index");
        ExpectedRecnum = testConfig.getUnsignedVector("mpi size4.rank1.recnum");
        testConfig.get("mpi size4.rank1.recidx", recidxTestConfig);
      } else if (MyMpiRank == 2) {
        ExpectedIndex = testConfig.getUnsignedVector("mpi size4.rank2.index");
        ExpectedRecnum = testConfig.getUnsignedVector("mpi size4.rank2.recnum");
        testConfig.get("mpi size4.rank2.recidx", recidxTestConfig);
      } else {
        ExpectedIndex = testConfig.getUnsignedVector("mpi size4.rank3.index");
        ExpectedRecnum = testConfig.getUnsignedVector("mpi size4.rank3.recnum");
        testConfig.get("mpi size4.rank3.recidx", recidxTestConfig);
      }
    }

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
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEINDEXRECNUM_H_
