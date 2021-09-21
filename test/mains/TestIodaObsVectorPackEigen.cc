/*
 * (C) Copyright 2021- UCAR.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "oops/runs/Run.h"

#include "ioda/test/ioda/ObsVectorPackEigen.h"

int main(int argc,  char ** argv) {
  oops::Run run(argc, argv);
  ioda::test::ObsVectorPackEigen tests;
  return run.execute(tests);
}
