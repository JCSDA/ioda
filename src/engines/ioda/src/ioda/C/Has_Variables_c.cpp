/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Has_Variables_c.cpp
/// \brief C bindings for ioda::Has_Variables

#include "ioda/C/Has_Variables_c.h"

#include "ioda/C/c_binding_macros.h"
#include "ioda/C/structs_c.h"
#include "ioda/Group.h"

extern "C" {

void ioda_has_variables_destruct(ioda_has_variables* vars) {
  C_TRY;
  Expects(vars != nullptr);
  delete vars;
  C_CATCH_AND_TERMINATE;
}

ioda_string_ret_t* ioda_has_variables_list(const ioda_has_variables* vars) {
  ioda_string_ret_t* res = nullptr;
  C_TRY;
  Expects(vars != nullptr);
  std::vector<std::string> vals = vars->vars.list();

  res = create_str_vector_c(vals);
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

int ioda_has_variables_exists(const ioda_has_variables* vars, const char* name) {
  C_TRY;
  Expects(vars != nullptr);
  Expects(name != nullptr);
  bool exists = vars->vars.exists(std::string{name});
  C_CATCH_AND_RETURN((exists) ? 1 : 0, -1);
}

bool ioda_has_variables_remove(ioda_has_variables* vars, const char* name) {
  C_TRY;
  Expects(vars != nullptr);
  Expects(name != nullptr);
  vars->vars.remove(std::string{name});
  C_CATCH_AND_RETURN(true, false);
}

ioda_variable* ioda_has_variables_open(const ioda_has_variables* vars, const char* name) {
  ioda_variable* res = nullptr;
  C_TRY;
  Expects(vars != nullptr);
  Expects(name != nullptr);
  res = new ioda_variable;
  Expects(res != nullptr);
  res->var = vars->vars.open(std::string{name});
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

#define IODA_HAS_VARIABLES_CREATE_IMPL(funcnamestr, Type)                                           \
  IODA_DL ioda_variable* funcnamestr(ioda_has_variables* has_vars, const char* name, size_t n_dims, \
                                     const long* dims, const long* max_dims,                        \
                                     const struct ioda_variable_creation_parameters* params) {      \
    ioda_variable* res = nullptr;                                                                   \
    C_TRY;                                                                                          \
    Expects(has_vars != nullptr);                                                                   \
    Expects(name != nullptr);                                                                       \
    Expects(dims != nullptr);                                                                       \
    Expects(max_dims != nullptr);                                                                   \
    Expects(params != nullptr);                                                                     \
    std::vector<ioda::Dimensions_t> vdims(n_dims);                                                  \
    for (size_t i = 0; i < n_dims; ++i) vdims[i] = (ioda::Dimensions_t)dims[i];                     \
    std::vector<ioda::Dimensions_t> vmaxdims(n_dims);                                               \
    for (size_t i = 0; i < n_dims; ++i) vmaxdims[i] = (ioda::Dimensions_t)max_dims[i];              \
    res = new ioda_variable;                                                                        \
    Expects(res != nullptr);                                                                        \
    res->var = has_vars->vars.create<Type>(std::string{name}, vdims, vmaxdims, params->params);     \
    C_CATCH_RETURN_FREE(res, nullptr, res);                                                         \
  }

C_TEMPLATE_FUNCTION_DEFINITION(ioda_has_variables_create, IODA_HAS_VARIABLES_CREATE_IMPL);
}
