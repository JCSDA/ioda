/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_attribute
 * @{
 * \file Attribute_c.cpp
 * \brief @link ioda_attribute C bindings @endlink for ioda::Attribute
 */

#include "ioda/C/Attribute_c.h"

#include "./structs_c.h"
#include "ioda/Attributes/Attribute.h"
#include "ioda/C/c_binding_macros.h"

extern "C" {

void ioda_attribute_destruct(ioda_attribute* att) {
  C_TRY;
  Expects(att != nullptr);
  delete att;
  C_CATCH_AND_TERMINATE;
}

ioda_dimensions* ioda_attribute_get_dimensions(const ioda_attribute* att) {
  ioda_dimensions* res = nullptr;
  C_TRY;
  Expects(att != nullptr);
  res = new ioda_dimensions;
  Expects(res != nullptr);
  res->d = att->att.getDimensions();
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

// isA
#define IODA_ATTRIBUTE_ISA_IMPL(funcnamestr, Type)                                                 \
  IODA_DL int funcnamestr(const ioda_attribute* att) {                                             \
    C_TRY;                                                                                         \
    Expects(att != nullptr);                                                                       \
    bool res = att->att.isA<Type>();                                                               \
    C_CATCH_AND_RETURN((res) ? 1 : 0, -1);                                                         \
  }

C_TEMPLATE_FUNCTION_DEFINITION(ioda_attribute_isa, IODA_ATTRIBUTE_ISA_IMPL);

// write

#define IODA_ATTRIBUTE_WRITE(funcnamestr, Type)                                                    \
  IODA_DL bool funcnamestr(ioda_attribute* att, size_t sz, const Type* vals) {                     \
    C_TRY;                                                                                         \
    Expects(att != nullptr);                                                                       \
    Expects(vals != nullptr);                                                                      \
    att->att.write<Type>(gsl::span<const Type>(vals, sz));                                         \
    C_CATCH_AND_RETURN(true, false);                                                               \
  }

C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_attribute_write, IODA_ATTRIBUTE_WRITE);

IODA_DL bool ioda_attribute_write_str(ioda_attribute* att, size_t sz, const char* const* vals) {
  C_TRY;
  Expects(att != nullptr);
  Expects(vals != nullptr);
  std::vector<std::string> vdata(sz);
  for (size_t i = 0; i < sz; ++i) vdata[i] = std::string(vals[i]);
  att->att.write<std::string>(vdata);
  C_CATCH_AND_RETURN(true, false);
}

// read

#define IODA_ATTRIBUTE_READ(funcnamestr, Type)                                                     \
  IODA_DL bool funcnamestr(const ioda_attribute* att, size_t sz, Type* vals) {                     \
    C_TRY;                                                                                         \
    Expects(att != nullptr);                                                                       \
    Expects(vals != nullptr);                                                                      \
    att->att.read<Type>(gsl::span<Type>(vals, sz));                                                \
    C_CATCH_AND_RETURN(true, false);                                                               \
  }

C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_attribute_read, IODA_ATTRIBUTE_READ);

IODA_DL ioda_string_ret_t* ioda_attribute_read_str(const ioda_attribute* att) {
  C_TRY;
  Expects(att != nullptr);
  std::vector<std::string> vdata;
  att->att.read<std::string>(vdata);

  auto* res = create_str_vector_c(vdata);
  C_CATCH_AND_RETURN(res, nullptr);
}
}

/// @}
