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

#include <string>
#include <vector>

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"
#include "oops/runs/Test.h"
#include "oops/util/dot_product.h"
#include "test/interface/ObsTestsFixture.h"
#include "test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait>  Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    boost::scoped_ptr<ObsVector_> ov(new ObsVector_(Test_::obspace()[jj].observationspace()));
    BOOST_CHECK(ov.get());

    ov.reset();
    BOOST_CHECK(!ov.get());
  }
}

// -----------------------------------------------------------------------------

void testCopyConstructor() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait>  Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    boost::scoped_ptr<ObsVector_> ov(new ObsVector_(Test_::obspace()[jj].observationspace()));

    boost::scoped_ptr<ObsVector_> other(new ObsVector_(*ov));
    BOOST_CHECK(other.get());

    other.reset();
    BOOST_CHECK(!other.get());

    BOOST_CHECK(ov.get());
  }
}

// -----------------------------------------------------------------------------

void testNotZero() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait>  Test_;
  typedef ioda::ObsVector  ObsVector_;
  const double zero = 0.0;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    ObsVector_ ov(Test_::obspace()[jj].observationspace());

    ov.random();

    const double ovov2 = dot_product(ov, ov);
    BOOST_CHECK(ovov2 > zero);

    ov.zero();

    const double zz = dot_product(ov, ov);
    BOOST_CHECK(zz == zero);
  }
}

// -----------------------------------------------------------------------------

void testRead() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait>  Test_;
  typedef ioda::ObsVector  ObsVector_;

  ioda::ObsSpace * Odb;
  double Rms;
  double ExpectedRms;
  double Tol;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    Odb = &(Test_::obspace()[jj].observationspace());
    boost::scoped_ptr<ObsVector_> ov(new ObsVector_(*Odb));

    // Grab the expected RMS value and tolerance from the obsdb_ configuration.
    const eckit::Configuration & Conf(Odb->config());
    ExpectedRms = Conf.getDouble("ObsData.ObsDataIn.rms_equiv");
    Tol = Conf.getDouble("ObsData.ObsDataIn.tolerance");

    // Read in a vector and check contents with norm function.
    ov->read("ObsValue");
    Rms = ov->rms();
    
    BOOST_CHECK_CLOSE(Rms, ExpectedRms, Tol);
  }
}

// -----------------------------------------------------------------------------

void testSave() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait>  Test_;
  typedef ioda::ObsVector  ObsVector_;

  ioda::ObsSpace * Odb;
  double Rms;
  double ExpectedRms;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    Odb = &(Test_::obspace()[jj].observationspace());
    boost::scoped_ptr<ObsVector_> ov_orig(new ObsVector_(*Odb));

    // Read in a vector and save the rms value. Then write the vector into a
    // test group, read it out of the test group and compare the rms of the
    // vector read out of the test group with that of the original.
    ov_orig->read("ObsValue");
    ExpectedRms = ov_orig->rms();
    ov_orig->save("ObsTestValue");

    boost::scoped_ptr<ObsVector_> ov_test(new ObsVector_(*ov_orig));
    ov_test->zero();
    ov_test->read("ObsTestValue");
    Rms = ov_test->rms();
    
    BOOST_CHECK_CLOSE(Rms, ExpectedRms, 1.0e-12);
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
    boost::unit_test::test_suite * ts = BOOST_TEST_SUITE("ObsVector");

    ts->add(BOOST_TEST_CASE(&testConstructor));
    ts->add(BOOST_TEST_CASE(&testCopyConstructor));
    ts->add(BOOST_TEST_CASE(&testNotZero));
    ts->add(BOOST_TEST_CASE(&testRead));
    ts->add(BOOST_TEST_CASE(&testSave));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSVECTOR_H_
