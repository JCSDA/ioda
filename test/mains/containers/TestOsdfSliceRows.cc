/*
 * (C) Copyright 2026, UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/test/containers/OsdfSliceRows.h"
#include "oops/runs/Run.h"

int main(int argc, char **argv) {
  oops::Run run(argc, argv);
  ioda::test::OsdfSliceRows tests;
  return run.execute(tests);
}
