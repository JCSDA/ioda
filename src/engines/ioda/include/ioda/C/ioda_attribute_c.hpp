#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include "ioda/Attributes/Attribute.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/C/ioda_vecstring_c.hpp"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_dimensions_c.hpp"

/// no need to guard this with __cpluscplus, if this isn't using the c++ compiler it won't work period
extern "C" {

// \brief allocate the data_ptr for ioda_attribute_c
// ioda::Atrribute

void * ioda_attribute_c_alloc();

void ioda_attribute_c_dtor(void **v);

bool ioda_attribute_c_is_allocated(void *p);

void ioda_attribute_c_clone(void **p,void *rhs);

//
//  first pointer is the fortran c_ptr for ioda_attributes
//  second pointer is the fortran c_ptr for ioda_dimensions
// ioda::Dimensions
void * ioda_attribute_c_get_dimensions(void *v);

#define IODA_FUN(NAME,TYPE) bool ioda_attribute_c_write##NAME (void *v,int64_t n, const TYPE * data);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_attribute_c_write_char(void *v,int64_t n,const void *data);

bool ioda_attribute_c_write_str(void *v,void *vvs);

#define IODA_FUN(NAME,TYPE) bool ioda_attribute_c_read##NAME (void *v,int64_t n, TYPE * data);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
#undef IODA_FUN

bool ioda_attribute_c_read_char(void *v,int64_t n,void *data);

bool  ioda_attribute_c_read_str(void *v,void * vvs);

#define IODA_FUN(NAME,TYPE) int ioda_attribute_c_is_a##NAME (void *v);

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

} // end extern "C"
