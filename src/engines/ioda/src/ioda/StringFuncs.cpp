/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */

#include "ioda/Misc/StringFuncs.h"

#include "ioda/Exception.h"

namespace ioda {
std::vector<std::string> splitPaths(const std::string& p) {
  try {
    using std::string;
    std::vector<string> res;
    if (p.empty()) return res;
    size_t off = 0;
    if (p[0] == '/') {
      res.emplace_back("/");
      off++;
    }

    size_t end = 0;
    while (end != string::npos) {
      end      = p.find('/', off);
      string s = p.substr(off, end - off);
      if (!s.empty()) res.push_back(s);
      off = end + 1;
    }

    return res;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while performing path expansion.", ioda_Here())
      .add("path", p));
  }
}

std::string condensePaths(const std::vector<std::string>& p, size_t start, size_t end) {
  try {
    std::string res;
    if (end == std::string::npos) end = p.size();

    for (size_t i = start; i < end; ++i) {
      if ((i != start) && (res != "/")) res.append("/");
      res.append(p[i]);
    }

    return res;
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda.", ioda_Here()));
  }
}

std::vector<std::string> concatenateStringVectors(const std::vector<std::vector<std::string> > &stringVectors)
{
  try {
    std::vector<std::string> derivedVector = stringVectors.at(0);
    for (size_t vectorIndex = 1; vectorIndex < stringVectors.size(); vectorIndex++) {
      if (stringVectors[vectorIndex].size() != derivedVector.size())
        throw Exception("string vectors are of unequal lengths", ioda_Here());
      for (size_t entry = 0; entry < stringVectors[vectorIndex].size(); entry++) {
        derivedVector.at(entry) += stringVectors[vectorIndex].at(entry);
      }
    }
    // remove trailing spaces
    for (std::string& currentEntry : derivedVector) {
      size_t stringEnd = currentEntry.find_last_not_of(' ');
      if (stringEnd < currentEntry.size() - 1) {
        currentEntry.erase(stringEnd + 1, std::string::npos);
      } else if (stringEnd == std::string::npos) {
        currentEntry.erase(0, std::string::npos);
      }
    }
    return derivedVector;
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda.", ioda_Here()));
  }

  
}

std::string convertV1PathToV2Path(const std::string & path) {
  try {
    const char delim = '@';

    // Only do this swap if an '@' is found.
    if (path.find(delim) == std::string::npos) return path;

    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    do {
      pos = path.find(delim, prev);
      if (pos == std::string::npos) pos = path.length();
      std::string token = path.substr(prev, pos - prev);
      if (!token.empty()) tokens.push_back(std::move(token));
      prev = pos + 1;
    } while (pos < path.length() && prev < path.length());

    // Reverse the tokens to get the output path
    std::string out;
    for (auto it = tokens.crbegin(); it != tokens.crend(); ++it) {
      if (it != tokens.crbegin()) out += "/";
      out += *it;
    }
    return out;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while converting a path to v2 format.", ioda_Here())
      .add("path", path));
  }
}

}  // namespace ioda
