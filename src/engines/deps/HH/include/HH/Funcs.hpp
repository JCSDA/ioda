#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <string>
#include <vector>

#include "defs.hpp"

namespace HH {
HH_DL std::vector<std::string> splitPaths(const std::string& p);

HH_DL std::string condensePaths(const std::vector<std::string>& p, size_t start = 0, size_t end = 0);
}  // namespace HH
