/*
 * (C) Copyright 2020-2022 UCAR
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
#include "ioda/C/VecString_c.h"

#include "./structs_c.h"
#include "ioda/C/String_c.h"
#include "ioda/C/c_binding_macros.h"  // C_TRY and C_CATCH_AND_TERMINATE
#include "ioda/Group.h"

namespace ioda {
namespace C {
namespace Groups {

ioda_group* ioda_group_wrap(Group g);

c_ioda_group* ioda_group_wrap_inner(Group g) {
  c_ioda_group* res = nullptr;
  C_TRY;
  res = new c_ioda_group;
  res->g = g;
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

void ioda_group_destruct(ioda_group* g) {
  C_TRY;
  if (!g) return;
  delete g;
  C_CATCH_AND_TERMINATE;
}

ioda_VecString* ioda_group_list(const ioda_group* g) {
  ioda_VecString* ret = nullptr;
  C_TRY;
  if (!g) throw Exception("Cannot list a nullptr group.", ioda_Here());
  std::vector<std::string> vals = g->grp->g.list();
  ret = ioda::C::VecStrings::vecToVecString(vals);
  C_CATCH_RETURN_FREE(ret, NULL, ret);
}

int ioda_group_exists(const ioda_group* g, size_t sz, const char* name) {
  C_TRY;
  if (!g) throw Exception("Parameter 'g' is nullptr.", ioda_Here());
  if (!g->grp) throw Exception("Parameter 'g' is ill-formed.", ioda_Here());
  if (!name) throw Exception("Parameter 'name' is nullptr.", ioda_Here());
  C_CATCH_AND_RETURN((g->grp->g.exists(std::string(name, sz))) ? 1 : 0, -1);
}

ioda_group* ioda_group_create(ioda_group* g, size_t sz, const char* name) {
  ioda_group* res = nullptr;
  C_TRY;
  if (!g) throw Exception("Parameter 'g' is nullptr.", ioda_Here());
  if (!g->grp) throw Exception("Parameter 'g' is ill-formed.", ioda_Here());
  if (!name) throw Exception("Parameter 'name' is nullptr.", ioda_Here());
  res = ioda_group_wrap(g->grp->g.create(std::string(name, sz)));
  C_CATCH_RETURN_FREE(res, NULL, res);
}

ioda_group* ioda_group_open(const ioda_group* g, size_t sz, const char* name) {
  ioda_group* res = nullptr;
  C_TRY;
  if (!g) throw Exception("Parameter 'g' is nullptr.", ioda_Here());
  if (!g->grp) throw Exception("Parameter 'g' is ill-formed.", ioda_Here());
  if (!name) throw Exception("Parameter 'name' is nullptr.", ioda_Here());
  res = ioda_group_wrap(g->grp->g.open(std::string(name, sz)));
  C_CATCH_RETURN_FREE(res, NULL, res);
}

ioda_group* ioda_group_clone(const ioda_group* g) {
  ioda_group* res = nullptr;
  C_TRY;
  if (!g) throw Exception("Parameter 'g' is nullptr.", ioda_Here());
  if (!g->grp) throw Exception("Parameter 'g' is ill-formed.", ioda_Here());
  res = ioda_group_wrap(g->grp->g);
  C_CATCH_RETURN_FREE(res, NULL, res);
}

/*
IODA_DL ioda_has_attributes* ioda_group_atts(const ioda_group* g) {
  ioda_has_attributes* res = nullptr;
  C_TRY;
  if (!g) throw Exception("Parameter 'g' is nullptr.", ioda_Here());
  if (!g->grp) throw Exception("Parameter 'g' is ill-formed.", ioda_Here());
  res       = new ioda_has_attributes;
  res->atts = g->g.atts;
  C_CATCH_RETURN_FREE(res, NULL, res);
}

IODA_DL ioda_has_variables* ioda_group_vars(const ioda_group* g) {
  ioda_has_variables* res = nullptr;
  C_TRY;
  if (!g) throw Exception("Parameter 'g' is nullptr.", ioda_Here());
  res       = new ioda_has_variables;
  res->vars = g->g.vars;
  C_CATCH_RETURN_FREE(res, NULL, res);
}
*/

// This function gets exported to the rest of ioda's C interfaces
ioda_group* ioda_group_base() {
  ioda_group *res = nullptr;
  C_TRY;
  res = new ioda_group;
  res->destruct = &ioda_group_destruct;
  res->list = &ioda_group_list;
  res->exists = &ioda_group_exists;
  res->create = &ioda_group_create;
  res->open = &ioda_group_open;
  res->clone = &ioda_group_clone;
  res->grp = nullptr;
  res->atts = nullptr;
  res->vars = nullptr;
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

// This variable is re-declared as extern in ioda_c.cpp.
ioda_group general_c_ioda_group {
  &ioda_group_destruct, &ioda_group_list, &ioda_group_exists,
  &ioda_group_create, &ioda_group_open, &ioda_group_clone,
  nullptr, nullptr, nullptr
};

// This function gets exported to the rest of ioda's C interfaces
ioda_group* ioda_group_wrap(Group g) {
  ioda_group *res = nullptr;
  C_TRY;
  res = ioda_group_base();
  res->grp = ioda_group_wrap_inner(g);  // On failure, nullptr. This is safe for exceptions.
  res->atts = nullptr;
  res->vars = nullptr;
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

}  // end namespace Groups
}  // end namespace C
}  // end namespace ioda

/// @}
