/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Layouts/Layout_ObsGroup_ODB.h"

#include <string>
#include <typeindex>

#include "eckit/testing/Test.h"

/// @def TEST_SOURCE_DIR Convenience definition set in build system
/// to find the source directory's test file.

using namespace eckit::testing;

namespace ioda {
namespace test {

CASE("Concatenation mapping file") {
  std::string yamlMappingFile = std::string(TEST_SOURCE_DIR) + "/odb_concat_name_map.yaml";
  ioda::detail::DataLayoutPolicy_ObsGroup_ODB dataLayoutPolicy(yamlMappingFile);
  //existent variable in mapping file
  EXPECT(dataLayoutPolicy.isComplementary("firstPart"));
  EXPECT(dataLayoutPolicy.isComplementary("secondPart"));
  EXPECT(dataLayoutPolicy.isComplementary("thirdPart"));
  EXPECT_EQUAL(dataLayoutPolicy.getComplementaryPosition("firstPart"), 0);
  EXPECT_EQUAL(dataLayoutPolicy.getComplementaryPosition("secondPart"), 1);
  EXPECT_EQUAL(dataLayoutPolicy.getComplementaryPosition("thirdPart"), 2);
  EXPECT_EQUAL(dataLayoutPolicy.getInputsNeeded("firstPart"), 3);
  EXPECT_EQUAL(dataLayoutPolicy.getInputsNeeded("secondPart"), 3);
  EXPECT_EQUAL(dataLayoutPolicy.getInputsNeeded("thirdPart"), 3);
  EXPECT(dataLayoutPolicy.getMergeMethod("firstPart") ==
               ioda::detail::DataLayoutPolicy_ObsGroup_ODB::MergeMethod::Concat);
  EXPECT(dataLayoutPolicy.getMergeMethod("secondPart") ==
               ioda::detail::DataLayoutPolicy_ObsGroup_ODB::MergeMethod::Concat);
  EXPECT(dataLayoutPolicy.getMergeMethod("thirdPart") ==
               ioda::detail::DataLayoutPolicy_ObsGroup_ODB::MergeMethod::Concat);
  EXPECT_EQUAL(dataLayoutPolicy.getOutputNameFromComponent("firstPart"),
               std::string("combined"));
  EXPECT_EQUAL(dataLayoutPolicy.getOutputNameFromComponent("firstPart"),
               dataLayoutPolicy.getOutputNameFromComponent("secondPart"));
  EXPECT_EQUAL(dataLayoutPolicy.getOutputNameFromComponent("firstPart"),
               dataLayoutPolicy.getOutputNameFromComponent("thirdPart"));
  EXPECT(dataLayoutPolicy.getOutputVariableDataType("firstPart") ==
               std::type_index(typeid(std::string)));
  EXPECT(dataLayoutPolicy.getOutputVariableDataType("firstPart") ==
               dataLayoutPolicy.getOutputVariableDataType("secondPart"));
  EXPECT(dataLayoutPolicy.getOutputVariableDataType("firstPart") ==
               dataLayoutPolicy.getOutputVariableDataType("thirdPart"));
  EXPECT_NOT(dataLayoutPolicy.isComplementary("notInMapping"));
  EXPECT_THROWS(dataLayoutPolicy.getComplementaryPosition("notInMapping"));
  EXPECT_THROWS(dataLayoutPolicy.getInputsNeeded("notInMapping"));
  EXPECT_THROWS(dataLayoutPolicy.getMergeMethod("notInMapping"));
  EXPECT_THROWS(dataLayoutPolicy.getOutputNameFromComponent("notInMapping"));
  EXPECT_THROWS(dataLayoutPolicy.getOutputVariableDataType("notInMapping"));
}
CASE("Input data name matches the export data name") {
  std::string yamlMappingFile = std::string(TEST_SOURCE_DIR) + "/odb_matchinginputoutput_name_map.yaml";
  EXPECT_THROWS(ioda::detail::DataLayoutPolicy_ObsGroup_ODB policy(yamlMappingFile));
}
//The vertical coordinate merge method is currently unsupported
CASE("Vertical coordinate mapping file") {
  std::string yamlMappingFile = std::string(TEST_SOURCE_DIR) + "/odb_verticalreference_name_map.yaml";
  EXPECT_THROWS(ioda::detail::DataLayoutPolicy_ObsGroup_ODB dataLayoutPolicy(yamlMappingFile));
}

}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
