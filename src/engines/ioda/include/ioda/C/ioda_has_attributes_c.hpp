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
#include <iostream>

#include "ioda/Attributes/Attribute.h"
#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/C/ioda_attribute_c.hpp"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_vecstring_c.hpp"
#include "ioda/defs.h"

extern "C"
{

void  * ioda_has_attributes_c_alloc();

void ioda_has_attributes_c_dtor(void **v);

void ioda_has_attributes_c_clone(void **v,void *rhs);

//ioda_vecstring_t *  
void * ioda_has_attributes_c_list(void *v);

int ioda_has_attributes_c_exists(void * v,int64_t n,void *name);

bool ioda_has_attributes_c_remove(void *v,int64_t n,void *name);

bool ioda_has_attributes_c_rename(void * v,int64_t old_sz,const char *old_name,int64_t new_sz,const char *new_name);

//ioda_attribute_t * 
void * ioda_has_attributes_c_open(void *,int64_t n,const char *name);

#define IODA_FUN(NAME,TYPE)\
bool ioda_has_attributes_c_create##NAME (void *v,int64_t name_sz,const char *name,int64_t sz,int64_t *dims,void **va);

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16)
IODA_FUN(_int32,int32)
IODA_FUN(_int64,int64)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN
}

