/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_variable_creation_parameters
 *
 * @{
 * \file Variable_Creation_Parameters_c.cpp
 * \brief @link ioda_variable_creation_parameters C bindings @endlink for
 * ioda::VariableCreationParameters, used in ioda::Has_Variables::create.
 */

#include "./structs_c.h"
#include "ioda/C/Group_c.h"
#include "ioda/C/c_binding_macros.h"
#include "ioda/Group.h"

extern "C" {

void ioda_variable_creation_parameters_destruct(ioda_variable_creation_parameters* params) {
  C_TRY;
  Expects(params != nullptr);
  delete params;
  C_CATCH_AND_TERMINATE;
}

ioda_variable_creation_parameters* ioda_variable_creation_parameters_create() {
  C_TRY;
  auto* res = new ioda_variable_creation_parameters;
  Expects(res != nullptr);
  C_CATCH_AND_RETURN(res, NULL);
}

ioda_variable_creation_parameters* ioda_variable_creation_parameters_clone(
  const ioda_variable_creation_parameters* p) {
  C_TRY;
  Expects(p != nullptr);
  auto* res = new ioda_variable_creation_parameters;
  Expects(res != nullptr);
  res->params = p->params;
  C_CATCH_AND_RETURN(res, NULL);
}

void ioda_variable_creation_parameters_chunking(struct ioda_variable_creation_parameters* p,
                                                bool doChunking, size_t Ndims,
                                                const ptrdiff_t* chunks) {
  C_TRY;
  Expects(p != nullptr);
  p->params.chunk = doChunking;
  if (doChunking) {
    Expects(chunks != nullptr);
    p->params.chunks.resize(Ndims);
    for (size_t i = 0; i < Ndims; ++i)
      p->params.chunks[i] = gsl::narrow<ioda::Dimensions_t>(chunks[i]);
  }
  C_CATCH_AND_TERMINATE;
}

void ioda_variable_creation_parameters_noCompress(struct ioda_variable_creation_parameters* p) {
  C_TRY;
  Expects(p != nullptr);
  p->params.noCompress();
  C_CATCH_AND_TERMINATE;
}

void ioda_variable_creation_parameters_compressWithGZIP(struct ioda_variable_creation_parameters* p,
                                                        int level) {
  C_TRY;
  Expects(p != nullptr);
  p->params.compressWithGZIP(level);
  C_CATCH_AND_TERMINATE;
}

void ioda_variable_creation_parameters_compressWithSZIP(struct ioda_variable_creation_parameters* p,
                                                        unsigned PixelsPerBlock, unsigned options) {
  C_TRY;
  Expects(p != nullptr);
  p->params.compressWithSZIP(PixelsPerBlock, options);
  C_CATCH_AND_TERMINATE;
}

// Fill value
#define IODA_VCP_FILL_IMPL(funcnamestr, Type)                                                      \
  IODA_DL void funcnamestr(struct ioda_variable_creation_parameters* p, Type value) {              \
    C_TRY;                                                                                         \
    Expects(p != nullptr);                                                                         \
    p->params.setFillValue<Type>(value);                                                           \
    C_CATCH_AND_TERMINATE;                                                                         \
  }

C_TEMPLATE_FUNCTION_DEFINITION_NOSTR(ioda_variable_creation_parameters_setFillValue,
                                     IODA_VCP_FILL_IMPL);
}

/// @}
