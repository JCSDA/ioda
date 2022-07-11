/*
 * (C) Crown copyright 2020, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "oops/runs/Run.h"

#include "ioda/test/ioda/Sort.h"

int main(int argc,  char ** argv) {
  oops::Run run(argc, argv);
  ioda::test::Sort tests;
  return run.execute(tests);
}
