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

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"
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
  EXPECT(TestContainer.get());

  TestContainer.reset();
  EXPECT(!TestContainer.get());
  }

// -----------------------------------------------------------------------------

void testGrpVarIter() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::unique_ptr<ioda::ObsSpaceContainer> TestContainer;

  // Instantiate a container
  TestContainer.reset(new ioda::ObsSpaceContainer());
  EXPECT(TestContainer.get());

  // Try storing the variables from the YAML into the container, then load them
  // from the containter into new variables, and then check that they match.
  std::vector<std::string> Variables = conf.getStringVector("TestStoreLoad.variables");
  std::vector<std::string> Groups = conf.getStringVector("TestStoreLoad.groups");
  std::vector<std::string> DataTypes = conf.getStringVector("TestStoreLoad.datatypes");

  typedef std::tuple<std::string, std::string, std::string, std::vector<std::size_t>> VarDescrip;
  std::set<VarDescrip> VarInfo;

  for(std::size_t i = 0; i < Variables.size(); i++) {
    std::string VarName = Variables[i];
    std::string GroupName = Groups[i];
    std::string VarTypeName = DataTypes[i];
    std::vector<std::size_t> VarShape(1, 0);

    // Read the var values from the config file. The ith variable has its values
    // in the sub-keyword "var" + i. Eg. when i = 0, then read var0, i = 1 read var1, etc.
    const std::type_info & VarType = typeid(void);
    std::string ConfVarValues = "TestStoreLoad.var" + std::to_string(i);
    if (VarTypeName.compare("int") == 0) {
      std::vector<int> StoreData = conf.getIntVector(ConfVarValues);
      VarShape[0] = StoreData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName.compare("float") == 0) {
      std::vector<float> StoreData = conf.getFloatVector(ConfVarValues);
      VarShape[0] = StoreData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName.compare("string") == 0) {
      std::vector<std::string> StoreData = conf.getStringVector(ConfVarValues);
      VarShape[0] = StoreData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName.compare("datetime") == 0) {
      std::vector<std::string> TempStoreData = conf.getStringVector(ConfVarValues);
      std::vector<util::DateTime> StoreData(TempStoreData.size());
      for (std::size_t j = 0; j < TempStoreData.size(); j++) {
        util::DateTime TempDateTime(TempStoreData[j]);
        StoreData[j] = TempDateTime;
      }
      VarShape[0] = StoreData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else {
      oops::Log::debug() << "test::ObsSpaceContainer::testGrpVarIter: "
                         << "container only supports data types int, float and string."
                         << std::endl;
    }

    VarInfo.emplace(std::make_tuple(GroupName, VarName, VarTypeName, VarShape));
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
    } else if (TestContainer->var_iter_type(ivar) == typeid(std::string)) {
      TestVarTypeName = "string";
    } else if (TestContainer->var_iter_type(ivar) == typeid(util::DateTime)) {
      TestVarTypeName = "datetime";
    }
    TestVarInfo.emplace(std::make_tuple(TestContainer->var_iter_gname(ivar),
                 TestContainer->var_iter_vname(ivar), TestVarTypeName,
                 TestContainer->var_iter_shape(ivar)));
  }

  EXPECT(TestVarInfo == VarInfo);
}


// -----------------------------------------------------------------------------

void testStoreLoad() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::unique_ptr<ioda::ObsSpaceContainer> TestContainer;

  // Instantiate a container
  TestContainer.reset(new ioda::ObsSpaceContainer());
  EXPECT(TestContainer.get());

  // Try storing the variables from the YAML into the container, then load them
  // from the containter into new variables, and then check that they match.
  std::vector<std::string> Variables = conf.getStringVector("TestStoreLoad.variables");
  std::vector<std::string> Groups = conf.getStringVector("TestStoreLoad.groups");
  std::vector<std::string> DataTypes = conf.getStringVector("TestStoreLoad.datatypes");

  for(std::size_t i = 0; i < Variables.size(); i++) {
    std::string VarName = Variables[i];
    std::string GroupName = Groups[i];
    std::string VarTypeName = DataTypes[i];
    std::vector<std::size_t> VarShape(1, 0);

    // Read the var values from the config file. The ith variable has its values
    // in the sub-keyword "var" + i. Eg. when i = 0, then read var0, i = 1 read var1, etc.
    const std::type_info & VarType = typeid(void);
    std::string ConfVarValues = "TestStoreLoad.var" + std::to_string(i);
    if (VarTypeName.compare("int") == 0) {
      std::vector<int> ExpectedIntData = conf.getIntVector(ConfVarValues);
      VarShape[0] = ExpectedIntData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedIntData.data());

      std::vector<int> TestIntData(ExpectedIntData.size(), 0);
      TestContainer->LoadFromDb(GroupName, VarName, VarShape, TestIntData.data());
      for(std::size_t j = 0; j < TestIntData.size(); j++) {
        EXPECT(TestIntData[j] == ExpectedIntData[j]);
      }
    } else if (VarTypeName.compare("float") == 0) {
      std::vector<float> ExpectedFloatData = conf.getFloatVector(ConfVarValues);
      VarShape[0] = ExpectedFloatData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedFloatData.data());

      std::vector<float> TestFloatData(ExpectedFloatData.size(), 0.0);
      TestContainer->LoadFromDb(GroupName, VarName, VarShape, TestFloatData.data());
      for(std::size_t j = 0; j < TestFloatData.size(); j++) {
        EXPECT(TestFloatData[j] == ExpectedFloatData[j]);
      }
    } else if (VarTypeName.compare("string") == 0) {
      std::vector<std::string> ExpectedStringData = conf.getStringVector(ConfVarValues);
      VarShape[0] = ExpectedStringData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedStringData.data());

      std::vector<std::string> TestStringData(ExpectedStringData.size(), "xx");
      TestContainer->LoadFromDb(GroupName, VarName, VarShape, TestStringData.data());
      for(std::size_t j = 0; j < TestStringData.size(); j++) {
        EXPECT(TestStringData[j] == ExpectedStringData[j]);
      }
    } else if (VarTypeName.compare("datetime") == 0) {
      std::vector<std::string> DtStrings = conf.getStringVector(ConfVarValues);
      std::vector<util::DateTime> ExpectedDateTimeData(DtStrings.size());
      for (std::size_t j = 0; j < DtStrings.size(); j++) {
        util::DateTime TempDateTime(DtStrings[j]);
        ExpectedDateTimeData[j] = TempDateTime;
      }
      VarShape[0] = ExpectedDateTimeData.size();
      TestContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedDateTimeData.data());

      util::DateTime TempDt("0000-01-01T00:00:00Z");
      std::vector<util::DateTime> TestDateTimeData(ExpectedDateTimeData.size(), TempDt);
      TestContainer->LoadFromDb(GroupName, VarName, VarShape, TestDateTimeData.data());
      for(std::size_t j = 0; j < TestDateTimeData.size(); j++) {
        EXPECT(TestDateTimeData[j] == ExpectedDateTimeData[j]);
      }
    } else {
      oops::Log::debug() << "test::ObsSpaceContainer::testGrpVarIter: "
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
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("database/ObsSpaceContainer/testConstructor")
      { testConstructor; });
    ts.emplace_back(CASE("database/ObsSpaceContainer/testGrpVarIter")
      { testGrpVarIter; });
    ts.emplace_back(CASE("database/ObsSpaceContainer/testStoreLoad")
      { testStoreLoad; });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
