/*
 * (C) Crown copyright 2026 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */


#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace ioda {
namespace test {

// Returns true if the same elements exist in both vectors with the same multiplicities
// but not necessarily in the same order
bool unorderedVectorComparison(std::vector<std::string> firstVector,
                               std::vector<std::string> secondVector) {
  std::sort(firstVector.begin(), firstVector.end());
  std::sort(secondVector.begin(), secondVector.end());
  return (firstVector == secondVector);
}

}  // namespace test
}  // namespace ioda

