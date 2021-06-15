/*
 * (C) Crown Copyright 2021 UK Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_ODBQUERYPARAMETERS_H_
#define TEST_IODA_ODBQUERYPARAMETERS_H_

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/OdbQueryParameters.h"


namespace ioda {
namespace test {

class TestParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(TestParameters, Parameters);
 public:
  oops::Parameter<std::vector<OdbWhereParameters>> where{"where", {}, this};
};

// -----------------------------------------------------------------------------

void testOdbQueryParams(const eckit::LocalConfiguration &conf) {
  std::vector<eckit::LocalConfiguration> confs;
  conf.get("ODB Parameters", confs);
  for (size_t jconf = 0; jconf < confs.size(); ++jconf) {
    eckit::LocalConfiguration config = confs[jconf];
    TestParameters params;
    params.validate(config);
  }
}

// -----------------------------------------------------------------------------

class OdbQueryParameters : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::OdbQueryParameters";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    ts.emplace_back(CASE("ioda/OdbQueryParameters/", conf)
      { testOdbQueryParams(conf); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_ODBQUERYPARAMETERS_H_
