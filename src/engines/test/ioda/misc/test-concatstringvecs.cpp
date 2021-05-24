/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Misc/StringFuncs.h"

#include <string>
#include <vector>

#include "eckit/testing/Test.h"

using namespace eckit::testing;

namespace ioda {
namespace test {

CASE("Non-empty inputs of equal element counts") {
  std::vector<std::string> strVec1 = {"a", "A", "1"};
  std::vector<std::string> strVec2 = {"b", "B", "2"};
  std::vector<std::string> strVec3 = {"c", "C", "3"};
  std::vector<std::string> expectedOutputVec = {"abc", "ABC", "123"};

  std::vector<std::vector<std::string> > combinedVec = {strVec1, strVec2, strVec3};

  std::vector<std::string> outputVector = ioda::concatenateStringVectors(combinedVec);

  EXPECT(expectedOutputVec == outputVector);
}

CASE("Empty inputs of equal element counts") {
  std::vector<std::string> strVec1 = {"", "", ""};
  std::vector<std::string> strVec2 = {"", "", ""};
  std::vector<std::string> strVec3 = {"", "", ""};
  std::vector<std::string> expectedOutputVec = {"", "", ""};

  std::vector<std::vector<std::string> > combinedVec = {strVec1, strVec2, strVec3};

  std::vector<std::string> outputVector = ioda::concatenateStringVectors(combinedVec);

  EXPECT(expectedOutputVec == outputVector);
}

CASE("Unequal element counts") {
  std::vector<std::string> strVec1 = {"a", "A", "1"};
  std::vector<std::string> strVec2 = {"b", "B", "2"};
  std::vector<std::string> strVec3 = {"c", "3"};

  std::vector<std::vector<std::string> > combinedVec = {strVec1, strVec2, strVec3};

  EXPECT_THROWS(ioda::concatenateStringVectors(combinedVec));
}

CASE("One vector input") {
  std::vector<std::string> strVec1 = {"a", "A", "1"};

  std::vector<std::vector<std::string> > combinedVec = {strVec1};
  std::vector<std::string> outputVector = ioda::concatenateStringVectors(combinedVec);
  EXPECT(strVec1 == outputVector);
}

CASE("Trailing spaces removed") {
  std::vector<std::string> strVec1 = {"f oo", "b ar", "   baz"};
  std::vector<std::string> strVec2 = {"f o o", "ba r", "baz"};
  std::vector<std::string> strVec3 = {"f o o   ", "bar   ", "baz   "};
  std::vector<std::string> expectedOutputVec = {"f oof o of o o", "b arba rbar", "   bazbazbaz"};

  std::vector<std::vector<std::string> > combinedVec = {strVec1, strVec2, strVec3};

  std::vector<std::string> outputVector = ioda::concatenateStringVectors(combinedVec);

  EXPECT(expectedOutputVec == outputVector);
}

CASE("All space vectors") {
  std::vector<std::string> strVec1 = {"", "", "   "};
  std::vector<std::string> strVec2 = {"", " ", " "};
  std::vector<std::string> strVec3 = {"   ", "   ", "    "};
  std::vector<std::string> expectedOutputVec = {"", "", ""};

  std::vector<std::vector<std::string> > combinedVec = {strVec1, strVec2, strVec3};

  std::vector<std::string> outputVector = ioda::concatenateStringVectors(combinedVec);

  EXPECT(expectedOutputVec == outputVector);
}

}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
