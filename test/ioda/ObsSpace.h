/*
 * (C) Copyright 2009-2016 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef TEST_INTERFACE_OBSSPACE_H_
#define TEST_INTERFACE_OBSSPACE_H_

#include <string>

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "oops/runs/Test.h"
#include "test/interface/ObsTestsFixture.h"
#include "test/TestEnvironment.h"

namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ObsTestsFixture<ioda::IodaTrait> Test_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    BOOST_CHECK_EQUAL(Test_::obspace()[jj].windowStart(), Test_::tbgn());
    BOOST_CHECK_EQUAL(Test_::obspace()[jj].windowEnd(),   Test_::tend());
  }
}

// -----------------------------------------------------------------------------

class ObsSpace : public oops::Test {
 public:
  ObsSpace() {}
  virtual ~ObsSpace() {}
 private:
  std::string testid() const {return "test::ObsSpace<ioda::IodaTrait>";}

  void register_tests() const {
    boost::unit_test::test_suite * ts = BOOST_TEST_SUITE("ObsSpace");

    ts->add(BOOST_TEST_CASE(&testConstructor));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// -----------------------------------------------------------------------------

}  // namespace test

#endif  // TEST_INTERFACE_OBSSPACE_H_
