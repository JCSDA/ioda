/*
 * (C) Copyright 2018-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACE_H_
#define TEST_IODA_OBSSPACE_H_

#include <cmath>
#include <set>
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

#include "ioda/distribution/Accumulator.h"
#include "ioda/distribution/DistributionUtils.h"
#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"

namespace eckit {
  // Don't use the contracted output for these types: the current implementation works only
  // with integer types.
  template <> struct VectorPrintSelector<float> { typedef VectorPrintSimple selector; };
  template <> struct VectorPrintSelector<util::DateTime> { typedef VectorPrintSimple selector; };
}  // namespace eckit

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

class ObsSpaceTestFixture : private boost::noncopyable {
 public:
  static ioda::ObsSpace & obspace(const std::size_t ii) {
    return *getInstance().ospaces_.at(ii);
  }
  static const eckit::LocalConfiguration & config(const std::size_t ii) {
    return getInstance().configs_.at(ii);
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
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    ::test::TestEnvironment::config().get("observations", configs_);

    for (std::size_t jj = 0; jj < configs_.size(); ++jj) {
      eckit::LocalConfiguration obsconf(configs_[jj], "obs space");
      ioda::ObsTopLevelParameters obsparams;
      obsparams.validateAndDeserialize(obsconf);
      boost::shared_ptr<ioda::ObsSpace> tmp(new ioda::ObsSpace(obsparams, oops::mpi::world(),
                                                               bgn, end, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<eckit::LocalConfiguration> configs_;
  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the obs space and test data configurations
    eckit::LocalConfiguration obsConfig;
    eckit::LocalConfiguration testConfig;
    conf[jj].get("obs space", obsConfig);
    conf[jj].get("test data", testConfig);

    std::string DistMethod = obsConfig.getString("distribution", "RoundRobin");

    const ObsSpace &odb = Test_::obspace(jj);

    // Get the numbers of locations (nlocs) from the ObsSpace object
    std::size_t GlobalNlocs = odb.globalNumLocs();
    std::size_t Nlocs = odb.nlocs();
    std::size_t Nvars = odb.nvars();

    // Get the purturbation seed from the ObsSpace object
    int obsPertSeed = odb.params().obsPertSeed();

    // Get the expected nlocs from the obspace object's configuration
    std::size_t ExpectedGlobalNlocs = testConfig.getUnsigned("nlocs");
    std::size_t ExpectedNvars = testConfig.getUnsigned("nvars");

    // Get the expected purturbation seed from the config object
    int ExpectedObsPertSeed = testConfig.getUnsigned("obs perturbations seed");

    // Get the obs grouping/sorting parameters from the ObsSpace object
    std::vector<std::string> ObsGroupVars = odb.obs_group_vars();
    std::string ObsSortVar = odb.obs_sort_var();
    std::string ObsSortOrder = odb.obs_sort_order();

    // Get the expected obs grouping/sorting parameters from the configuration
    std::vector<std::string> ExpectedObsGroupVars =
      testConfig.getStringVector("expected group variables");
    std::string ExpectedObsSortVar = testConfig.getString("expected sort variable");
    std::string ExpectedObsSortOrder = testConfig.getString("expected sort order");

    oops::Log::debug() << "GlobalNlocs, ExpectedGlobalNlocs: " << GlobalNlocs << ", "
                       << ExpectedGlobalNlocs << std::endl;
    oops::Log::debug() << "Nvars, ExpectedNvars: " << Nvars << ", "
                       << ExpectedNvars << std::endl;
    // records are ambigious for halo distribution
    // e.g. consider airplane (a single record in round robin) flying accros the globe
    // for Halo distr this record will be considered unique on each PE
    if (DistMethod != "Halo") {
      std::size_t NRecs = 0;
      std::set<std::size_t> recIndices;
      auto accumulator = odb.distribution()->createAccumulator<std::size_t>();
      for (std::size_t loc = 0; loc < Nlocs; ++loc) {
        if (bool isNewRecord = recIndices.insert(odb.recnum()[loc]).second) {
          accumulator->addTerm(loc, 1);
          ++NRecs;
        }
      }
      std::size_t ExpectedNRecs = odb.nrecs();
      EXPECT_EQUAL(NRecs, ExpectedNRecs);

      // Calculate the global number of unique records
      std::size_t GlobalNRecs = accumulator->computeResult();
      std::size_t ExpectedGlobalNrecs = testConfig.getUnsigned("nrecs");
      EXPECT_EQUAL(GlobalNRecs, ExpectedGlobalNrecs);
    }

    oops::Log::debug() << "ObsGroupVars, ExpectedObsGroupVars: " << ObsGroupVars << ", "
                       << ExpectedObsGroupVars << std::endl;
    oops::Log::debug() << "ObsSortVar, ExpectedObsSortVar: " << ObsSortVar << ", "
                       << ExpectedObsSortVar << std::endl;
    oops::Log::debug() << "ObsSortOrder, ExpectedObsSortOrder: " << ObsSortOrder << ", "
                       << ExpectedObsSortOrder << std::endl;

    // get the standard nlocs and nchans dimension names and compare with expected values
    std::string nlocsName = odb.get_dim_name(ioda::ObsDimensionId::Nlocs);
    std::string nchansName = odb.get_dim_name(ioda::ObsDimensionId::Nchans);

    EXPECT(GlobalNlocs == ExpectedGlobalNlocs);
    EXPECT(Nvars == ExpectedNvars);

    EXPECT(obsPertSeed == ExpectedObsPertSeed);

    EXPECT(ObsGroupVars == ExpectedObsGroupVars);
    EXPECT(ObsSortVar == ExpectedObsSortVar);
    EXPECT(ObsSortOrder == ExpectedObsSortOrder);

    EXPECT(nlocsName == "nlocs");
    EXPECT(nchansName == "nchans");

    EXPECT(odb.get_dim_id("nlocs") == ioda::ObsDimensionId::Nlocs);
    EXPECT(odb.get_dim_id("nchans") == ioda::ObsDimensionId::Nchans);
  }
}

// -----------------------------------------------------------------------------

void testGetDb() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the obs space and test data configurations
    eckit::LocalConfiguration obsConfig;
    eckit::LocalConfiguration testConfig;
    conf[jj].get("obs space", obsConfig);
    conf[jj].get("test data", testConfig);

    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace * Odb = &(Test_::obspace(jj));
    std::size_t Nlocs = Odb->nlocs();

    // Get the variables section from the test data and perform checks accordingly
    std::vector<eckit::LocalConfiguration> varconf =
                            testConfig.getSubConfigurations("variables for get test");
    double Tol = testConfig.getDouble("tolerance");
    for (std::size_t i = 0; i < varconf.size(); ++i) {
      // Read in the variable group, name and expected norm values from the configuration
      std::string VarName = varconf[i].getString("name");
      std::string GroupName = varconf[i].getString("group");
      std::string VarType = varconf[i].getString("type");
      bool SkipDerived = varconf[i].getBool("skip derived", false);

      // Do different checks according to type
      if (VarType == "float") {
        // Check if the variable exists
        EXPECT(Odb->has(GroupName, VarName, SkipDerived));

        // Check the type from ObsSpace
        ObsDtype VarDataType = Odb->dtype(GroupName, VarName, SkipDerived);
        EXPECT(VarDataType == ObsDtype::Float);

        // Check auto-conversion to double from ObsSpace float
        // Check the norm
        std::vector<double> TestVec(Nlocs);
        Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived);

        // Calculate the norm of the vector
        double ExpectedVnorm = varconf[i].getDouble("norm");
        double Vnorm = dotProduct(*Odb->distribution(), 1, TestVec, TestVec);
        Vnorm = sqrt(Vnorm);

        EXPECT(oops::is_close(Vnorm, ExpectedVnorm, Tol));
      } else if (VarType == "integer") {
        // Check if the variable exists
        EXPECT(Odb->has(GroupName, VarName, SkipDerived));

        // Check the type from ObsSpace
        ObsDtype VarDataType = Odb->dtype(GroupName, VarName, SkipDerived);
        EXPECT(VarDataType == ObsDtype::Integer);

        // Check the norm
        std::vector<int> TestVec(Nlocs);
        Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived);

        // Calculate the norm of the vector
        double ExpectedVnorm = varconf[i].getDouble("norm");
        double Vnorm = dotProduct(*Odb->distribution(), 1, TestVec, TestVec);
        Vnorm = sqrt(Vnorm);

        EXPECT(oops::is_close(Vnorm, ExpectedVnorm, Tol));
      } else if (VarType == "string") {
        // Check if the variable exists
        EXPECT(Odb->has(GroupName, VarName, SkipDerived));

        // Check the type from ObsSpace
        ObsDtype VarDataType = Odb->dtype(GroupName, VarName, SkipDerived);
        EXPECT(VarDataType == ObsDtype::String);

        // Check the first and last values of the vector
        std::string ExpectedFirstValue = varconf[i].getString("first value");
        std::string ExpectedLastValue = varconf[i].getString("last value");
        std::vector<std::string> TestVec(Nlocs);
        Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived);
        EXPECT(TestVec[0] == ExpectedFirstValue);
        EXPECT(TestVec[Nlocs-1] == ExpectedLastValue);
      } else if (VarType == "bool") {
        // Check if the variable exists
        EXPECT(Odb->has(GroupName, VarName, SkipDerived));

        // Check the type from ObsSpace
        const ObsDtype VarDataType = Odb->dtype(GroupName, VarName, SkipDerived);
        EXPECT(VarDataType == ObsDtype::Bool);

        // Check the first and last values of the vector
        const bool ExpectedFirstValue = varconf[i].getBool("first value");
        const bool ExpectedLastValue = varconf[i].getBool("last value");
        std::vector<bool> TestVec(Nlocs);
        Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived);
        EXPECT(TestVec[0] == ExpectedFirstValue);
        EXPECT(TestVec[Nlocs-1] == ExpectedLastValue);
      } else if (VarType == "none") {
        // Check if the variable exists
        EXPECT_NOT(Odb->has(GroupName, VarName, SkipDerived));

        // Check the type from ObsSpace
        ObsDtype VarDataType = Odb->dtype(GroupName, VarName, SkipDerived);
        EXPECT(VarDataType == ObsDtype::None);


        // A call to get_db should produce an exception
        {
          std::vector<double> TestVec(Nlocs);
          EXPECT_THROWS(Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived));
        }
        {
          std::vector<float> TestVec(Nlocs);
          EXPECT_THROWS(Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived));
        }
        {
          std::vector<int> TestVec(Nlocs);
          EXPECT_THROWS(Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived));
        }
        {
          std::vector<std::string> TestVec(Nlocs);
          EXPECT_THROWS(Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived));
        }
        {
          std::vector<util::DateTime> TestVec(Nlocs);
          EXPECT_THROWS(Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived));
        }
        {
          std::vector<bool> TestVec(Nlocs);
          EXPECT_THROWS(Odb->get_db(GroupName, VarName, TestVec, {}, SkipDerived));
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------

void testPutDb() {
  typedef ObsSpaceTestFixture Test_;

  const std::string GroupName("MetaData");

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace & Odb = Test_::obspace(jj);
    const std::size_t Nlocs = Odb.nlocs();

    // Create a dummy vector of each supported type to put into the database.
    // Load up the vector with contrived data, put the vector, then
    // get the vector and see if the contrived data made it through.

    // double
    {
      const std::string VarName("DummyDoubleVar");
      std::vector<double> TestVec(Nlocs);
      std::vector<double> ExpectedVec(Nlocs);

      for (std::size_t i = 0; i < Nlocs; ++i) {
        ExpectedVec[i] = i * 0.5;
      }

      // Put the vector into the database. Then read the vector back from the database
      // and compare to the original
      Odb.put_db(GroupName, VarName, ExpectedVec);
      Odb.get_db(GroupName, VarName, TestVec);

      EXPECT_EQUAL(ExpectedVec, TestVec);
    }

    // float
    {
      const std::string VarName("DummyFloatVar");
      std::vector<float> TestVec(Nlocs);
      std::vector<float> ExpectedVec(Nlocs);

      for (std::size_t i = 0; i < Nlocs; ++i) {
        ExpectedVec[i] = i * 0.5f;
      }

      // Put the vector into the database. Then read the vector back from the database
      // and compare to the original
      Odb.put_db(GroupName, VarName, ExpectedVec);
      Odb.get_db(GroupName, VarName, TestVec);

      EXPECT_EQUAL(ExpectedVec, TestVec);
    }

    // int
    {
      const std::string VarName("DummyIntVar");
      std::vector<int> TestVec(Nlocs);
      std::vector<int> ExpectedVec(Nlocs);

      for (std::size_t i = 0; i < Nlocs; ++i) {
        ExpectedVec[i] = i;
      }

      // Put the vector into the database. Then read the vector back from the database
      // and compare to the original
      Odb.put_db(GroupName, VarName, ExpectedVec);
      Odb.get_db(GroupName, VarName, TestVec);

      EXPECT_EQUAL(ExpectedVec, TestVec);
    }

    // string
    {
      const std::string VarName("DummyStringVar");
      std::vector<std::string> TestVec(Nlocs);
      std::vector<std::string> ExpectedVec(Nlocs);

      for (std::size_t i = 0; i < Nlocs; ++i) {
        ExpectedVec[i] = "location " + std::to_string(i);
      }

      // Put the vector into the database. Then read the vector back from the database
      // and compare to the original
      Odb.put_db(GroupName, VarName, ExpectedVec);
      Odb.get_db(GroupName, VarName, TestVec);

      EXPECT_EQUAL(ExpectedVec, TestVec);
    }

    // datetime
    {
      const std::string VarName("DummyDateTimeVar");
      std::vector<util::DateTime> TestVec(Nlocs);
      std::vector<util::DateTime> ExpectedVec(Nlocs);

      const util::DateTime start(2001, 1, 1, 0, 0, 0);
      for (std::size_t i = 0; i < Nlocs; ++i) {
        ExpectedVec[i] = start + util::Duration(i);
      }

      // Put the vector into the database. Then read the vector back from the database
      // and compare to the original
      Odb.put_db(GroupName, VarName, ExpectedVec);
      Odb.get_db(GroupName, VarName, TestVec);

      EXPECT_EQUAL(ExpectedVec, TestVec);
    }

    // bool
    {
      const std::string VarName("DummyBoolVar");
      std::vector<bool> TestVec(Nlocs);
      std::vector<bool> ExpectedVec(Nlocs);

      for (std::size_t i = 0; i < Nlocs; ++i) {
        ExpectedVec[i] = (i % 2) == 0;
      }

      // Put the vector into the database. Then read the vector back from the database
      // and compare to the original
      Odb.put_db(GroupName, VarName, ExpectedVec);
      Odb.get_db(GroupName, VarName, TestVec);

      EXPECT_EQUAL(ExpectedVec, TestVec);
    }
  }
}

// -----------------------------------------------------------------------------

void testPutGetChanSelect() {
  typedef ObsSpaceTestFixture Test_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Grab the obs space and test data configurations
    eckit::LocalConfiguration obsConfig;
    eckit::LocalConfiguration testConfig;
    conf[jj].get("obs space", obsConfig);
    conf[jj].get("test data", testConfig);

    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace * Odb = &(Test_::obspace(jj));
    std::size_t Nlocs = Odb->nlocs();

    // Get the variables section from the test data and perform checks accordingly
    std::vector<eckit::LocalConfiguration> varconf =
                            testConfig.getSubConfigurations("variables for putget test");
    for (std::size_t i = 0; i < varconf.size(); ++i) {
      // Read in the variables from the ObsSpace (get_db) and use put_db to
      // place a copy in a new group. Then read the new variable back
      // in and see if you get the same values.
      std::string VarName = varconf[i].getString("name");
      std::string GroupName = varconf[i].getString("group");
      std::vector<int> Channels = varconf[i].getIntVector("channels", { });
      oops::Log::debug() << "Reading: " << GroupName << ", " << VarName << ", "
                         << Channels << std::endl;

      // Check if the variable exists
      EXPECT(Odb->has(GroupName, VarName));

      // Check the type from ObsSpace
      ObsDtype VarDataType = Odb->dtype(GroupName, VarName);
      EXPECT(VarDataType == ObsDtype::Float);

      // Read in the variable
      std::vector<float> OrigVec(Nlocs);
      Odb->get_db(GroupName, VarName, OrigVec, Channels);

      // Write the variable into the new group
      std::string TestGroupName = GroupName + "_Test";
      std::string PutDbVarName = VarName;
      std::vector<std::string> DimList = { "nlocs" };
      if (!Channels.empty()) {
        PutDbVarName += "_" + std::to_string(Channels[0]);
        DimList.push_back("nchans");
      }
      oops::Log::debug() << "Writing: " << TestGroupName << ", " << PutDbVarName << ", "
                         << DimList << std::endl;

      Odb->put_db(TestGroupName, PutDbVarName, OrigVec, DimList);

      // Read in what was just written and compare to original data
      std::vector<float> TestVec(Nlocs);
      Odb->get_db(TestGroupName, VarName, TestVec, Channels);

      EXPECT(TestVec == OrigVec);
    }
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
      ExpectedVec[i] = static_cast<double>(i);
    }

    // Put the vector into the database. Then read the vector back from the database
    // and compare to the original
    Odb->put_db("TestGroup", VarName, ExpectedVec);
    Odb->get_db("TestGroup", VarName, TestVec);

    bool VecMatch = true;
    for (std::size_t i = 0; i < Nlocs; ++i) {
      VecMatch = VecMatch && (static_cast<int>(ExpectedVec[i]) == static_cast<int>(TestVec[i]));
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
      VecMatch = VecMatch && (static_cast<int>(ExpectedVec[i]) == static_cast<int>(TestVec[i]));
    }
    EXPECT(VecMatch);
  }
}

// -----------------------------------------------------------------------------

void testMultiDimTransfer() {
  typedef ObsSpaceTestFixture Test_;

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    // Set up a pointer to the ObsSpace object for convenience
    ioda::ObsSpace * Odb = &(Test_::obspace(jj));

    // Create a dummy array to put into the database
    // Load up the array with contrived data, put the array then
    // get the array and see if the contrived data made it through.
    // If Nchans comes back equal to zero, it means that this obs space does not
    // have an nchans dimension. In this case, this test is reduced to testing
    // a 1D vector.
    std::size_t Nlocs = Odb->nlocs();
    std::size_t Nchans = Odb->nchans();

    std::vector<int> TestValues;
    std::vector<int> ExpectedValues;
    std::vector<std::string> dimList;

    int numElements = Nlocs;
    dimList.push_back(Odb->get_dim_name(ObsDimensionId::Nlocs));
    if (Nchans > 0) {
        numElements *= Nchans;
        dimList.push_back(Odb->get_dim_name(ObsDimensionId::Nchans));
    }

    // Load up the expected values with numbers 0..n-1.
    TestValues.resize(numElements);
    ExpectedValues.resize(numElements);
    int testValue = 0;
    for (std::size_t i = 0; i < numElements; ++i) {
      ExpectedValues[i] = testValue;
      testValue++;
    }

    // Put the data into the ObsSpace, then get the data back from the ObsSpace and
    // compare to the original.
    Odb->put_db("MultiDimData", "DummyVar", ExpectedValues, dimList);
    Odb->get_db("MultiDimData", "DummyVar", TestValues, {} /*select all channels*/);
    EXPECT(TestValues == ExpectedValues);

    const int numOddChannels = Nchans/2;
    if (numOddChannels > 0) {
      // Test retrieval of only the odd channels.

      const std::vector<int>& channels = Odb->obsvariables().channels();
      ASSERT(channels.size() == Nchans);

      std::vector<int> chanSelect;
      for (int i = 0; i < numOddChannels; ++i) {
        const std::size_t channelIndex = 1 + 2 * i;
        chanSelect.push_back(channels[channelIndex]);
      }

      ExpectedValues.clear();
      for (std::size_t loc = 0; loc < Nlocs; ++loc) {
        for (int i = 0; i < numOddChannels; ++i) {
          const std::size_t channelIndex = 1 + 2 * i;
          ExpectedValues.push_back(loc * Nchans + channelIndex);
        }
      }

      Odb->get_db("MultiDimData", "DummyVar", TestValues, chanSelect);
      EXPECT_EQUAL(TestValues, ExpectedValues);
    }

    if (numOddChannels > 0) {
      // Test retrieval of a single channel using the old syntax
      // (variable name with a channel suffix)

      const std::vector<int>& channels = Odb->obsvariables().channels();
      const int channelIndex = 1;
      const int channelNumber = channels[channelIndex];

      ExpectedValues.clear();
      for (std::size_t loc = 0; loc < Nlocs; ++loc)
        ExpectedValues.push_back(loc * Nchans + channelIndex);

      Odb->get_db("MultiDimData", "DummyVar_" + std::to_string(channelNumber), TestValues);
      EXPECT_EQUAL(TestValues, ExpectedValues);
    }
  }
}

// -----------------------------------------------------------------------------

// Test the obsvariables(), initial_obsvariables() and derived_obsvariables() methods.
void testObsVariables() {
  typedef ObsSpaceTestFixture Test_;

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    const ioda::ObsSpace & Odb = Test_::obspace(jj);

    ObsTopLevelParameters obsparams;
    obsparams.validateAndDeserialize(Test_::config(jj));

    const oops::Variables & allSimVars = Odb.obsvariables();
    const oops::Variables & initialSimVars = Odb.initial_obsvariables();
    const oops::Variables & derivedSimVars = Odb.derived_obsvariables();

    EXPECT_EQUAL(initialSimVars, obsparams.simVars);
    EXPECT_EQUAL(derivedSimVars, obsparams.derivedSimVars);
    EXPECT_EQUAL(allSimVars.size(), initialSimVars.size() + derivedSimVars.size());
  }
}

// -----------------------------------------------------------------------------

// Verify that for any derived simulated variable <var> a newly created ObsSpace has a variable
// <var> in the ObsError group and that it is filled with missing values.
void testDerivedObsError() {
  typedef ObsSpaceTestFixture Test_;

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    const ioda::ObsSpace & Odb = Test_::obspace(jj);

    const oops::Variables & derivedSimVars = Odb.derived_obsvariables();
    for (size_t i = 0; i < derivedSimVars.size(); ++i) {
      EXPECT(Odb.has("ObsError", derivedSimVars[i]));
      std::vector<float> values(Odb.nlocs());
      Odb.get_db("ObsError", derivedSimVars[i], values);

      std::vector<float> expectedValues(Odb.nlocs(), util::missingValue(float()));
      EXPECT_EQUAL(values, expectedValues);
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

class ObsSpace : public oops::Test {
 public:
  ObsSpace() {}
  virtual ~ObsSpace() {}

 private:
  std::string testid() const override {return "test::ObsSpace<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpace/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testGetDb")
      { testGetDb(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testPutDb")
      { testPutDb(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testPutGetChanSelect")
      { testPutGetChanSelect(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testWriteableGroup")
      { testWriteableGroup(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testMultiDimTransfer")
      { testMultiDimTransfer(); });
    ts.emplace_back(CASE("ioda/ObsSpace/testCleanup")
      { testCleanup(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACE_H_
