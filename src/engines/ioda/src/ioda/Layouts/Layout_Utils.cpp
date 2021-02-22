/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Layout_Utils.cpp
/// \brief Contains implementations for Layout Utilities.

#include "ioda/Layouts/Layout_Utils.h"

#include <string>
#include <vector>

namespace ioda {
namespace detail {
  std::string reverseStringSwapDelimiter(const std::string &str) {
    const char delim = '@';
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do {
      pos = str.find(delim, prev);
      if (pos == std::string::npos) pos = str.length();
      std::string token = str.substr(prev, pos - prev);
      if (!token.empty()) tokens.push_back(token);
      prev = pos + 1;
    } while (pos < str.length() && prev < str.length());

    // Reverse the tokens to get the output string
    std::string out;
    for (auto it = tokens.crbegin(); it != tokens.crend(); ++it) {
      if (it != tokens.crbegin()) out += "/";
      out += *it;
    }
    return out;
  }

}  // namespace detail
}  // namespace ioda
