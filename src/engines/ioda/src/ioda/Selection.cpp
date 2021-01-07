/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/Selection.h"

#include "ioda/defs.h"

namespace ioda {
// Note the NOLINTs. These are trivial and thus should never
// actually fail.
const Selection Selection::all({}, SelectionState::ALL);    // NOLINT
const Selection Selection::none({}, SelectionState::NONE);  // NOLINT

}  // namespace ioda
