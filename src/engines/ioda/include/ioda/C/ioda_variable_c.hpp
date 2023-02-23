#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include "ioda/Variables/Variable.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/C/ioda_has_attributes_c.hpp"
#include "ioda/C/ioda_dimensions_c.hpp"
#include "ioda/C/ioda_c_utils.hpp"


extern "C" {

//ioda_variable_t * 
void * ioda_variable_c_alloc();

void ioda_variable_c_dtor(void **p);

void ioda_variable_c_clone(void **v,void *rhs);

///ioda_has_attributes_t * 
void * ioda_variable_c_has_attributes(void *p);
//ioda_dimensions_t * 
void * ioda_variable_c_get_dimensions(void *p);
bool ioda_variable_c_resize(void *p,int64_t n,void *dim_ptr);
bool ioda_variable_c_attach_dim_scale(void *p,int32_t dim_n,void *var_ptr);
bool ioda_variable_detach_dim_scale(void *p,int32_t dim_n,void *var_ptr);
bool ioda_variable_set_dim_scale(void *p,int64_t dim_n,void *var_ptr);
int32_t ioda_variable_c_is_dim_scale(void *p);
bool ioda_variable_c_set_is_dimension_scale(void *p,int64_t sz,void *name_p);
int64_t ioda_variable_c_get_dimension_scale_name(void *p,int64_t n,void **out_p);
int32_t ioda_variable_c_is_dimension_scale_attached(void *p,int32_t dim_num,void *scale_p);

#define IODA_FUN(NAME,TYPE) \
int32_t ioda_variable_c_is_a##NAME (void * p);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_ldouble,long double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_int16,uint16_t)
IODA_FUN(_int32,uint32_t)
IODA_FUN(_int64,uint64_t)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN
    
#define IODA_FUN(NAME,TYPE)\
bool ioda_variable_c_write##NAME(void *p,int64_t n, const TYPE *data);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_variable_c_write_char(void *p,int64_t n,void * vptr);

bool ioda_variable_c_write_str(void *p,void *vstr_p);

#define IODA_FUN(NAME) bool ioda_variable_c_read##NAME (void *p,int64_t n, void **data);

IODA_FUN(_float)
IODA_FUN(_double)
IODA_FUN(_char)
IODA_FUN(_int16)
IODA_FUN(_int32)
IODA_FUN(_int64)
IODA_FUN(_str)
#undef IODA_FUN

}