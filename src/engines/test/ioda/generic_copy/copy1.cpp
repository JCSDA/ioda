/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** \file copy1.cpp Tests generic copying functions. Used for benchmarking.
 * \details
 * This test tries to copy data between backends. The initial data file
 * is passed form the command line.
 **/

#include <iostream>
#include <vector>

#include "ioda/Copying.h"
#include "ioda/Engines/HH.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/Group.h"

int main(int argc, char** argv) {
  try {
    using namespace std;
    using namespace ioda;

    if (argc < 3 || argc > 4) throw std::logic_error("Bad number of arguments.");
    string srcfile{argv[1]};
    string srcmode{argv[2]};
    Group src = (srcmode == "memmap")
                  ? Engines::HH::openMemoryFile(srcfile, Engines::BackendOpenModes::Read_Only)
                  : Engines::HH::openFile(srcfile, Engines::BackendOpenModes::Read_Only);

    std::map<string, Group> grps{
      {"HH-mem", Engines::HH::createMemoryFile(Engines::HH::genUniqueName(),
                                               Engines::BackendCreateModes::Truncate_If_Exists)},
      {"ObsStore", Engines::ObsStore::createRootGroup()}};

    auto doTest = [](const std::string& name, Group src, Group dest) {
      std::cout << "Testing " << name << std::endl;
      ScaleMapping sm;
      sm.autocreate = true;
      ObjectSelection dgrp{dest};
      ioda::copy({src}, dgrp, sm);
      std::cout << "Done testing " << name << std::endl;
    };

    if (argc == 4) {
      // Attempt timing information for only one backend.
      // Useful when running in a profiler.
      // If name matches one of the special strings, use that
      // in-memory backend. Otherwise, create an output file and
      // use that as the backend. Output name equals the final
      // string argument.
      string backend{argv[3]};
      if (grps.count(backend))
        doTest(backend, src, grps.at(backend));
      else
        doTest(backend, src,
               Engines::HH::createFile(backend, Engines::BackendCreateModes::Truncate_If_Exists));
    } else {
      // Attempt timing information for all backends
      for (auto& b : grps) doTest(b.first, src, b.second);
    }

    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unhandled exception\n";
    return 2;
  }
}
