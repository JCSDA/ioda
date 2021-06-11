#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
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
#include "Attribute_c.h"
#include "Dimensions_c.h"
#include "Engines_c.h"
#include "Group_c.h"
#include "Has_Attributes_c.h"
#include "Has_Variables_c.h"
#include "String_c.h"
#include "Variable_Creation_Parameters_c.h"
#include "Variable_c.h"

#ifdef __cplusplus
extern "C" {
#endif

struct c_ioda {
  struct c_ioda_engines Engines;
  struct c_ioda_group Group;
  struct c_attribute Attribute;
  struct c_has_attributes Has_Attributes;
  struct c_dimensions Dimensions;
  struct c_strings Strings;
  struct c_variable Variable;
  struct c_variable_creation_parameters VariableCreationParams;
  struct c_has_variables Has_Variables;
};
IODA_DL struct c_ioda use_c_ioda();

#ifdef __cplusplus
}
#endif

/// @} // Close Doxygen block
