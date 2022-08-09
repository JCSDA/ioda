#pragma once
/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

/*! \defgroup ioda_c_api API
 * \brief C API
 * \ingroup ioda_c
 *
 * @{
 * \file ioda_c.h
 * \brief C bindings for ioda-engines. Provides a class-like structure.
 */

#include <stdbool.h>

#include "../defs.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct ioda_engines;
  struct ioda_group;
  struct ioda_string;
  struct ioda_VecString;
  //struct c_dimensions;
  //struct c_variable_creation_parameters;

/// \note Fortran interface must match!
struct ioda_c_interface {
  const struct ioda_engines* Engines;
  const struct ioda_group* Groups;
  const struct ioda_string* Strings;
  const struct ioda_VecString* VecStrings;
  //const struct c_dimensions* Dimensions;
  //const struct c_variable_creation_parameters* VariableCreationParams;
};
IODA_DL const struct ioda_c_interface* get_ioda_c_interface();

#ifdef __cplusplus
}
#endif

/// @} // Close Doxygen block
