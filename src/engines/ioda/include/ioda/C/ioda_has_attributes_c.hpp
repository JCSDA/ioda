/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <iostream>
#include <string>
#include "ioda/Attributes/Attribute.h"
#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/C/ioda_attribute_c.hpp"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/cxx_vector_string.hpp"
#include "ioda/defs.h"
#include "ioda/C/ioda_decls.hpp"

extern "C"
{
ioda_has_attributes_t ioda_has_attributes_c_alloc();
void ioda_has_attributes_c_dtor(ioda_has_attributes_t *v);
ioda_has_attributes_t  ioda_has_attributes_c_list(ioda_has_attributes_t v);
void ioda_has_attributes_c_clone(ioda_has_attributes_t *t_p,ioda_has_attributes_t rhs_p);
int ioda_has_attributes_c_exists(ioda_has_attributes_t v,int64_t n,const char  *name);
bool ioda_has_attributes_c_remove(ioda_has_attributes_t v,int64_t n,const char *name);
bool ioda_has_attributes_c_rename(ioda_has_attributes_t v,int64_t old_sz,
    const char *old_name,int64_t new_sz,const char *new_name);
ioda_has_attributes_t ioda_has_attributes_c_open(ioda_has_attributes_t v,int64_t n,const char *name);

#define IODA_FUN(NAME,TYPE)\
bool ioda_has_attributes_c_create##NAME (ioda_has_attributes_t v,int64_t name_sz,\
    const char *name, int64_t sz,void **dims_p,ioda_attribute_t *Attr);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_str,std::string)
#undef IODA_FUN
}

