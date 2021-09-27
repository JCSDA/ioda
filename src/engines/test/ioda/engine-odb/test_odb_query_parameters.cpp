/*
 * (C) Crown Copyright 2021 UK Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Run.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

// Private header file used by this test. See CMakeLists.txt.
#include "Engines/ODC/OdbQueryParameters.h"

// -----------------------------------------------------------------------------

CASE("Validation") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  std::vector<eckit::LocalConfiguration> confs;
  conf.get("ODB Parameters", confs);
  for (size_t jconf = 0; jconf < confs.size(); ++jconf) {
    eckit::LocalConfiguration config = confs[jconf];
    ioda::Engines::ODC::OdbQueryParameters params;
    params.validateAndDeserialize(config);
  }
}

// -----------------------------------------------------------------------------

class OdbQueryParameters : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::OdbQueryParameters";}

  void register_tests() const override {}

  void clear() const override {}
};

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
  oops::Run run(argc, argv);
  OdbQueryParameters tests;
  return run.execute(tests);
}
