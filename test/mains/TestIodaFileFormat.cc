/*
 * (C) Copyright 2021 Met Office UK
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/runs/Run.h"

#include "ioda/test/ioda/FileFormat.h"

int main(int argc,  char ** argv) {
  oops::Run run(argc, argv);
  ioda::test::FileFormat tests;
  return run.execute(tests);
}
