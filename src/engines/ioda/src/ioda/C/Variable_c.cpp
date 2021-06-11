/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_variable
 *
 * @{
 * \file Variable_c.cpp
 * \brief @link ioda_variable C bindings @endlink for ioda::Variable
 */

#include "ioda/C/Variable_c.h"

#include "./structs_c.h"
#include "ioda/C/c_binding_macros.h"
#include "ioda/Exception.h"
#include "ioda/Types/Type.h"
#include "ioda/Variables/Variable.h"

extern "C" {

void ioda_variable_destruct(ioda_variable* var) {
  C_TRY;
  Expects(var != nullptr);
  delete var;
  C_CATCH_AND_TERMINATE;
}

ioda_has_attributes* ioda_variable_atts(const ioda_variable* var) {
  ioda_has_attributes* res = nullptr;
  C_TRY;
  Expects(var != nullptr);
  res       = new ioda_has_attributes;
  res->atts = var->var.atts;
  C_CATCH_RETURN_FREE(res, NULL, res);
}

ioda_dimensions* ioda_variable_get_dimensions(const ioda_variable* var) {
  ioda_dimensions* res = nullptr;
  C_TRY;
  Expects(var != nullptr);
  res = new ioda_dimensions;
  Expects(res != nullptr);
  res->d = var->var.getDimensions();
  C_CATCH_RETURN_FREE(res, nullptr, res);
}

bool ioda_variable_resize(struct ioda_variable* var, size_t N, const long* newDims) {
  C_TRY;
  Expects(var != nullptr);
  ioda::Selection::VecDimensions_t nd(N);
  for (size_t i = 0; i < N; ++i)
    nd[i] = gsl::narrow<ioda::Selection::VecDimensions_t::value_type>(newDims[i]);
  var->var.resize(nd);
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_variable_attachDimensionScale(struct ioda_variable* var, unsigned int DimensionNumber,
                                        const struct ioda_variable* scale) {
  C_TRY;
  Expects(var != nullptr);
  Expects(scale != nullptr);
  var->var.attachDimensionScale(DimensionNumber, scale->var);
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_variable_detachDimensionScale(struct ioda_variable* var, unsigned int DimensionNumber,
                                        const struct ioda_variable* scale) {
  C_TRY;
  Expects(var != nullptr);
  Expects(scale != nullptr);
  var->var.detachDimensionScale(DimensionNumber, scale->var);
  C_CATCH_AND_RETURN(true, false);
}

bool ioda_variable_setDimScale(struct ioda_variable* var, size_t N,
                               const struct ioda_variable* const* dims) {
  C_TRY;
  Expects(var != nullptr);
  Expects(dims != nullptr);
  std::vector<ioda::Variable> newDims;
  newDims.reserve(N);
  for (size_t i = 0; i < N; ++i) {
    Expects(dims[i] != nullptr);
    newDims.push_back(dims[i]->var);
  }
  var->var.setDimScale(newDims);
  C_CATCH_AND_RETURN(true, false);
}

int ioda_variable_isDimensionScale(const struct ioda_variable* var) {
  C_TRY;
  Expects(var != nullptr);
  bool res = var->var.isDimensionScale();
  C_CATCH_AND_RETURN((res) ? 1 : 0, -1);
}

bool ioda_variable_setIsDimensionScale(struct ioda_variable* var, size_t sz,
                                       const char* dimensionScaleName) {
  C_TRY;
  Expects(var != nullptr);
  Expects(dimensionScaleName != nullptr);
  var->var.setIsDimensionScale(std::string(dimensionScaleName, sz));
  C_CATCH_AND_RETURN(true, false);
}

size_t ioda_variable_getDimensionScaleName(const struct ioda_variable* var, size_t N, char* out) {
  C_TRY;
  Expects(var != nullptr);
  std::string name;
  var->var.getDimensionScaleName(name);
  if (name.size() == SIZE_MAX) throw ioda::Exception(
    "Dimension scale name is too large.", ioda_Here());
  if (!out) return name.size() + 1;
  Expects(out != nullptr);
  ioda::detail::COMPAT_strncpy_s(out, N, name.data(), name.size() + 1);

  C_CATCH_AND_RETURN(name.size() + 1, 0);
}

int ioda_variable_isDimensionScaleAttached(const struct ioda_variable* var,
                                           unsigned int DimensionNumber,
                                           const struct ioda_variable* scale) {
  C_TRY;
  Expects(var != nullptr);
  Expects(scale != nullptr);
  bool ret = var->var.isDimensionScaleAttached(DimensionNumber, scale->var);
  C_CATCH_AND_RETURN((ret) ? 1 : 0, -1);
}

// isA
#define IODA_VARIABLE_ISA_IMPL(funcnamestr, Type)                                                  \
  IODA_DL int funcnamestr(const ioda_variable* var) {                                              \
    C_TRY;                                                                                         \
    Expects(var != nullptr);                                                                       \
    bool ret = var->var.isA<Type>();                                                               \
    C_CATCH_AND_RETURN((ret) ? 1 : 0, -1);                                                         \
  }

C_TEMPLATE_FUNCTION_DEFINITION(ioda_variable_isa, IODA_VARIABLE_ISA_IMPL);

// write

#define IODA_VARIABLE_WRITE_FULL(funcnamestr, Type)                                                \
  IODA_DL bool funcnamestr(ioda_variable* var, size_t sz, const Type* vals) {                      \
    C_TRY;                                                                                         \
    Expects(var != nullptr);                                                                       \
    Expects(vals != nullptr);                                                                      \
    var->var.write<Type>(gsl::span<const Type>(vals, sz));                                         \
    C_CATCH_AND_RETURN(true, false);                                                               \
  }

C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_write_full, IODA_VARIABLE_WRITE_FULL);

IODA_DL bool ioda_variable_write_full_str(ioda_variable* var, size_t sz, const char* const* vals) {
  C_TRY;
  Expects(var != nullptr);
  Expects(vals != nullptr);
  std::vector<std::string> vdata(sz);
  for (size_t i = 0; i < sz; ++i) vdata[i] = std::string(vals[i]);
  var->var.write<std::string>(vdata);
  C_CATCH_AND_RETURN(true, false);
}

// read

#define IODA_VARIABLE_READ_FULL(funcnamestr, Type)                                                 \
  IODA_DL bool funcnamestr(const ioda_variable* var, size_t sz, Type* vals) {                      \
    C_TRY;                                                                                         \
    Expects(var != nullptr);                                                                       \
    Expects(vals != nullptr);                                                                      \
    var->var.read<Type>(gsl::span<Type>(vals, sz));                                                \
    C_CATCH_AND_RETURN(true, false);                                                               \
  }

C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_read_full, IODA_VARIABLE_READ_FULL);

IODA_DL ioda_string_ret_t* ioda_variable_read_full_str(const ioda_variable* var) {
  C_TRY;
  Expects(var != nullptr);
  std::vector<std::string> vdata;
  var->var.read<std::string>(vdata);

  C_CATCH_AND_RETURN(create_str_vector_c(vdata), nullptr);
}
}

/// @}
