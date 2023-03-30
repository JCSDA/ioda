#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <stdexcept>

#include "ioda/Group.h"
#include "ioda/C/ioda_vecstring_c.hpp"
#include "ioda/C/ioda_c_utils.hpp"

extern "C" {

void * ioda_group_c_alloc();

void ioda_group_c_dtor(void **p);

//ioda_vecstring_t * 
void * ioda_group_c_list(void *p);

int ioda_group_c_exists(void *p,int64_t sz,const void * name_p);

void * ioda_group_c_create(void *p,int64_t sz,const void *name_p);

void * ioda_group_c_open(void *p,int64_t sz,const void *name_p);

void ioda_group_c_clone(void **v,void *rhs);

void * ioda_group_c_has_attributes( void * g_p);

void * ioda_group_c_has_variables( void *g_p);

}

