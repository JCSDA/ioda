/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/Selection.h"

#include "ioda/defs.h"
#include "ioda/Variables/Variable.h"

namespace ioda {
// Note the NOLINTs. These are trivial and thus should never
// actually fail.
const Selection Selection::all({}, SelectionState::ALL);    // NOLINT
const Selection Selection::none({}, SelectionState::NONE);  // NOLINT

Selections::SelectionBackend_t Selection::concretize(const Variable& var) const {
  if (backend_) return backend_;
  backend_ = var.instantiateSelection(*this);
  return backend_;
}

namespace Selections {
InstantiatedSelection::~InstantiatedSelection() = default;
}  // namespace Selections

}  // namespace ioda
