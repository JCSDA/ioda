/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>
#include "ioda/Attributes/Attribute.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/C/cxx_vector_string.hpp"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_dimensions_c.hpp"
#include "ioda/C/ioda_decls.hpp"
#include "gsl/gsl-lite.hpp"

extern "C" {

ioda_attribute_t  ioda_attribute_c_alloc();
void ioda_attribute_c_dtor(ioda_attribute_t *v);
bool ioda_attribute_c_is_allocated(ioda_attribute_t v);
void ioda_attribute_c_clone(ioda_attribute_t *t_p,ioda_attribute_t rhs_p);
ioda_dimensions_t  ioda_attribute_c_get_dimensions(ioda_attribute_t v);
bool ioda_attribute_c_write_str(ioda_attribute_t v,cxx_string_t data_p);
bool ioda_attribute_c_read_str(ioda_attribute_t v,cxx_string_t *data_p);

#define IODA_FUN(NAME,TYPE)\
bool ioda_attribute_c_read##NAME (ioda_attribute_t v,int64_t n, void ** data_p);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_char,char)
#undef IODA_FUN

#define IODA_FUN(NAME,TYPE)\
bool ioda_attribute_c_write##NAME (ioda_attribute_t v,int64_t n, TYPE * data);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_char,char)
#undef IODA_FUN

#define IODA_FUN(NAME,TYPE)\
int ioda_attribute_c_is_a##NAME (ioda_attribute_t v);

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
IODA_FUN(_str,std::string)
#undef IODA_FUN

} // end extern "C"
