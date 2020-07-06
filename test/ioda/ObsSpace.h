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
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "test/TestEnvironment.h"

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
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsconf, oops::mpi::comm(),
                                                               bgn, end));
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
    // Get the distribution method. If the method is "InefficientDistribution", then
    // don't do the mpi allreduce calls. What is going on is that InefficientDistribution
    // is distributing all observations to all mpi tasks, so if you do the allreduce
    // calls you will overcount the sums (double count when 2 mpi tasks, triple count
    // with 3 mpi tasks, etc.)
    std::string DistMethod = conf[jj].getString("ObsSpace.distribution", "RoundRobin");

    // Get the numbers of locations (nlocs) from the ObsSpace object
    std::size_t Nlocs = Test_::obspace(jj).nlocs();
    std::size_t Nrecs = Test_::obspace(jj).nrecs();
    std::size_t Nvars = Test_::obspace(jj).nvars();
    if (DistMethod != "InefficientDistribution") {
      Test_::obspace(jj).comm().allReduceInPlace(Nlocs, eckit::mpi::sum());
      Test_::obspace(jj).comm().allReduceInPlace(Nrecs, eckit::mpi::sum());
    }

    // Get the expected nlocs from the obspace object's configuration
    std::size_t ExpectedNlocs = conf[jj].getUnsigned("ObsSpace.TestData.nlocs");
    std::size_t ExpectedNrecs = conf[jj].getUnsigned("ObsSpace.TestData.nrecs");
    std::size_t ExpectedNvars = conf[jj].getUnsigned("ObsSpace.TestData.nvars");

    // Get the obs grouping/sorting parameters from the ObsSpace object
    std::string ObsGroupVar = Test_::obspace(jj).obs_group_var();
    std::string ObsSortVar = Test_::obspace(jj).obs_sort_var();
    std::string ObsSortOrder = Test_::obspace(jj).obs_sort_order();

    // Get the expected obs grouping/sorting parameters from the configuration
    std::string ExpectedObsGroupVar = conf[jj].getString("ObsSpace.TestData.obs_group_var");
    std::string ExpectedObsSortVar = conf[jj].getString("ObsSpace.TestData.obs_sort_var");
    std::string ExpectedObsSortOrder = conf[jj].getString("ObsSpace.TestData.obs_sort_order");

    oops::Log::debug() << "Nlocs, ExpectedNlocs: " << Nlocs << ", "
                       << ExpectedNlocs << std::endl;
    oops::Log::debug() << "Nrecs, ExpectedNrecs: " << Nrecs << ", "
                       << ExpectedNrecs << std::endl;
    oops::Log::debug() << "Nvars, ExpectedNvars: " << Nvars << ", "
                       << ExpectedNvars << std::endl;

    oops::Log::debug() << "ObsGroupVar, ExpectedObsGroupVar: " << ObsGroupVar << ", "
                       << ExpectedObsGroupVar << std::endl;
    oops::Log::debug() << "ObsSortVar, ExpectedObsSortVar: " << ObsSortVar << ", "
                       << ExpectedObsSortVar << std::endl;
    oops::Log::debug() << "ObsSortOrder, ExpectedObsSortOrder: " << ObsSortOrder << ", "
                       << ExpectedObsSortOrder << std::endl;

    EXPECT(Nlocs == ExpectedNlocs);
    EXPECT(Nrecs == ExpectedNrecs);
    EXPECT(Nvars == ExpectedNvars);

    EXPECT(ObsGroupVar == ExpectedObsGroupVar);
    EXPECT(ObsSortVar == ExpectedObsSortVar);
    EXPECT(ObsSortOrder == ExpectedObsSortOrder);
  }
}

// -----------------------------------------------------------------------------

void testGetDb() {
  typedef ObsSpaceTestFixture Test_;

  const eckit::LocalConfiguration obsconf(::test::TestEnvironment::config(), "Observations");
  std::vector<eckit::LocalConfiguration> conf;
  obsconf.get("ObsTypes", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    std::string DistMethod = conf[jj].getString("ObsSpace.distribution", "RoundRobin");

    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace * Odb = &(Test_::obspace(jj));
    std::size_t Nlocs = Odb->nlocs();

    // Get the variables section from the test data and perform checks accordingly
    std::vector<eckit::LocalConfiguration> varconf =
                            conf[jj].getSubConfigurations("ObsSpace.TestData.variables");
    double Tol = conf[jj].getDouble("ObsSpace.TestData.tolerance");
    for (std::size_t i = 0; i < varconf.size(); ++i) {
      // Read in the variable group, name and expected norm values from the configuration
      std::string VarName = varconf[i].getString("name");
      std::string GroupName = varconf[i].getString("group");
      std::string VarType = varconf[i].getString("type");

      // Do different checks according to type
      if (VarType == "float") {
        // Check the type from ObsSpace
        ObsDtype VarDataType = Odb->dtype(GroupName, VarName);
        EXPECT(VarDataType == ObsDtype::Float);

        // Check auto-conversion to double from ObsSpace float
        // Check the norm
        std::vector<double> TestVec(Nlocs);
        Odb->get_db(GroupName, VarName, TestVec);

        // Calculate the norm of the vector
        double ExpectedVnorm = varconf[i].getDouble("norm");
        double Vnorm = 0.0;
        for (std::size_t j = 0; j < Nlocs; ++j) {
          Vnorm += pow(TestVec[j], 2.0);
        }
        if (DistMethod != "InefficientDistribution") {
          Test_::obspace(jj).comm().allReduceInPlace(Vnorm, eckit::mpi::sum());
        }
        Vnorm = sqrt(Vnorm);

        EXPECT(oops::is_close(Vnorm, ExpectedVnorm, Tol));
      } else if (VarType == "integer") {
        // Check the type from ObsSpace
        ObsDtype VarDataType = Odb->dtype(GroupName, VarName);
        EXPECT(VarDataType == ObsDtype::Integer);

        // Check the norm
        std::vector<int> TestVec(Nlocs);
        Odb->get_db(GroupName, VarName, TestVec);

        // Calculate the norm of the vector
        double ExpectedVnorm = varconf[i].getDouble("norm");
        double Vnorm = 0.0;
        for (std::size_t j = 0; j < Nlocs; ++j) {
          Vnorm += pow(static_cast<double>(TestVec[j]), 2.0);
        }
        if (DistMethod != "InefficientDistribution") {
          Test_::obspace(jj).comm().allReduceInPlace(Vnorm, eckit::mpi::sum());
        }
        Vnorm = sqrt(Vnorm);

        EXPECT(oops::is_close(Vnorm, ExpectedVnorm, Tol));
      } else if (VarType == "string") {
        // Check the type from ObsSpace
        ObsDtype VarDataType = Odb->dtype(GroupName, VarName);
        EXPECT(VarDataType == ObsDtype::String);

        // Check the first and last values of the vector
        std::string ExpectedFirstValue = varconf[i].getString("first_value");
        std::string ExpectedLastValue = varconf[i].getString("last_value");
        std::vector<std::string> TestVec(Nlocs);
        Odb->get_db(GroupName, VarName, TestVec);

        EXPECT(TestVec[0] == ExpectedFirstValue);
        EXPECT(TestVec[Nlocs-1] == ExpectedLastValue);
      }
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
    Odb->put_db("MetaData", VarName, ExpectedVec);
    Odb->get_db("MetaData", VarName, TestVec);

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
    Odb->put_db("TestGroup", VarName, ExpectedVec);
    Odb->get_db("TestGroup", VarName, TestVec);

    bool VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (int(ExpectedVec[i]) == int(TestVec[i]));
    }
    EXPECT(VecMatch);

    // Now update the vector with the original multiplied by 2.
    for (std::size_t i = 0; i < Nlocs; ++i) {
      ExpectedVec[i] = ExpectedVec[i] * 2;
    }

    Odb->put_db("TestGroup", VarName, ExpectedVec);
    Odb->get_db("TestGroup", VarName, TestVec);

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
