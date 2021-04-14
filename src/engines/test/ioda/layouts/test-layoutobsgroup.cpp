/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */


#include <string>
#include <typeindex>

#include "ioda/Layouts/Layout_ObsGroup.h"

#include "eckit/testing/Test.h"

using namespace eckit::testing;

namespace ioda {
namespace test {

CASE("Derived variable and unit conversion methods") {
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
}

}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
