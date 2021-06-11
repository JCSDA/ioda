#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_api
 *
 * @{
 * \file SFuncs.h
 * \brief Miscellaneous string functions
 */

#include <string>
#include <vector>

#include "ioda/defs.h"

namespace ioda {
/// @brief Split a string based on occurances of the '/' character.
/// @ingroup ioda_cxx_api
/// @param p is the input path
/// @return The split path components.
IODA_DL std::vector<std::string> splitPaths(const std::string& p);

/// @brief The inverse of splitPaths. Concatenate strings, separating with '/'.
/// @ingroup ioda_cxx_api
/// @param p represents the path components.
/// @param start is an index of the vector to start with.
/// @param end is an index of the vector to end with. If set to std::string::npos,
///   then the end will be determined from p.size().
/// @return The resulting string.
IODA_DL std::string condensePaths(const std::vector<std::string>& p, size_t start = 0,
                                  size_t end = std::string::npos);

/// @brief Concatenate equal-length vectors of strings element-by-element. Removes trailing spaces.
/// @param stringVectors represents the vectors of strings all stored together in one vector.
/// @return The resulting vector of strings.
IODA_DL std::vector<std::string> concatenateStringVectors(
    const std::vector<std::vector<std::string>>& stringVectors);

/// @brief Split `path` into substrings separated by `@` characters, then concatenate them in
/// reverse order, replacing the `@`s with `/`s.
///
/// For example, longitude@MetaData becomes MetaData/longitude.
///
/// @param path is the path to convert
/// @return The converted path.
IODA_DL std::string convertV1PathToV2Path(const std::string & path);

}  // namespace ioda

/// @}
