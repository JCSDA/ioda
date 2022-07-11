/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */


#include <string>
#include <typeindex>
#include "ioda/Exception.h"
#include "ioda/testconfig.h"

// This header is internal to ioda. It is not callable by end-users outside of the
// testing environment.
#include "Layouts/Layout_ObsGroup.h"

#include "eckit/testing/Test.h"

using namespace eckit::testing;

namespace ioda {
namespace test {

CASE("Derived variable, unit conversion, and exception checking methods") {
  ioda::detail::DataLayoutPolicy_ObsGroup dataLayoutPolicy;
  EXPECT_NOT(dataLayoutPolicy.isComplementary("anyVariable"));
  EXPECT_THROWS(dataLayoutPolicy.getComplementaryPosition("anyVariable"));
  EXPECT_THROWS(dataLayoutPolicy.getInputsNeeded("anyVariable"));
  EXPECT_THROWS(dataLayoutPolicy.getMergeMethod("anyVariable"));
  EXPECT_THROWS(dataLayoutPolicy.getOutputNameFromComponent("anyVariable"));
  EXPECT_THROWS(dataLayoutPolicy.getOutputVariableDataType("anyVariable"));
  // unit conversion methods
  EXPECT_NOT(dataLayoutPolicy.isMapped("anyVariable"));
  EXPECT_THROWS(dataLayoutPolicy.getUnit("anyVariable"));
  // exception checking method
  EXPECT_NOT(dataLayoutPolicy.isMapOutput("anyVariable"));
}

CASE("Generate variants") {
  detail::DataLayoutPolicy::generate("ObsGroup");
  detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroup);
  std::string str;
  EXPECT_THROWS(detail::DataLayoutPolicy::generate("ObsGroup", str));
  EXPECT_THROWS(detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroup,
                                                   str));
  EXPECT_THROWS(detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::None, str));
}

}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
