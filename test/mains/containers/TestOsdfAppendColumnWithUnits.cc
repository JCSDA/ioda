/*
 * (C) Crown copyright 2026 MetOffice
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/test/containers/OsdfAppendColumnWithUnits.h"
#include "oops/runs/Run.h"

int main(int argc, char **argv) {
  oops::Run run(argc, argv);
  ioda::test::OsdfAppendColumnWithUnits tests;
  return run.execute(tests);
}
