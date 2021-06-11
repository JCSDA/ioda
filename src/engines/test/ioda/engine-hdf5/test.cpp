/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <iostream>

#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/defs.h"

int main(int, char**) {
  try {
    using namespace ioda;
    std::string name = Engines::HH::genUniqueName();
    auto f =
      Engines::HH::createMemoryFile(name, Engines::BackendCreateModes::Truncate_If_Exists, false, 10000);
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
