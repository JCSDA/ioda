/*
 * (C) Copyright 2009-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef TEST_INTERFACE_OBSVECTOR_H_
#define TEST_INTERFACE_OBSVECTOR_H_

#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0  

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"
#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"
#include "oops/runs/Test.h"
#include "oops/util/dot_product.h"
#include "test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

class ObsVecTestFixture : private boost::noncopyable {
  typedef ioda::ObsSpace ObsSpace_;

 public:
  static std::vector<boost::shared_ptr<ObsSpace_> > & obspace() {return getInstance().ospaces_;}

 private:
  static ObsVecTestFixture& getInstance() {
    static ObsVecTestFixture theObsVecTestFixture;
    return theObsVecTestFixture;
  }

  ObsVecTestFixture(): ospaces_() {
    util::DateTime bgn((::test::TestEnvironment::config().getString("window_begin")));
    util::DateTime end((::test::TestEnvironment::config().getString("window_end")));

    const eckit::LocalConfiguration obsconf(::test::TestEnvironment::config(), "Observations");
    std::vector<eckit::LocalConfiguration> conf;
    obsconf.get("ObsTypes", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      eckit::LocalConfiguration obsconf(conf[jj], "ObsSpace");
      boost::shared_ptr<ObsSpace_> tmp(new ObsSpace_(obsconf, bgn, end));
      ospaces_.push_back(tmp);
      eckit::LocalConfiguration ObsDataInConf;
      obsconf.get("ObsDataIn", ObsDataInConf);
    }
  }

  ~ObsVecTestFixture() {}

  std::vector<boost::shared_ptr<ObsSpace_> > ospaces_;
};

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    std::unique_ptr<ObsVector_> ov(new ObsVector_(*Test_::obspace()[jj]));
    EXPECT(ov.get());

    ov.reset();
    EXPECT(!ov.get());
  }
}

// -----------------------------------------------------------------------------

void testCopyConstructor() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    std::unique_ptr<ObsVector_> ov(new ObsVector_(*Test_::obspace()[jj]));

    std::unique_ptr<ObsVector_> other(new ObsVector_(*ov));
    EXPECT(other.get());

    other.reset();
    EXPECT(!other.get());

    EXPECT(ov.get());
  }
}

// -----------------------------------------------------------------------------

void testNotZero() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;
  const double zero = 0.0;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    ObsVector_ ov(*Test_::obspace()[jj]);

    ov.random();

    const double ovov2 = dot_product(ov, ov);
    EXPECT(ovov2 > zero);

    ov.zero();

    const double zz = dot_product(ov, ov);
    EXPECT(zz == zero);
  }
}

// -----------------------------------------------------------------------------

void testRead() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  const eckit::LocalConfiguration obsconf(::test::TestEnvironment::config(), "Observations");
  std::vector<eckit::LocalConfiguration> conf;
  obsconf.get("ObsTypes", conf);

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    ioda::ObsSpace * Odb = Test_::obspace()[jj].get();

    // Grab the expected RMS value and tolerance from the obsdb_ configuration.
    double ExpectedRms = conf[jj].getDouble("ObsSpace.ObsDataIn.rms_equiv");
    double Tol = conf[jj].getDouble("ObsSpace.ObsDataIn.tolerance");

    // Read in a vector and check contents with norm function.
    std::unique_ptr<ObsVector_> ov(new ObsVector_(*Odb, "ObsValue"));
    double Rms = ov->rms();
    
    EXPECT(oops::is_close(Rms, ExpectedRms, Tol));
  }
}

// -----------------------------------------------------------------------------

void testSave() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  ioda::ObsSpace * Odb;
  double Rms;
  double ExpectedRms;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    Odb = Test_::obspace()[jj].get();

    // Read in a vector and save the rms value. Then write the vector into a
    // test group, read it out of the test group and compare the rms of the
    // vector read out of the test group with that of the original.
    std::unique_ptr<ObsVector_> ov_orig(new ObsVector_(*Odb, "ObsValue"));
    ExpectedRms = ov_orig->rms();
    ov_orig->save("ObsTest");

    std::unique_ptr<ObsVector_> ov_test(new ObsVector_(*Odb, "ObsTest"));
    Rms = ov_test->rms();
    
    EXPECT(oops::is_close(Rms, ExpectedRms, 1.0e-12));
  }
}

// -----------------------------------------------------------------------------

class ObsVector : public oops::Test {
 public:
  ObsVector() {}
  virtual ~ObsVector() {}
 private:
  std::string testid() const {return "test::ObsVector<ioda::IodaTrait>";}

  void register_tests() const {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

     ts.emplace_back(CASE("ioda/ObsVector/testConstructor")
      { testConstructor(); });
     ts.emplace_back(CASE("ioda/ObsVector/testCopyConstructor")
      { testCopyConstructor(); });
     ts.emplace_back(CASE("ioda/ObsVector/testNotZero")
      { testNotZero(); }); 
     ts.emplace_back(CASE("ioda/ObsVector/testRead")
      { testRead(); });  
     ts.emplace_back(CASE("ioda/ObsVector/testSave")
      { testSave(); });  
  }
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSVECTOR_H_
