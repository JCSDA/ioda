/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACECONTDA_H_
#define TEST_IODA_OBSSPACECONTDA_H_

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
#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"
#include "IodaTestUtils.h"

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
      applyContainerDefault(::test::TestEnvironment::config(), obsconf);
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, oops::mpi::world(),
                                                               timeWindow, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testShiftWindow() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    ObsSpace &odb = Test_::obspace(jj);
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);

    // Initial check
    EXPECT_EQUAL(odb.globalNumLocs(), testConfig.getUnsigned("after constructor.gnlocs"));

    // Create an ObsVector and ObsDataVector to check robust reduce (Issue 1 & ASSERT fix)
    ioda::ObsVector vec(odb, "ObsValue");
    ioda::ObsDataVector<int> flags(odb, odb.obsvariables(), "");

    std::vector<eckit::LocalConfiguration> shiftSequence;
    testConfig.get("shift sequence", shiftSequence);

    for (const auto & shiftConf : shiftSequence) {
      oops::Log::info() << "Shifting window: " << shiftConf << std::endl;
      // This calls updateObsSpace which now includes syncAppend() and reduce()
      odb.updateObsSpace(shiftConf);

      // Check global nlocs after reduce + append
      EXPECT_EQUAL(odb.globalNumLocs(), shiftConf.getUnsigned("expected gnlocs"));

      // Check that ObsVector also has correct size (triggers internal reduce)
      // This verifies that syncAppend() worked and we didn't hit the ASSERT
      EXPECT_EQUAL(vec.nlocs(), odb.nlocs());
      EXPECT_EQUAL(vec.space().globalNumLocs(), shiftConf.getUnsigned("expected gnlocs"));

      // Check that ObsDataVector also has correct size
      EXPECT_EQUAL(flags.nlocs(), odb.nlocs());
    }
  }
}

// -----------------------------------------------------------------------------

class ObsSpaceContDA : public oops::Test {
 public:
  ObsSpaceContDA() {}
  virtual ~ObsSpaceContDA() {}
 private:
  std::string testid() const override {return "test::ObsSpaceContDA<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpaceContDA/testShiftWindow")
      { testShiftWindow(); });
  }

  void clear() const override {
    typedef ObsSpaceTestFixture Test_;
    Test_::cleanup();
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACECONTDA_H_
