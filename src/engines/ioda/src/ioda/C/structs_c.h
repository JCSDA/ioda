#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file structs_c.h
/// \brief C wrappers for ioda classes and structures. Private header. Can have C++!
/// \ingroup ioda_c_api

#include <string>
#include <vector>

#include "ioda/C/String_c.h"
#include "ioda/C/c_binding_macros.h"
#include "ioda/Group.h"

extern "C" {
struct ioda_group {
  ioda::Group g;
};
struct ioda_has_attributes {
  ioda::Has_Attributes atts;
};
struct ioda_has_variables {
  ioda::Has_Variables vars;
};
struct ioda_attribute {
  ioda::Attribute att;
};
struct ioda_variable {
  ioda::Variable var;
};
struct ioda_dimensions {
  ioda::Dimensions d;
};
struct ioda_variable_creation_parameters {
  ioda::VariableCreationParameters params;
};
}

inline ioda_string_ret_t* create_str_vector_c(const std::vector<std::string>& vdata) noexcept {
  ioda_string_ret_t* res = nullptr;

  res = new ioda_string_ret_t;
  if (!res) return NULL;

  res->n = vdata.size();

  res->strings = new char*[res->n];
  if (!res->strings) goto hadError_Strings;
  memset(res->strings, 0, sizeof(char*) * res->n);

  for (size_t i = 0; i < res->n; ++i) {
    size_t slen     = vdata[i].length();  // length of string without the ending null byte
    res->strings[i] = new char[slen + 1];
    if (!res->strings[i]) goto hadError_InnerStrings;

    std::copy_n(vdata[i].cbegin(), slen, res->strings[i]);
    res->strings[i][slen] = '\0';
  }

  return res;

hadError_InnerStrings:
  for (size_t i = 0; i < res->n; ++i)
    if (res->strings[i]) delete res->strings[i];
  // Fallthrough to hadError_Strings.

hadError_Strings:
  delete res;

  return nullptr;
}
