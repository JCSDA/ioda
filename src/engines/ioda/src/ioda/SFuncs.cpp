/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */

#include "ioda/Misc/SFuncs.h"

namespace ioda {
std::vector<std::string> splitPaths(const std::string& p) {
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
}

std::string condensePaths(const std::vector<std::string>& p, size_t start, size_t end) {
  std::string res;
  if (end == std::string::npos) end = p.size();

  for (size_t i = start; i < end; ++i) {
    if ((i != start) && (res != "/")) res.append("/");
    res.append(p[i]);
  }

  return res;
}
}  // namespace ioda
