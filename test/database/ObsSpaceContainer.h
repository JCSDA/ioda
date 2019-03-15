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

#include <cmath>
#include <string>
#include <tuple>
#include <typeinfo>

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

void testGrpVarIter() {
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

  typedef std::tuple<std::string, std::string, std::string> VarDescrip;
  std::set<VarDescrip> VarInfo;

  for(std::size_t i = 0; i < Variables.size(); i++) {
    std::string VarName = Variables[i];
    std::string GroupName = Groups[i];
    std::string VarTypeName = DataTypes[i];

    VarInfo.emplace(std::make_tuple(GroupName, VarName, VarTypeName));

    // Read the var values from the config file. The ith variable has its values
    // in the sub-keyword "var" + i. Eg. when i = 0, then read var0, i = 1 read var1, etc.
    const std::type_info & VarType = typeid(void);
    std::string ConfVarValues = "TestStoreLoad.var" + std::to_string(i);
    if (VarTypeName.compare("int") == 0) {
      std::vector<int> StoreData = conf.getIntVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarTypeName << ", " << StoreData << std::endl;

      std::vector<std::size_t> VarShape(1, StoreData.size());
      TestContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName.compare("float") == 0) {
      std::vector<float> StoreData = conf.getFloatVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarTypeName << ", " << StoreData << std::endl;

      std::vector<std::size_t> VarShape(1, StoreData.size());
      TestContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName.compare("string") == 0) {
      std::vector<std::string> StoreData = conf.getStringVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarTypeName << ", " << StoreData << std::endl;

      std::vector<std::size_t> VarShape(1, StoreData.size());
      TestContainer->StoreToDb(GroupName, VarName, VarShape, StoreData);
    } else {
      oops::Log::debug() << "test::ObsSpaceContainer::testGrpVarIter: "
                         << "container only supports data types int, float and string."
                         << std::endl;
    }
  }

  for (std::set<VarDescrip>::iterator idesc = VarInfo.begin(); idesc != VarInfo.end(); idesc++) {
    std::cout << "DEBUG: VarInfo: G, V, T: " << std::get<0>(*idesc) << ", "
              << std::get<1>(*idesc) << ", " << std::get<2>(*idesc) << std::endl;
  }

  // Walk through the container using the group, var iterators and check if all of
  // the expected GroupName, VarName combinations got in.
  std::set<VarDescrip> TestVarInfo;
  for (ObsSpaceContainer::VarIter ivar = TestContainer->var_iter_begin();
            ivar != TestContainer->var_iter_end(); ivar++) {
    std::string TestVarTypeName("void");
    if (TestContainer->var_iter_type(ivar) == typeid(int)) {
      TestVarTypeName = "int";
    } else if (TestContainer->var_iter_type(ivar) == typeid(float)) {
      TestVarTypeName = "float";
    } else if (TestContainer->var_iter_type(ivar) == typeid(char)) {
      TestVarTypeName = "string";
    }
    TestVarInfo.emplace(std::make_tuple(TestContainer->var_iter_gname(ivar),
                 TestContainer->var_iter_vname(ivar), TestVarTypeName));
  }

  for (std::set<VarDescrip>::iterator idesc = TestVarInfo.begin();
                                      idesc != TestVarInfo.end(); idesc++) {
    std::cout << "DEBUG: TestVarInfo: G, V, T: " << std::get<0>(*idesc) << ", "
              << std::get<1>(*idesc) << ", " << std::get<2>(*idesc) << std::endl;
  }

  BOOST_CHECK(TestVarInfo == VarInfo);
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
    std::string VarTypeName = DataTypes[i];

    // Read the var values from the config file. The ith variable has its values
    // in the sub-keyword "var" + i. Eg. when i = 0, then read var0, i = 1 read var1, etc.
    std::string ConfVarValues = "TestStoreLoad.var" + std::to_string(i);
    if (VarTypeName.compare("int") == 0) {
      std::vector<int> StoreData = conf.getIntVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarTypeName << ", " << StoreData << std::endl;
    } else if (VarTypeName.compare("float") == 0) {
      std::vector<float> StoreData = conf.getFloatVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarTypeName << ", " << StoreData << std::endl;
    } else if (VarTypeName.compare("string") == 0) {
      std::vector<std::string> StoreData = conf.getStringVector(ConfVarValues);
      std::cout << "DEBUG: G, V, Type, Vals: " << GroupName << ", " << VarName << ", "
                << VarTypeName << ", " << StoreData << std::endl;
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

    ts->add(BOOST_TEST_CASE(&testGrpVarIter));
    ts->add(BOOST_TEST_CASE(&testConstructor));
    ts->add(BOOST_TEST_CASE(&testStoreLoad));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
