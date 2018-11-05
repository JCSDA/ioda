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
#include <cmath>

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

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait> Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nobs;
  std::size_t Nlocs;

  int ExpectedNobs;
  int ExpectedNlocs;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    BOOST_CHECK_EQUAL(Test_::obspace()[jj].windowStart(), Test_::tbgn());
    BOOST_CHECK_EQUAL(Test_::obspace()[jj].windowEnd(),   Test_::tend());

    Odb = &(Test_::obspace()[jj].observationspace());

    // Get the numbers of observations (nobs) and locations (nlocs) from the obspace object
    Nobs = Odb->nobs();
    Nlocs = Odb->nlocs();

    // Get the expected nobs and nlocs from the obspace object's configuration
    const eckit::Configuration & Conf(Odb->config());
    ExpectedNobs  = Conf.getInt("ObsData.ObsDataIn.metadata.nobs");
    ExpectedNlocs = Conf.getInt("ObsData.ObsDataIn.metadata.nlocs");

    BOOST_CHECK_EQUAL(Nobs, ExpectedNobs);
    BOOST_CHECK_EQUAL(Nlocs, ExpectedNlocs);
  }
}

// -----------------------------------------------------------------------------

void testGetObsVector() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait> Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nlocs;

  double Vnorm;
  double Tol;

  std::vector<std::string> VarNames;
  std::vector<double> ExpectedVnorms;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    Odb = &(Test_::obspace()[jj].observationspace());

    // Read in the variable names and expected norm values from the configuration
    const eckit::Configuration & Conf(Odb->config());
    VarNames = Conf.getStringVector("ObsData.ObsDataIn.TestData.variables");
    ExpectedVnorms = Conf.getDoubleVector("ObsData.ObsDataIn.TestData.norms");
    Tol = Conf.getDouble("ObsData.ObsDataIn.TestData.tolerance");

    Nlocs = Odb->nlocs();
    for (std::size_t i = 0; i < VarNames.size(); ++i) {
      // Read in the table, calculate the norm and compare with the expected norm.
      std::vector<double> TestVec(Nlocs);
      Odb->getObsVector(VarNames[i], TestVec);

      // Calculate the norm of the vector
      Vnorm = 0.0;
      for (std::size_t j = 0; j < Nlocs; ++j) {
        Vnorm += pow(TestVec[j], 2.0);
      }
      Vnorm = sqrt(Vnorm);

      BOOST_CHECK_CLOSE(Vnorm, ExpectedVnorms[i], Tol);
    }
  }
}

// -----------------------------------------------------------------------------

void testPutObsVector() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait> Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nlocs;
  bool VecMatch;

  std::string VarName("DummyVector");

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    Odb = &(Test_::obspace()[jj].observationspace());

    // Create a dummy vector to put into the database
    // Load up the vector with contrived data, put the vector then
    // get the vector and see if the contrived data made it through.
    Nlocs = Odb->nlocs();
    std::vector<double> TestVec(Nlocs);
    std::vector<double> ExpectedVec(Nlocs);

    for (std::size_t i = 0; i < Nlocs; ++i) {
      ExpectedVec[i] = double(i);
    }

    // Put the vector into the database. Then read the vector back from the database
    // and compare to the original
    Odb->putObsVector(VarName, ExpectedVec);
    Odb->getObsVector(VarName, TestVec);

    VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (int(ExpectedVec[i]) == int(TestVec[i]));
    }

    BOOST_CHECK(VecMatch);
  }
}

// -----------------------------------------------------------------------------

void testGetDb() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait> Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nlocs;

  double Vnorm;
  double Tol;

  std::vector<std::string> VarNames;
  std::vector<double> ExpectedVnorms;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    Odb = &(Test_::obspace()[jj].observationspace());

    // Read in the variable names and expected norm values from the configuration
    const eckit::Configuration & Conf(Odb->config());
    VarNames = Conf.getStringVector("ObsData.ObsDataIn.TestData.variables");
    ExpectedVnorms = Conf.getDoubleVector("ObsData.ObsDataIn.TestData.norms");
    Tol = Conf.getDouble("ObsData.ObsDataIn.TestData.tolerance");

    Nlocs = Odb->nlocs();
    for (std::size_t i = 0; i < VarNames.size(); ++i) {
      // Read in the table, calculate the norm and compare with the expected norm.
      std::vector<double> TestVec(Nlocs);
      Odb->get_db<double>("MetaData", VarNames[i], Nlocs, TestVec.data());

      // Calculate the norm of the vector
      Vnorm = 0.0;
      for (std::size_t j = 0; j < Nlocs; ++j) {
        Vnorm += pow(TestVec[j], 2.0);
      }
      Vnorm = sqrt(Vnorm);

      BOOST_CHECK_CLOSE(Vnorm, ExpectedVnorms[i], Tol);
    }
  }
}

// -----------------------------------------------------------------------------

void testPutDb() {
  typedef ::test::ObsTestsFixture<ioda::IodaTrait> Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nlocs;
  bool VecMatch;

  std::string VarName("DummyRow");

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    Odb = &(Test_::obspace()[jj].observationspace());

    // Create a dummy vector to put into the database
    // Load up the vector with contrived data, put the vector then
    // get the vector and see if the contrived data made it through.
    Nlocs = Odb->nlocs();
    std::vector<double> TestVec(Nlocs);
    std::vector<double> ExpectedVec(Nlocs);

    for (std::size_t i = 0; i < Nlocs; ++i) {
      ExpectedVec[i] = double(i);
    }

    // Put the vector into the database. Then read the vector back from the database
    // and compare to the original
    Odb->put_db<double>("MetaData", VarName, Nlocs, ExpectedVec.data());
    Odb->get_db<double>("MetaData", VarName, Nlocs, TestVec.data());

    VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (int(ExpectedVec[i]) == int(TestVec[i]));
    }

    BOOST_CHECK(VecMatch);
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
    ts->add(BOOST_TEST_CASE(&testGetObsVector));
    ts->add(BOOST_TEST_CASE(&testPutObsVector));
    ts->add(BOOST_TEST_CASE(&testGetDb));
    ts->add(BOOST_TEST_CASE(&testPutDb));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
