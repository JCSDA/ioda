#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>


#include "ioda/Variables/Variable.h"
#include "ioda/Variables/Has_Variables.h"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_vecstring_c.hpp"
#include "ioda/C/ioda_variable_creation_parameters_c.hpp"
#include "ioda/C/ioda_variable_c.hpp"

extern "C" {

//ioda_has_variables_t * 
void * ioda_has_variables_c_alloc();

void ioda_has_variables_c_dtor(void **p);

void ioda_has_variables_c_clone(void **v,void *rhs);

//ioda_vecstring_t * 
void * ioda_has_variables_c_list(void *p);

bool ioda_has_variables_c_exists(void *p,int64_t n,const void *name_str);

bool ioda_has_variables_c_remove(void *p,int64_t n,const void *name_str);

//ioda_variable_t * 
void * ioda_has_variables_c_open(void *p,int64_t n,const void *name_str);

#define IODA_FUN(NAME,TYPE)\
void  * ioda_has_variables_c_create##NAME( void *p, int64_t name_sz, 		\
    const void *name_p, int64_t ndim, int64_t * dims);				\
                                                                                \
void * ioda_has_variables_c_create2##NAME( void *p, int64_t name_sz, 		\
    const void *name_p, int64_t ndim, int64_t * dims,				\
    int64_t *max_dims,void *creation_par_p);

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
