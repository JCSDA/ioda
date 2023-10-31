/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>

#include "ioda/Group.h"
#include "ioda/C/cxx_vector_string.hpp"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_decls.hpp"

extern "C" {
ioda_group_t ioda_group_c_alloc();
void ioda_group_c_dtor(ioda_group_t *p);
void ioda_group_c_clone(ioda_group_t *t_p,ioda_group_t rhs_p);
cxx_vector_string_t ioda_group_c_list(ioda_group_t p);
int ioda_group_c_exists(ioda_group_t p,int64_t sz,const char *name);
ioda_group_t ioda_group_c_create(ioda_group_t p,int64_t sz,const char *name);
ioda_group_t ioda_group_c_open(ioda_group_t p,int64_t sz,const char *name);
ioda_has_attributes_t ioda_group_c_has_attributes(ioda_group_t g_p);
ioda_has_variables_t ioda_group_c_has_variables(ioda_group_t g_p);
}

