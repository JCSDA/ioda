#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "ioda/Variables/Has_Variables.h"
#include "ioda/Variables/Variable.h"
#include "ioda/C/ioda_c_utils.hpp"

extern "C" {

//ioda_variable_creation_parameters_t * 
void * ioda_variable_creation_parameters_c_alloc();

void ioda_variable_creation_parameters_c_dtor(void **p);

//ioda_variable_creation_parameters_t *  
void ioda_variable_creation_parameters_c_clone(void **t,void *p);

void ioda_variable_creation_parameters_c_chunking(void *p,bool do_chunking,int64_t ndims,const ptrdiff_t *chunks);
void ioda_variable_creation_parameters_c_no_compress(void *p);
void ioda_variable_creation_parameters_c_compress_with_gzip(void *p,int32_t level);
void ioda_variable_creation_parameters_c_compress_with_szip(void *p,int32_t pixsels_per_block,int32_t options);

#define IODA_FUN(NAME,TYPE) void ioda_variable_creation_parameters_c_set_fill_value##NAME(void *p, TYPE value);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN    
}
