/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Exception.h"
#include "ioda/Misc/StringFuncs.h"

#include <string>
#include <vector>

#include "eckit/testing/Test.h"

using namespace eckit::testing;

namespace ioda {
namespace test {

CASE("input: empty") {
  std::string expectedResult = "";
  std::string result = ioda::convertV1PathToV2Path("");
  EXPECT_EQUAL(result, expectedResult);
}

CASE("input: ioda-v1 variable name") {
  std::string expectedResult = "ObsValue/air_temperature";
  std::string result = ioda::convertV1PathToV2Path("air_temperature@ObsValue");
  EXPECT_EQUAL(result, expectedResult);
}

CASE("input: ioda-v2 variable name") {
  std::string expectedResult = "ObsValue/air_temperature";
  // Should leave the string unchanged
  std::string result = ioda::convertV1PathToV2Path("ObsValue/air_temperature");
  EXPECT_EQUAL(result, expectedResult);
}

CASE("input: variable name without group") {
  std::string expectedResult = "air_temperature";
  std::string result = ioda::convertV1PathToV2Path("air_temperature");
  EXPECT_EQUAL(result, expectedResult);
}

}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
