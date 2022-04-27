/*
 * (C) Copyright 2018-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_COPYATTRIBUTES_H_
#define TEST_IODA_COPYATTRIBUTES_H_

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/Copying.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/Engines/Factory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
template <typename AttrType>
void addTestAttr(const std::string & attrName, const std::vector<AttrType> & attrData,
                 ioda::Has_Attributes & attrContainer) {
  std::vector<ioda::Dimensions_t> attrDims(1, attrData.size());
  attrContainer.add<AttrType>(attrName, attrData, attrDims);
}

// -----------------------------------------------------------------------------
template <typename AttrType>
void checkTestAttrExact(const std::string & attrName,
                 const std::vector<AttrType> & attrRefData,
                 ioda::Has_Attributes & attrContainer) {
  std::vector<AttrType> attrTestData;
  attrContainer.read<AttrType>(attrName, attrTestData);

  EXPECT(attrRefData.size() == attrTestData.size());
  for (std::size_t i = 0; i < attrRefData.size(); ++i) {
    EXPECT(attrTestData[i] == attrRefData[i]);
  }
}

// -----------------------------------------------------------------------------
template <typename AttrType>
void checkTestAttrWithTol(const std::string & attrName,
                 const std::vector<AttrType> & attrRefData,
                 ioda::Has_Attributes & attrContainer, AttrType Tol) {
  std::vector<AttrType> attrTestData;
  attrContainer.read<AttrType>(attrName, attrTestData);

  EXPECT(oops::are_all_close_relative<AttrType>(attrRefData, attrTestData, Tol));
}

// -----------------------------------------------------------------------------
template <typename VarType>
ioda::Variable addTestVar(const std::string & varName, const std::vector<VarType> & varData,
                 ioda::Has_Variables & varContainer) {
  std::vector<ioda::Dimensions_t> varDims(1, varData.size());
  return varContainer.create<VarType>(varName, varDims).write(varData);
}

// -----------------------------------------------------------------------------

void setAndCheckAttributes(ioda::Has_Attributes & srcAttrContainer,
                           ioda::Has_Attributes & destAttrContainer,
                           std::vector<eckit::LocalConfiguration> & attrConf, double Tol) {
  // attrConf has a list of attributes that will be tested. Set these values into
  // the source container.
  for (std::size_t iattr = 0; iattr < attrConf.size(); ++iattr) {
    std::string attrName = attrConf[iattr].getString("name");
    std::string attrType = attrConf[iattr].getString("type");

    if (attrType == "int") {
        std::vector<int> attrData = attrConf[iattr].getIntVector("values");
        addTestAttr<int>(attrName, attrData, srcAttrContainer);
    } else if (attrType == "float") {
        std::vector<float> attrData = attrConf[iattr].getFloatVector("values");
        addTestAttr<float>(attrName, attrData, srcAttrContainer);
    } else if (attrType == "double") {
        std::vector<double> attrData = attrConf[iattr].getDoubleVector("values");
        addTestAttr<double>(attrName, attrData, srcAttrContainer);
    } else if (attrType == "string") {
        std::vector<std::string> attrData = attrConf[iattr].getStringVector("values");
        addTestAttr<std::string>(attrName, attrData, srcAttrContainer);
    } else {
      std::string errorMsg = std::string("\nUnrecognized attribute type: ") + attrType +
          std::string("\nMust use one of 'int', 'float', 'double' or 'string'");
      throw ioda::Exception(errorMsg.c_str(), ioda_Here());
    }
  }

  // Do the copy
  ioda::copyAttributes(srcAttrContainer, destAttrContainer);

  // Check the copy
  for (std::size_t iattr = 0; iattr < attrConf.size(); ++iattr) {
    std::string attrName = attrConf[iattr].getString("name");
    std::string attrType = attrConf[iattr].getString("type");

    if (attrType == "int") {
        std::vector<int> attrData = attrConf[iattr].getIntVector("values");
        checkTestAttrExact<int>(attrName, attrData, destAttrContainer);
    } else if (attrType == "float") {
        std::vector<float> attrData = attrConf[iattr].getFloatVector("values");
        checkTestAttrWithTol<float>(attrName, attrData, destAttrContainer, static_cast<float>(Tol));
    } else if (attrType == "double") {
        std::vector<double> attrData = attrConf[iattr].getDoubleVector("values");
        checkTestAttrWithTol<double>(attrName, attrData, destAttrContainer, Tol);
    } else if (attrType == "string") {
        std::vector<std::string> attrData = attrConf[iattr].getStringVector("values");
        checkTestAttrExact<std::string>(attrName, attrData, destAttrContainer);
    }
  }
}

// -----------------------------------------------------------------------------

void testGroupAttributes() {
  eckit::LocalConfiguration groupConf;
  ::test::TestEnvironment::config().get("group copy", groupConf);

  std::vector<eckit::LocalConfiguration> attrConf;
  groupConf.get("attributes", attrConf);
  double Tol = groupConf.getDouble("tolerance");

  // Create a framework for testing. Use the ObsStore backend (memory) and
  // create two groups. One for holding the source attributes and the other
  // for holding the destination attributes. Build the attributes in the source
  // group and use the copyAttributes function to copy them to the destination
  // group and then check that they match.
  ioda::Engines::BackendNames backendName = ioda::Engines::BackendNames::ObsStore;
  ioda::Engines::BackendCreationParameters backendParams;
  ioda::Group topLevelGroup = constructBackend(backendName, backendParams);

  ioda::Group srcGroup = topLevelGroup.create("source");
  ioda::Group destGroup = topLevelGroup.create("destination");

  setAndCheckAttributes(srcGroup.atts, destGroup.atts, attrConf, Tol);
}

// -----------------------------------------------------------------------------

void testVariableAttributes() {
  eckit::LocalConfiguration groupConf;
  ::test::TestEnvironment::config().get("variable copy", groupConf);

  std::vector<eckit::LocalConfiguration> varConf;
  groupConf.get("variables", varConf);
  double Tol = groupConf.getDouble("tolerance");

  // Create a framework for testing. Use the ObsStore backend (memory) and
  // create two sub groups. One for holding the source variables and the other
  // for holding the destination variables. Build the attributes in the source
  // variables and use the copyAttributes function to copy them to the destination
  // variables and then check that they match.
  ioda::Engines::BackendNames backendName = ioda::Engines::BackendNames::ObsStore;
  ioda::Engines::BackendCreationParameters backendParams;
  ioda::Group topLevelGroup = constructBackend(backendName, backendParams);

  ioda::Group srcGroup = topLevelGroup.create("source");
  ioda::Group destGroup = topLevelGroup.create("destination");

  for (std::size_t ivar = 0; ivar < varConf.size(); ++ivar) {
    std::string varName = varConf[ivar].getString("name");
    std::string varType = varConf[ivar].getString("type");
    std::vector<eckit::LocalConfiguration> attrConf;
    varConf[ivar].get("attributes", attrConf);

    ioda::Variable srcVar;
    ioda::Variable destVar;
    if (varType == "int") {
        std::vector<int> varData = varConf[ivar].getIntVector("values");
        srcVar = addTestVar<int>(varName, varData, srcGroup.vars);
        destVar = addTestVar<int>(varName, varData, destGroup.vars);
    } else if (varType == "float") {
        std::vector<float> varData = varConf[ivar].getFloatVector("values");
        srcVar = addTestVar<float>(varName, varData, srcGroup.vars);
        destVar = addTestVar<float>(varName, varData, destGroup.vars);
    } else if (varType == "double") {
        std::vector<double> varData = varConf[ivar].getDoubleVector("values");
        srcVar = addTestVar<double>(varName, varData, srcGroup.vars);
        destVar = addTestVar<double>(varName, varData, destGroup.vars);
    } else if (varType == "string") {
        std::vector<std::string> varData = varConf[ivar].getStringVector("values");
        srcVar = addTestVar<std::string>(varName, varData, srcGroup.vars);
        destVar = addTestVar<std::string>(varName, varData, destGroup.vars);
    } else {
      std::string errorMsg = std::string("\nUnrecognized variable type: ") + varType +
          std::string("\nMust use one of 'int', 'float', 'double' or 'string'");
      throw ioda::Exception(errorMsg.c_str(), ioda_Here());
    }

    setAndCheckAttributes(srcVar.atts, destVar.atts, attrConf, Tol);
  }
}

// -----------------------------------------------------------------------------

void testUnsupportedType() {
  // Create a framework for testing. Use the ObsStore backend (memory) and
  // create two groups. One for holding the source attributes and the other
  // for holding the destination attributes. Build the attributes in the source
  // group and use the copyAttributes function to copy them to the destination
  // group and then check that they match.
  ioda::Engines::BackendNames backendName = ioda::Engines::BackendNames::ObsStore;
  ioda::Engines::BackendCreationParameters backendParams;
  ioda::Group topLevelGroup = constructBackend(backendName, backendParams);

  ioda::Group srcGroup = topLevelGroup.create("source");
  ioda::Group destGroup = topLevelGroup.create("destination");

  // Need to keep this in sync with the supported attribute types in
  // Currently, uint64_t is not a supported attribute type.
  std::string attrName("uint64_t_attr");
  srcGroup.atts.add<uint64_t>(attrName, { 1, 2, 3 }, { 3 });

  EXPECT_THROWS(ioda::copyAttributes(srcGroup.atts, destGroup.atts));
}
// -----------------------------------------------------------------------------

class CopyAttributes : public oops::Test {
 public:
  CopyAttributes() {}
  virtual ~CopyAttributes() {}

 private:
  std::string testid() const override {return "test::CopyAttributes";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/CopyAttributes/testGroupAttributes")
      { testGroupAttributes(); });
    ts.emplace_back(CASE("ioda/CopyAttributes/testVariableAttributes")
      { testVariableAttributes(); });
    ts.emplace_back(CASE("ioda/CopyAttributes/testUnsupportedType")
      { testUnsupportedType(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_COPYATTRIBUTES_H_
