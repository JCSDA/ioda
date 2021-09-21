/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/testconfig.h"

#include "ioda/Exception.h"

// This header is internal to ioda. It is not callable by end-users outside of the
// testing environment.
#include "ioda/Layouts/Layout_ObsGroup_ODB.h"

#include <string>
#include <typeindex>

#include "eckit/testing/Test.h"

using namespace eckit::testing;

namespace ioda {
namespace test {

CASE("Concatenation mapping file; error checks of unit conversion methods") {
  std::string yamlMappingFile
    = std::string(IODA_ENGINES_TEST_SOURCE_DIR) + "/layouts/odb_concat_name_map.yaml";
  ioda::detail::DataLayoutPolicy_ObsGroup_ODB dataLayoutPolicy(yamlMappingFile);
  // Manually adding a variable which was already included in the mapping file
  EXPECT_THROWS(ioda::detail::DataLayoutPolicy::generate("ObsGroupODB", yamlMappingFile,
                                                         {"firstPart"}));
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
  // unit conversion methods
  EXPECT_NOT(dataLayoutPolicy.isMapped("notInMapping"));
  EXPECT_THROWS(dataLayoutPolicy.getUnit("notInMapping"));
}
CASE("Input data name matches the export data name") {
  std::string yamlMappingFile = std::string(IODA_ENGINES_TEST_SOURCE_DIR)
    + "/layouts/odb_matchinginputoutput_name_map.yaml";
  EXPECT_THROWS(ioda::detail::DataLayoutPolicy_ObsGroup_ODB policy(yamlMappingFile));
}
//The vertical coordinate merge method is currently unsupported
CASE("Vertical coordinate mapping file") {
  std::string yamlMappingFile = std::string(IODA_ENGINES_TEST_SOURCE_DIR)
    + "/layouts/odb_verticalreference_name_map.yaml";
  EXPECT_THROWS(ioda::detail::DataLayoutPolicy_ObsGroup_ODB dataLayoutPolicy(yamlMappingFile));
}

CASE("Missing YAML on generate") {
  EXPECT_THROWS(detail::DataLayoutPolicy::generate("ObsGroupODB"));
  EXPECT_THROWS(detail::DataLayoutPolicy::generate(
                  detail::DataLayoutPolicy::Policies::ObsGroupODB));
}

}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
