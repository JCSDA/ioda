/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

extern "C" {
#define IODA_DECL(NAME) typedef void* NAME

IODA_DECL(ioda_dimensions_t);
IODA_DECL(ioda_variable_t);
IODA_DECL(ioda_attribute_t);
IODA_DECL(ioda_has_variables_t);
IODA_DECL(ioda_has_attributes_t);
IODA_DECL(ioda_group_t);
IODA_DECL(cxx_vector_string_t);
IODA_DECL(cxx_vector_int_t);
IODA_DECL(cxx_vector_dbl_t);
IODA_DECL(cxx_string_t);
IODA_DECL(ioda_variable_creation_parameters_t);
#undef IODA_DECL

#define VOID_TO_CXX(type,pin,pout) type * pout = reinterpret_cast< type * >(pin)
#define CXX_TO_VOID(p) reinterpret_cast< void * >((p))
#define DECL_TYPE(TYPE) typedef void* TYPE;

}
#undef IODA_DECL




