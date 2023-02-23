#pragma once
/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/Engines/HH.h"
#include "ioda/C/ioda_group_c.hpp"
#include "ioda/C/ioda_vecstring_c.hpp"
#include "ioda/C/ioda_c_utils.hpp"

extern "C" {
void * ioda_engines_c_obstore_create_root_group();
void * ioda_engines_c_hh_create_memory_file(const void *name,int64_t increment_len);
void * ioda_engines_c_hh_create_memory_file(const void *name,int64_t increment_len);
void * ioda_engines_c_hh_open_file(const void * name,int backend_mode);
void * ioda_engines_c_hh_create_file(const void * name,int backend_mode);
void * ioda_engines_c_construct_from_command_line(void *vs,const void *def_name);
}

