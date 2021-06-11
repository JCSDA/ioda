/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <iostream>
#include <vector>

#include "ioda/Engines/Capabilities.h"
#include "ioda/Engines/Factory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

template <class T>
bool testVar(ioda::Group &f, const std::string &varname, const std::vector<T> &data,
             const std::vector<ioda::Dimensions_t> &dims, bool chunk, bool gzip,
             bool szip)  // NOLINT(google-runtime-references): f can be a non-const reference
{
  try {
    std::cout << "Testing variable " << varname << std::endl;
    std::cout << "\tCreating..." << std::endl;
    using namespace ioda;
    VariableCreationParameters params;
    if (chunk) {
      params.chunk = true;
      params.chunks = dims;
      if (gzip) {
        params.compressWithGZIP();
      } else if (szip) {
        params.compressWithSZIP();
      }
    }
    ioda::Variable v_i = f.vars.create<T>(varname, dims, dims, params);
    std::cout << "\t\tSuccess.\n\tWriting..." << std::endl;

    v_i.write<T>(data);

    std::cout << "\t\tSuccess.\n\tCheck that the variable's chunking options match the reference."
              << std::endl;

    ioda::Variable v = f.vars.open(varname);  // Re-opening to make sure backend persists the params.

    auto vChunks = v.getChunkSizes();
    if (chunk) {
      if (vChunks.size() != dims.size()) {
        std::cout << "\t\tFailed. Chunk was not set." << std::endl;
        return false;
      }
      for (size_t i = 0; i < vChunks.size(); ++i) {
        if (vChunks[i] != dims[i]) {
          std::cout << "\t\tFailed. Chunk size does not match reference." << std::endl;
          return false;
        }
      }
    } else {
      if (!vChunks.empty()) {
        std::cout << "\t\tFailed. Chunk should not be set." << std::endl;
        return false;
      }
    }

    std::cout << "\t\tSuccess.\n\tCheck compression options." << std::endl;

    auto vGZIP = v.getGZIPCompression();
    if (gzip != vGZIP.first) {
      std::cout << "\t\tFailed. GZIP enable flag does not match reference." << std::endl;
      return false;
    }
    if (gzip && !vGZIP.second) {
      std::cout << "\t\tFailed. GZIP enabled, but at zero compression level." << std::endl;
      return false;
    }

    auto vSZIP = v.getSZIPCompression();
    if (szip != std::get<0>(vSZIP)) {
      std::cout << "\t\tFailed. SZIP enable flag does not match reference." << std::endl;
      return false;
    }
    if (szip) {
      std::cout << "\t\tSZIP compression was enabled." << std::endl;
      // TODO(rhoneyager): H5Pset_szip's flags do not match those returned by the HDF5 filter!
      // These should be decoded and checked in the future.
    }

    std::cout << "\t\tSuccess." << std::endl;

    return true;
  } catch (...) {
    std::cout << "\t\tFailed with exception." << std::endl;
    return false;
  }
}

int main(int argc, char **argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-filters.hdf5");

    // These tests try to read and write fill values.
    // We try basic numbers, fixed-length strings and variable-length
    // strings.
    int good = 0, bad = 0;

    (testVar<std::string>(f, "varlen-string-test", {"This is a test"}, {1}, false, false, false)) ? good++
                                                                                                  : bad++;
    (testVar<int>(f, "int-test-nochunks", {1, 2, 3, 4}, {2, 2}, false, false, false)) ? good++ : bad++;
    (testVar<int>(f, "int-test-chunks", {2, 3, 4, 5}, {2, 2}, true, false, false)) ? good++ : bad++;
    (testVar<int>(f, "int-test-chunks-gzip", {1, -4, 9, -16}, {2, 2}, true, true, false)) ? good++ : bad++;
    // Only run if the engine will not fail on SZIP compression.
    if (f.getCapabilities().canCompressWithSZIP != ioda::Engines::Capability_Mask::Unsupported)
      (testVar<int>(f, "int-test-chunks-szip", {9, 4, 3, -1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {4, 4},
                    true, false, true))
        ? good++
        : bad++;
    else
      std::cout << "\tSkipping SZIP checks since the backend disables them." << std::endl;

    std::cout << "\n\nSuccesses: " << good << "\nFailures: " << bad << std::endl;
    return (bad) ? 1 : 0;
  } catch (const std::exception &e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
}
