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

#include "eckit/config/LocalConfiguration.h"
#include "eckit/mpi/Comm.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "test/TestEnvironment.h"

#include "database/ObsSpaceContainer.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::unique_ptr<ioda::ObsSpaceContainer> TestContainer;

  // Try constructing and destructing
  TestContainer.reset(new ioda::ObsSpaceContainer());
  BOOST_CHECK(TestContainer.get());

  TestContainer.reset();
  BOOST_CHECK(!TestContainer.get());
  }

// -----------------------------------------------------------------------------

void testStoreLoad() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::unique_ptr<ioda::ObsSpaceContainer> TestContainer;

  // Instantiate a container
  TestContainer.reset(new ioda::ObsSpaceContainer());
  BOOST_CHECK(TestContainer.get());

  // Try storing the variables from the YAML into the container, then load them
  // from the containter into new variables, and then check that they match.
  std::vector<std::string> Variables = conf.getStringVector("TestStoreLoad.variables");
  std::vector<std::string> Groups = conf.getStringVector("TestStoreLoad.groups");
  std::vector<std::string> DataTypes = conf.getStringVector("TestStoreLoad.datatypes");

  for(std::size_t i = 0; i < Variables.size(); i++) {
    std::string VarName = Variables[i];
    std::string GroupName = Groups[i];
    std::string VarType = DataTypes[i];

    // Read the var values from the config file. The ith variable has its values
    // in the sub-keyword "var" + i. Eg. when i = 0, then read var0, i = 1 read var1, etc.
    std::string ConfVarValues = "TestStoreLoad.var" + std::to_string(i);
    if (VarType.compare("int") == 0) {
      std::vector<int> StoreData = conf.getIntVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarType << ", " << StoreData << std::endl;
    } else if (VarType.compare("float") == 0) {
      std::vector<float> StoreData = conf.getFloatVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarType << ", " << StoreData << std::endl;
    } else if (VarType.compare("string") == 0) {
      std::vector<std::string> StoreData = conf.getStringVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarType << ", " << StoreData << std::endl;
    } else {
      oops::Log::debug() << "test::ObsSpaceContainer::testStoreLoad: "
                         << "container only supports data types int, float and string."
                         << std::endl;
    }
  }
}

// -----------------------------------------------------------------------------

class ObsSpaceContainer : public oops::Test {
 public:
  ObsSpaceContainer() {}
  virtual ~ObsSpaceContainer() {}
 private:
  std::string testid() const {return "test::ObsSpaceContainer";}

  void register_tests() const {
    boost::unit_test::test_suite * ts = BOOST_TEST_SUITE("ObsSpaceContainer");

    ts->add(BOOST_TEST_CASE(&testConstructor));
    ts->add(BOOST_TEST_CASE(&testStoreLoad));
//    ts->add(BOOST_TEST_CASE(&testGrpVarIter));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
