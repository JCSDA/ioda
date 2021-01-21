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
    obspace_ = boost::make_unique<ObsSpace_>(obsconf, oops::mpi::world(),
                                             bgn, end, oops::mpi::myself());
  }

  std::unique_ptr<ObsSpace_> obspace_;
};


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
  std::string output = stream.str();
  std::string expectedOutput = conf.getString("expected output");
  EXPECT_EQUAL(output, expectedOutput);
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
