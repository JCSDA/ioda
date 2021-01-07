/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Has_Attributes_c.cpp
/// \brief C bindings for ioda::Has_Attributes

#include "ioda/C/Group_c.h"
#include "ioda/C/c_binding_macros.h"
#include "ioda/C/structs_c.h"
#include "ioda/Group.h"

extern "C" {

void ioda_has_attributes_destruct(ioda_has_attributes* atts) {
  C_TRY;
  Expects(atts != nullptr);
  delete atts;
  C_CATCH_AND_TERMINATE;
}

ioda_string_ret_t* ioda_has_attributes_list(const ioda_has_attributes* atts) {
  ioda_string_ret_t* res = nullptr;
  C_TRY;
  Expects(atts != nullptr);
  std::vector<std::string> vals = atts->atts.list();

  res = create_str_vector_c(vals);
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

int ioda_has_attributes_exists(const ioda_has_attributes* atts, const char* name) {
  C_TRY;
  Expects(atts != nullptr);
  Expects(name != nullptr);
  bool exists = atts->atts.exists(std::string{name});
  C_CATCH_AND_RETURN((exists) ? 1 : 0, -1);
}

bool ioda_has_attributes_remove(ioda_has_attributes* atts, const char* name) {
  C_TRY;
  Expects(atts != nullptr);
  Expects(name != nullptr);
  atts->atts.remove(std::string{name});
  C_CATCH_AND_RETURN(true, false);
}

ioda_attribute* ioda_has_attributes_open(const ioda_has_attributes* atts, const char* name) {
  ioda_attribute* res = nullptr;
  C_TRY;
  Expects(atts != nullptr);
  Expects(name != nullptr);
  res = new ioda_attribute;
  Expects(res != nullptr);
  res->att = atts->atts.open(std::string{name});
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

bool ioda_has_attributes_rename(ioda_has_attributes* atts, const char* oldname, const char* newname) {
  C_TRY;
  Expects(atts != nullptr);
  Expects(oldname != nullptr);
  Expects(newname != nullptr);
  atts->atts.rename(std::string{oldname}, std::string{newname});
  C_CATCH_AND_RETURN(true, false);
}

#define IODA_HAS_ATTRIBUTES_CREATE_IMPL(funcnamestr, Type)                                            \
  IODA_DL ioda_attribute* funcnamestr(ioda_has_attributes* has_atts, const char* name, size_t n_dims, \
                                      const long* dims) {                                             \
    ioda_attribute* res = nullptr;                                                                    \
    C_TRY;                                                                                            \
    Expects(has_atts != nullptr);                                                                     \
    Expects(name != nullptr);                                                                         \
    Expects(dims != nullptr);                                                                         \
    std::vector<ioda::Dimensions_t> vdims(n_dims);                                                    \
    for (size_t i = 0; i < n_dims; ++i) vdims[i] = (ioda::Dimensions_t)dims[i];                       \
    res = new ioda_attribute;                                                                         \
    Expects(res != nullptr);                                                                          \
    res->att = has_atts->atts.create<Type>(std::string{name}, vdims);                                 \
    C_CATCH_RETURN_FREE(res, nullptr, res);                                                           \
  }

C_TEMPLATE_FUNCTION_DEFINITION(ioda_has_attributes_create, IODA_HAS_ATTRIBUTES_CREATE_IMPL);
}
