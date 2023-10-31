/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
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

ioda_variable_t  ioda_variable_c_alloc();
void ioda_variable_c_dtor(ioda_variable_t *p);
void ioda_variable_c_clone(ioda_variable_t *t_p,ioda_variable_t rhs_p);
ioda_has_attributes_t ioda_variable_c_has_attributes(ioda_variable_t p);
ioda_dimensions_t ioda_variable_c_get_dimensions(ioda_variable_t p);
bool ioda_variable_c_resize(ioda_variable_t p,int64_t n,void *dim_ptr);
bool ioda_variable_c_attach_dim_scale(ioda_variable_t p,int32_t dim_n,ioda_variable_t var_ptr);
bool ioda_variable_c_detach_dim_scale(ioda_variable_t p,int32_t dim_n,ioda_variable_t var_ptr);
bool ioda_variable_c_set_dim_scale(ioda_variable_t p,int64_t ndim,ioda_variable_t *var_ptr);
int32_t ioda_variable_c_is_dim_scale(ioda_variable_t p);
bool ioda_variable_c_set_is_dimension_scale(ioda_variable_t p,int64_t sz,const char *name_p);
int64_t ioda_variable_c_get_dimension_scale_name(ioda_variable_t p,int64_t n,char **out_p);
int32_t ioda_variable_c_is_dimension_scale_attached(ioda_variable_t p,int32_t dim_num,ioda_variable_t scale_p);

#define IODA_FUN(NAME,TYPE) \
int32_t ioda_variable_c_is_a##NAME (ioda_variable_t  p);

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
bool ioda_variable_c_write##NAME(ioda_variable_t p,int64_t n, const TYPE *data);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_variable_c_write_char(ioda_variable_t p,int64_t n,const char * vptr);
bool ioda_variable_c_write_str(ioda_variable_t p,cxx_vector_string_t vstr_p);

#define IODA_FUN(NAME,TYPE)\
bool ioda_variable_c_read##NAME(ioda_variable_t p,int64_t n,void **dptr);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_variable_c_read_char(ioda_variable_t p,int64_t n,void **vstr);
bool ioda_variable_c_read_str(void *p,int64_t n,cxx_vector_string_t *vstr);

}
