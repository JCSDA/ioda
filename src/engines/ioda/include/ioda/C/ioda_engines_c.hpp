/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/Engines/HH.h"
#include "ioda/C/ioda_decls.hpp"
#include "ioda/C/ioda_group_c.hpp"
#include "ioda/C/cxx_vector_string.hpp"
#include "ioda/C/ioda_c_utils.hpp"
#include "ioda/C/ioda_decls.hpp"

extern "C" {
ioda_group_t ioda_engines_c_obstore_create_root_group();
ioda_group_t ioda_engines_c_hh_create_file(const char * fname,int backend_mode);
ioda_group_t ioda_engines_c_hh_open_file(const char * fname,int backend_mode);
ioda_group_t ioda_engines_c_hh_create_memory_file(const char *fname,int64_t increment_len);
ioda_group_t ioda_engines_c_hh_open_memory_file(const char *fname,int64_t increment_len);
ioda_group_t ioda_engines_c_construct_from_command_line(cxx_vector_string_t vs,const char *default_filename);
}

