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

  // There are four data types currently supported. Try instantiating one of each.
  // Data types are: int, float, std::string, util::DateTime.
  std::unique_ptr<ioda::ObsSpaceContainer<int>> TestIntContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<float>> TestFloatContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<std::string>> TestStringContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<util::DateTime>> TestDateTimeContainer;

  // Try constructing and destructing
  TestIntContainer.reset(new ioda::ObsSpaceContainer<int>());
  TestFloatContainer.reset(new ioda::ObsSpaceContainer<float>());
  TestStringContainer.reset(new ioda::ObsSpaceContainer<std::string>());
  TestDateTimeContainer.reset(new ioda::ObsSpaceContainer<util::DateTime>());
  EXPECT(TestIntContainer.get());
  EXPECT(TestFloatContainer.get());
  EXPECT(TestStringContainer.get());
  EXPECT(TestDateTimeContainer.get());

  TestIntContainer.reset();
  TestFloatContainer.reset();
  TestStringContainer.reset();
  TestDateTimeContainer.reset();
  EXPECT(!TestIntContainer.get());
  EXPECT(!TestFloatContainer.get());
  EXPECT(!TestStringContainer.get());
  EXPECT(!TestDateTimeContainer.get());
  }

// -----------------------------------------------------------------------------

void testGrpVarIter() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::unique_ptr<ioda::ObsSpaceContainer<int>> TestIntContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<float>> TestFloatContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<std::string>> TestStringContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<util::DateTime>> TestDateTimeContainer;

  // Instantiate containers
  TestIntContainer.reset(new ioda::ObsSpaceContainer<int>());
  TestFloatContainer.reset(new ioda::ObsSpaceContainer<float>());
  TestStringContainer.reset(new ioda::ObsSpaceContainer<std::string>());
  TestDateTimeContainer.reset(new ioda::ObsSpaceContainer<util::DateTime>());
  EXPECT(TestIntContainer.get());
  EXPECT(TestFloatContainer.get());
  EXPECT(TestStringContainer.get());
  EXPECT(TestDateTimeContainer.get());

  // Try storing the variables from the YAML into the container, then load them
  // from the containter into new variables, and then check that they match.
  std::vector<eckit::LocalConfiguration> VarConfig =
                                         conf.getSubConfigurations("TestStoreLoad.variables");

  typedef std::tuple<std::string, std::string, std::vector<std::size_t>> VarDescrip;
  std::set<VarDescrip> VarInfo;

  for(std::size_t i = 0; i < VarConfig.size(); i++) {
    std::string VarName = VarConfig[i].getString("name");
    std::string GroupName = VarConfig[i].getString("group");
    std::string VarTypeName = VarConfig[i].getString("type");
    std::vector<std::size_t> VarShape(1, 0);

    // Read the var values from the config file. The ith variable has its values
    // in the sub-keyword "var" + i. Eg. when i = 0, then read var0, i = 1 read var1, etc.
    if (VarTypeName == "int") {
      std::vector<int> StoreData = VarConfig[i].getIntVector("values");
      VarShape[0] = StoreData.size();
      TestIntContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName == "float") {
      std::vector<float> StoreData = VarConfig[i].getFloatVector("values");
      VarShape[0] = StoreData.size();
      TestFloatContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName == "string") {
      std::vector<std::string> StoreData = VarConfig[i].getStringVector("values");
      VarShape[0] = StoreData.size();
      TestStringContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else if (VarTypeName == "datetime") {
      std::vector<std::string> TempStoreData = VarConfig[i].getStringVector("values");
      std::vector<util::DateTime> StoreData(TempStoreData.size());
      for (std::size_t j = 0; j < TempStoreData.size(); j++) {
        util::DateTime TempDateTime(TempStoreData[j]);
        StoreData[j] = TempDateTime;
      }
      VarShape[0] = StoreData.size();
      TestDateTimeContainer->StoreToDb(GroupName, VarName, VarShape, StoreData.data());
    } else {
      oops::Log::debug() << "test::ObsSpaceContainer::testGrpVarIter: "
          << "container only supports data types int, float, string and datetime." << std::endl;
    }

    VarInfo.emplace(std::make_tuple(GroupName, VarName, VarShape));
  }

  // Walk through the container using the group, var iterators and check if all of
  // the expected GroupName, VarName combinations got in.
  std::set<VarDescrip> TestVarInfo;
  for (ObsSpaceContainer<int>::VarIter ivar = TestIntContainer->var_iter_begin();
            ivar != TestIntContainer->var_iter_end(); ivar++) {
    TestVarInfo.emplace(std::make_tuple(TestIntContainer->var_iter_gname(ivar),
                 TestIntContainer->var_iter_vname(ivar),
                 TestIntContainer->var_iter_shape(ivar)));
  }
  for (ObsSpaceContainer<float>::VarIter ivar = TestFloatContainer->var_iter_begin();
            ivar != TestFloatContainer->var_iter_end(); ivar++) {
    TestVarInfo.emplace(std::make_tuple(TestFloatContainer->var_iter_gname(ivar),
                 TestFloatContainer->var_iter_vname(ivar),
                 TestFloatContainer->var_iter_shape(ivar)));
  }
  for (ObsSpaceContainer<std::string>::VarIter ivar = TestStringContainer->var_iter_begin();
            ivar != TestStringContainer->var_iter_end(); ivar++) {
    TestVarInfo.emplace(std::make_tuple(TestStringContainer->var_iter_gname(ivar),
                 TestStringContainer->var_iter_vname(ivar),
                 TestStringContainer->var_iter_shape(ivar)));
  }
  for (ObsSpaceContainer<util::DateTime>::VarIter ivar = TestDateTimeContainer->var_iter_begin();
            ivar != TestDateTimeContainer->var_iter_end(); ivar++) {
    TestVarInfo.emplace(std::make_tuple(TestDateTimeContainer->var_iter_gname(ivar),
                 TestDateTimeContainer->var_iter_vname(ivar),
                 TestDateTimeContainer->var_iter_shape(ivar)));
  }

  EXPECT(TestVarInfo == VarInfo);
}


// -----------------------------------------------------------------------------

void testStoreLoad() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::unique_ptr<ioda::ObsSpaceContainer<int>> TestIntContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<float>> TestFloatContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<std::string>> TestStringContainer;
  std::unique_ptr<ioda::ObsSpaceContainer<util::DateTime>> TestDateTimeContainer;

  // Instantiate containers
  TestIntContainer.reset(new ioda::ObsSpaceContainer<int>());
  TestFloatContainer.reset(new ioda::ObsSpaceContainer<float>());
  TestStringContainer.reset(new ioda::ObsSpaceContainer<std::string>());
  TestDateTimeContainer.reset(new ioda::ObsSpaceContainer<util::DateTime>());
  EXPECT(TestIntContainer.get());
  EXPECT(TestFloatContainer.get());
  EXPECT(TestStringContainer.get());
  EXPECT(TestDateTimeContainer.get());

  // Try storing the variables from the YAML into the container, then load them
  // from the containter into new variables, and then check that they match.
  std::vector<eckit::LocalConfiguration> VarConfig =
                                         conf.getSubConfigurations("TestStoreLoad.variables");

  for(std::size_t i = 0; i < VarConfig.size(); i++) {
    std::string VarName = VarConfig[i].getString("name");
    std::string GroupName = VarConfig[i].getString("group");
    std::string VarTypeName = VarConfig[i].getString("type");
    std::vector<std::size_t> VarShape(1, 0);

    // Read the var values from the config file. The ith variable has its values
    // in the sub-keyword "var" + i. Eg. when i = 0, then read var0, i = 1 read var1, etc.
    if (VarTypeName == "int") {
      std::vector<int> ExpectedIntData = VarConfig[i].getIntVector("values");
      VarShape[0] = ExpectedIntData.size();
      TestIntContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedIntData.data());

      std::vector<int> TestIntData(ExpectedIntData.size(), 0);
      TestIntContainer->LoadFromDb(GroupName, VarName, VarShape, TestIntData.data());
      for(std::size_t j = 0; j < TestIntData.size(); j++) {
        EXPECT(TestIntData[j] == ExpectedIntData[j]);
      }
    } else if (VarTypeName == "float") {
      std::vector<float> ExpectedFloatData = VarConfig[i].getFloatVector("values");
      VarShape[0] = ExpectedFloatData.size();
      TestFloatContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedFloatData.data());

      std::vector<float> TestFloatData(ExpectedFloatData.size(), 0.0);
      TestFloatContainer->LoadFromDb(GroupName, VarName, VarShape, TestFloatData.data());
      for(std::size_t j = 0; j < TestFloatData.size(); j++) {
        EXPECT(TestFloatData[j] == ExpectedFloatData[j]);
      }
    } else if (VarTypeName == "string") {
      std::vector<std::string> ExpectedStringData = VarConfig[i].getStringVector("values");
      VarShape[0] = ExpectedStringData.size();
      TestStringContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedStringData.data());

      std::vector<std::string> TestStringData(ExpectedStringData.size(), "xx");
      TestStringContainer->LoadFromDb(GroupName, VarName, VarShape, TestStringData.data());
      for(std::size_t j = 0; j < TestStringData.size(); j++) {
        EXPECT(TestStringData[j] == ExpectedStringData[j]);
      }
    } else if (VarTypeName == "datetime") {
      std::vector<std::string> DtStrings = VarConfig[i].getStringVector("values");
      std::vector<util::DateTime> ExpectedDateTimeData(DtStrings.size());
      for (std::size_t j = 0; j < DtStrings.size(); j++) {
        util::DateTime TempDateTime(DtStrings[j]);
        ExpectedDateTimeData[j] = TempDateTime;
      }
      VarShape[0] = ExpectedDateTimeData.size();
      TestDateTimeContainer->StoreToDb(GroupName, VarName, VarShape, ExpectedDateTimeData.data());

      util::DateTime TempDt("0000-01-01T00:00:00Z");
      std::vector<util::DateTime> TestDateTimeData(ExpectedDateTimeData.size(), TempDt);
      TestDateTimeContainer->LoadFromDb(GroupName, VarName, VarShape, TestDateTimeData.data());
      for(std::size_t j = 0; j < TestDateTimeData.size(); j++) {
        EXPECT(TestDateTimeData[j] == ExpectedDateTimeData[j]);
      }
    } else {
      oops::Log::debug() << "test::ObsSpaceContainer::testStoreLoad: "
          << "container only supports data types int, float, string and datetime." << std::endl;
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
      { testConstructor(); });
    ts.emplace_back(CASE("database/ObsSpaceContainer/testGrpVarIter")
      { testGrpVarIter(); });
    ts.emplace_back(CASE("database/ObsSpaceContainer/testStoreLoad")
      { testStoreLoad(); });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
