/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_group
 * @{
 * \file Group_c.cpp
 * \brief @link ioda_group C bindings @endlink for ioda::Group
 */

#include "ioda/C/Group_c.h"

#include "./structs_c.h"
#include "ioda/C/String_c.h"
#include "ioda/C/c_binding_macros.h"  // C_TRY and C_CATCH_AND_TERMINATE
#include "ioda/Group.h"

extern "C" {

void ioda_group_destruct(ioda_group* g) {
  C_TRY;
  Expects(g != nullptr);
  delete g;
  C_CATCH_AND_TERMINATE;
}

ioda_string_ret_t* ioda_group_list(const ioda_group* g) {
  ioda_string_ret_t* ret = nullptr;
  C_TRY;
  Expects(g != nullptr);
  std::vector<std::string> vals = g->g.list();

  ret = create_str_vector_c(vals);
  C_CATCH_RETURN_FREE(ret, NULL, ret);
}

int ioda_group_exists(const ioda_group* g, size_t sz, const char* name) {
  C_TRY;
  Expects(g != nullptr);
  Expects(name != nullptr);
  C_CATCH_AND_RETURN((g->g.exists(std::string(name, sz))) ? 1 : 0, -1);
}

ioda_group* ioda_group_create(ioda_group* g, size_t sz, const char* name) {
  ioda_group* res = nullptr;
  C_TRY;
  Expects(g != nullptr);
  Expects(name != nullptr);
  res = new ioda_group;
  Expects(res != nullptr);
  res->g = g->g.create(std::string(name, sz));
  C_CATCH_RETURN_FREE(res, NULL, res);
}

ioda_group* ioda_group_open(const ioda_group* g, size_t sz, const char* name) {
  ioda_group* res = nullptr;
  C_TRY;
  Expects(g != nullptr);
  Expects(name != nullptr);
  res = new ioda_group;
  Expects(res != nullptr);
  res->g = g->g.open(std::string(name, sz));
  C_CATCH_RETURN_FREE(res, NULL, res);
}

IODA_DL ioda_has_attributes* ioda_group_atts(const ioda_group* g) {
  ioda_has_attributes* res = nullptr;
  C_TRY;
  Expects(g != nullptr);
  res       = new ioda_has_attributes;
  res->atts = g->g.atts;
  C_CATCH_RETURN_FREE(res, NULL, res);
}

IODA_DL ioda_has_variables* ioda_group_vars(const ioda_group* g) {
  ioda_has_variables* res = nullptr;
  C_TRY;
  Expects(g != nullptr);
  res       = new ioda_has_variables;
  res->vars = g->g.vars;
  C_CATCH_RETURN_FREE(res, NULL, res);
}
}

/// @}
