/*
 * (C) Copyright 2018 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
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
#include "oops/../test/TestEnvironment.h"

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
    util::DateTime bgn(::test::TestEnvironment::config().getString("window_begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window_end"));

    const eckit::LocalConfiguration obsconf(::test::TestEnvironment::config(), "Observations");
    std::vector<eckit::LocalConfiguration> conf;
    obsconf.get("ObsTypes", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(conf[jj], bgn, end));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ObsSpaceTestFixture Test_;

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Get the numbers of observations (nobs) and locations (nlocs) from the obspace object
    std::size_t Nobs = Test_::obspace(jj).nobs();
    std::size_t Nlocs = Test_::obspace(jj).nlocs();

    // Get the expected nobs and nlocs from the obspace object's configuration
    const eckit::Configuration & Conf(Test_::obspace(jj).config());
    int ExpectedNobs  = Conf.getInt("ObsData.ObsDataIn.metadata.nobs");
    int ExpectedNlocs = Conf.getInt("ObsData.ObsDataIn.metadata.nlocs");

    BOOST_CHECK_EQUAL(Nobs, ExpectedNobs);
    BOOST_CHECK_EQUAL(Nlocs, ExpectedNlocs);
  }
}

// -----------------------------------------------------------------------------

void testGetDb() {
  typedef ObsSpaceTestFixture Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nlocs;

  double Vnorm;
  double Tol;

  std::vector<std::string> GroupNames;
  std::vector<std::string> VarNames;
  std::vector<double> ExpectedVnorms;

  std::string Gname;

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Set up a pointer to the ObsSpace object for convenience
    Odb = &(Test_::obspace(jj));

    // Read in the variable names and expected norm values from the configuration
    const eckit::Configuration & Conf(Odb->config());
    GroupNames = Conf.getStringVector("ObsData.ObsDataIn.TestData.groups");
    VarNames = Conf.getStringVector("ObsData.ObsDataIn.TestData.variables");
    ExpectedVnorms = Conf.getDoubleVector("ObsData.ObsDataIn.TestData.norms");
    Tol = Conf.getDouble("ObsData.ObsDataIn.TestData.tolerance");

    Nlocs = Odb->nlocs();
    for (std::size_t i = 0; i < VarNames.size(); ++i) {
      // Read in the table, calculate the norm and compare with the expected norm.
      std::vector<double> TestVec(Nlocs);
      Gname = GroupNames[i];
      if (Gname == "NoGroup") {
        Gname = "";
      }
      Odb->get_db(Gname, VarNames[i], Nlocs, TestVec.data());

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
  typedef ObsSpaceTestFixture Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nlocs;
  bool VecMatch;

  std::string VarName("DummyVar");

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    Odb = &(Test_::obspace(jj));

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
    Odb->put_db("MetaData", VarName, Nlocs, ExpectedVec.data());
    Odb->get_db("MetaData", VarName, Nlocs, TestVec.data());

    VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (int(ExpectedVec[i]) == int(TestVec[i]));
    }

    BOOST_CHECK(VecMatch);
  }
}

// -----------------------------------------------------------------------------

void testWriteableGroup() {
  typedef ObsSpaceTestFixture Test_;

  ioda::ObsSpace * Odb;

  std::size_t Nlocs;
  bool VecMatch;

  std::string VarName("DummyVar");

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    Odb = &(Test_::obspace(jj));

    // Create a dummy vector to put into the database
    // All rows read from the input file should be read only.
    // All rows added since the read of the input file should be writeable.
    Nlocs = Odb->nlocs();
    std::vector<double> TestVec(Nlocs);
    std::vector<double> ExpectedVec(Nlocs);

    for (std::size_t i = 0; i < Nlocs; ++i) {
      ExpectedVec[i] = double(i);
    }

    // Put the vector into the database. Then read the vector back from the database
    // and compare to the original
    Odb->put_db("TestGroup", VarName, Nlocs, ExpectedVec.data());
    Odb->get_db("TestGroup", VarName, Nlocs, TestVec.data());

    VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (int(ExpectedVec[i]) == int(TestVec[i]));
    }
    BOOST_CHECK(VecMatch);

    // Now update the vector with the original multiplied by 2.
    for (std::size_t i = 0; i < Nlocs; ++i) {
      ExpectedVec[i] = ExpectedVec[i] * 2;
    }

    Odb->put_db("TestGroup", VarName, Nlocs, ExpectedVec.data());
    Odb->get_db("TestGroup", VarName, Nlocs, TestVec.data());
    
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
    ts->add(BOOST_TEST_CASE(&testGetDb));
    ts->add(BOOST_TEST_CASE(&testPutDb));
    ts->add(BOOST_TEST_CASE(&testWriteableGroup));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
