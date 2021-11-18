/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSDATAVECTOR_H_
#define TEST_IODA_OBSDATAVECTOR_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/make_unique.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Expect.h"

#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"

namespace ioda {
namespace test {

class ObsDataVecTestFixture : private boost::noncopyable {
  typedef ioda::ObsSpace ObsSpace_;

 public:
  static ObsSpace_ & obspace() {return *getInstance().obspace_;}

 private:
  static ObsDataVecTestFixture& getInstance() {
    static ObsDataVecTestFixture theObsDataVecTestFixture;
    return theObsDataVecTestFixture;
  }

  ObsDataVecTestFixture() {
    const eckit::Configuration &conf = ::test::TestEnvironment::config();
    const util::DateTime bgn(conf.getString("window begin"));
    const util::DateTime end(conf.getString("window end"));

    eckit::LocalConfiguration obsconf(conf, "obs space");
    ioda::ObsTopLevelParameters obsparams;
    obsparams.validateAndDeserialize(obsconf);
    obspace_ = boost::make_unique<ObsSpace_>(obsparams, oops::mpi::world(),
                                             bgn, end, oops::mpi::myself());
  }

  std::unique_ptr<ObsSpace_> obspace_;
};

std::string trim(const std::string & str) {
  const auto strBegin = str.find_first_not_of("\n");
  if (strBegin == std::string::npos) return "";
  const auto strEnd = str.find_last_not_of("\n");
  const auto strRange = strEnd - strBegin + 1;
  return str.substr(strBegin, strRange);
}


template <typename T>
void testPrint(const std::string &datatype) {
  eckit::LocalConfiguration conf(::test::TestEnvironment::config(), "print." + datatype);

  oops::Variables vars;
  for (const std::string &var : conf.getStringVector("variables"))
    vars.push_back(var);
  const std::string group = conf.getString("group");
  ioda::ObsDataVector<T> vector(ObsDataVecTestFixture::obspace(), vars, group);

  std::stringstream stream;
  stream << vector;
  std::string output = trim(stream.str());
  std::string expectedOutput = trim(conf.getString("expected output"));
  EXPECT_EQUAL(output, expectedOutput);
}

void testAssignToExistingVariables(const std::string &testtype) {
  eckit::LocalConfiguration conf(::test::TestEnvironment::config(),
                                "assignToExistingVariables." + testtype);

  oops::Variables ObsDataVectorVars;
  for (const std::string &var : conf.getStringVector("ObsDataVector variables"))
    ObsDataVectorVars.push_back(var);
  const std::string group = conf.getString("group");
  // known good output:
  ioda::ObsDataVector<float> ObsDataVect(ObsDataVecTestFixture::obspace(),
                                         ObsDataVectorVars, group);
  // empty ObsDataVector, which should become kgo if assignToExistingVariables works:
  ioda::ObsDataVector<float> ObsDataVect0(ObsDataVecTestFixture::obspace(), ObsDataVectorVars);

  const ioda::ObsVector ObsVect(ObsDataVecTestFixture::obspace(), group);

  if (testtype == "variables not found in ObsVector") {
    EXPECT_THROWS_MSG(ObsDataVect0.assignToExistingVariables(ObsVect), "not found in ObsVector");
  } else {
    std::stringstream stream1;
    stream1 << ObsDataVect;
    std::string expectedOutput = trim(stream1.str());

    ObsDataVect0.assignToExistingVariables(ObsVect);

    std::stringstream stream2;
    stream2 << ObsDataVect0;
    std::string output = trim(stream2.str());

    EXPECT_EQUAL(output, expectedOutput);
  }
}


CASE("ioda/ObsDataVector/printFloat") {
  testPrint<float>("float");
}

CASE("ioda/ObsDataVector/printDouble") {
  testPrint<double>("double");
}

CASE("ioda/ObsDataVector/printInt") {
  testPrint<int>("int");
}

CASE("ioda/ObsDataVector/printString") {
  testPrint<std::string>("string");
}

CASE("ioda/ObsDataVector/printDateTime") {
  testPrint<util::DateTime>("datetime");
}

CASE("ioda/ObsDataVector/printBool") {
  testPrint<bool>("bool");
}

CASE("ioda/ObsDataVector/assignToExistingVariablesMulti") {
  testAssignToExistingVariables("multiple variables disordered");
}

CASE("ioda/ObsDataVector/assignToExistingVariablesExcess") {
  testAssignToExistingVariables("excess variables in ObsVector");
}

CASE("ioda/ObsDataVector/assignToExistingVariablesNotFound") {
  testAssignToExistingVariables("variables not found in ObsVector");
}

CASE("ioda/ObsDataVector/closeObsSpace") {
  // In case the obsdataout spec is ever used
  ObsDataVecTestFixture::obspace().save();
}

class ObsDataVector : public oops::Test {
 private:
  std::string testid() const override {return "test::ObsDataVector<ioda::IodaTrait>";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSDATAVECTOR_H_
