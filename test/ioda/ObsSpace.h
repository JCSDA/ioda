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

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"
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
      eckit::LocalConfiguration obsconf(conf[jj], "ObsSpace");
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, bgn, end));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ObsSpaceTestFixture Test_;

  const eckit::LocalConfiguration obsconf(::test::TestEnvironment::config(), "Observations");
  std::vector<eckit::LocalConfiguration> conf;
  obsconf.get("ObsTypes", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Get the numbers of locations (nlocs) from the obspace object
    std::size_t Nlocs = Test_::obspace(jj).nlocs();

    // Get the expected nlocs from the obspace object's configuration
    int ExpectedNlocs = conf[jj].getInt("ObsSpace.TestData.nlocs");

    EXPECT(Nlocs == ExpectedNlocs);
  }
}

// -----------------------------------------------------------------------------

void testGetDb() {
  typedef ObsSpaceTestFixture Test_;

  const eckit::LocalConfiguration obsconf(::test::TestEnvironment::config(), "Observations");
  std::vector<eckit::LocalConfiguration> conf;
  obsconf.get("ObsTypes", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace * Odb = &(Test_::obspace(jj));
    const eckit::LocalConfiguration dataconf(conf[jj], "ObsSpace.TestData");

    // Read in the variable names and expected norm values from the configuration
    std::vector<std::string> GroupNames = dataconf.getStringVector("groups");
    std::vector<std::string> VarNames = dataconf.getStringVector("variables");
    std::vector<double> ExpectedVnorms = dataconf.getDoubleVector("norms");
    double Tol = dataconf.getDouble("tolerance");

    std::size_t Nlocs = Odb->nlocs();
    std::vector<double> TestVec(Nlocs);
    for (std::size_t i = 0; i < VarNames.size(); ++i) {
      // Read in the table, calculate the norm and compare with the expected norm.
      std::string Gname = GroupNames[i];
      if (Gname == "NoGroup") {
        Gname = "";
      }

      double Vnorm = 0.0;
      Odb->get_db(Gname, VarNames[i], Nlocs, TestVec.data());

      // Calculate the norm of the vector
      for (std::size_t j = 0; j < Nlocs; ++j) {
        Vnorm += pow(TestVec[j], 2.0);
      }
      Vnorm = sqrt(Vnorm);

      EXPECT(oops::is_close(Vnorm, ExpectedVnorms[i], Tol));
    }
  }
}

// -----------------------------------------------------------------------------

void testPutDb() {
  typedef ObsSpaceTestFixture Test_;

  std::string VarName("DummyVar");

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace * Odb = &(Test_::obspace(jj));

    // Create a dummy vector to put into the database
    // Load up the vector with contrived data, put the vector then
    // get the vector and see if the contrived data made it through.
    std::size_t Nlocs = Odb->nlocs();
    std::vector<double> TestVec(Nlocs);
    std::vector<double> ExpectedVec(Nlocs);

    for (std::size_t i = 0; i < Nlocs; ++i) {
      ExpectedVec[i] = double(i);
    }

    // Put the vector into the database. Then read the vector back from the database
    // and compare to the original
    Odb->put_db("MetaData", VarName, Nlocs, ExpectedVec.data());
    Odb->get_db("MetaData", VarName, Nlocs, TestVec.data());

    bool VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (int(ExpectedVec[i]) == int(TestVec[i]));
    }

    EXPECT(VecMatch);
  }
}

// -----------------------------------------------------------------------------

void testWriteableGroup() {
  typedef ObsSpaceTestFixture Test_;

  std::string VarName("DummyVar");

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {

    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace * Odb = &(Test_::obspace(jj));

    // Create a dummy vector to put into the database
    // All rows read from the input file should be read only.
    // All rows added since the read of the input file should be writeable.
    std::size_t Nlocs = Odb->nlocs();
    std::vector<double> TestVec(Nlocs);
    std::vector<double> ExpectedVec(Nlocs);

    for (std::size_t i = 0; i < Nlocs; ++i) {
      ExpectedVec[i] = double(i);
    }

    // Put the vector into the database. Then read the vector back from the database
    // and compare to the original
    Odb->put_db("TestGroup", VarName, Nlocs, ExpectedVec.data());
    Odb->get_db("TestGroup", VarName, Nlocs, TestVec.data());

    bool VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (int(ExpectedVec[i]) == int(TestVec[i]));
    }
    EXPECT(VecMatch);

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
    EXPECT(VecMatch);
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
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpace/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testGetDb")
      { testGetDb(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testPutDb")
      { testPutDb(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testWriteableGroup")
      { testWriteableGroup(); });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
