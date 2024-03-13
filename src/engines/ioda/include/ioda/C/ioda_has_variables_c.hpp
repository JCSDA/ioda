/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>


#include "ioda/Variables/Variable.h"
#include "ioda/Variables/Has_Variables.h"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/cxx_vector_string.hpp"
#include "ioda/C/ioda_variable_creation_parameters_c.hpp"
#include "ioda/C/ioda_variable_c.hpp"
#include "ioda/C/ioda_decls.hpp"

extern "C" {

ioda_has_variables_t ioda_has_variables_c_alloc();
void ioda_has_variables_c_dtor(ioda_has_variables_t* p);
void ioda_has_variables_c_clone(ioda_has_variables_t *v,ioda_has_variables_t rhs_p);
cxx_vector_string_t  ioda_has_variables_c_list(ioda_has_variables_t p);
bool ioda_has_variables_c_exists(ioda_has_variables_t p,int64_t n,const char *name);
bool ioda_has_variables_c_remove(ioda_has_variables_t p,int64_t n,const char *name_str);
ioda_variable_t ioda_has_variables_c_open(ioda_has_variables_t p,int64_t n,const char *name);

#define IODA_FUN(NAME,TYPE)\
ioda_variable_t ioda_has_variables_c_create##NAME(ioda_has_variables_t p,int64_t name_sz,\
    const char  *name, int64_t ndims, int64_t *dims);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_ldouble,long double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_uint16,uint16_t)
IODA_FUN(_uint32,uint32_t)
IODA_FUN(_uint64,uint64_t)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN

#define IODA_FUN(NAME,TYPE)\
ioda_variable_t  ioda_has_variables_c_create2##NAME(ioda_has_variables_t p,\
    int64_t name_sz,\
    const char *name, int64_t ndims, int64_t * dims,\
    int64_t * max_dims,ioda_variable_creation_parameters_t creation_p);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_ldouble,long double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_uint16,uint16_t)
IODA_FUN(_uint32,uint32_t)
IODA_FUN(_uint64,uint64_t)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN
}
