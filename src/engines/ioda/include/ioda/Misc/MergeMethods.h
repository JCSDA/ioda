#ifndef ENGINES_IODA_INCLUDE_IODA_MISC_MERGEMETHODS_H_
#define ENGINES_IODA_INCLUDE_IODA_MISC_MERGEMETHODS_H_
/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file MergeMethods.h
/// \brief Utility functions and structs for combining multiple variables into one
/// \todo The method for merging vertical coordinates will be added to this header.

#include <string>
#include <vector>

#include "ioda/Layout.h"
#include "ioda/defs.h"

namespace ioda {

struct IODA_DL ComplementaryVariableCreationParameters {
  ComplementaryVariableCreationParameters(std::string name) :
    outputName(name) {}
  std::string outputName;

  size_t inputVarsNeededCount;
  size_t inputVarsEnteredCount;

  ioda::detail::DataLayoutPolicy::MergeMethod mergeMethod;

  std::vector<std::string> inputVariableNames;
};

} // namespace ioda
#endif // ENGINES_IODA_INCLUDE_IODA_MISC_MERGEMETHODS_H_
