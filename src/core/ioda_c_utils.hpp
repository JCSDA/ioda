/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <cstdio>
#include <cstdlib>
#include <ctime>

#define VOID_TO_CXX(type, pin, pout) type *pout = reinterpret_cast<type *>(pin)
#define CXX_TO_VOID(p) reinterpret_cast<void *>((p))
#define DECL_TYPE(TYPE) typedef void *TYPE;
DECL_TYPE(cxx_string_t)

extern "C" {

void fatal_error();

char *Strdup(const char *s);

void *Malloc(size_t n);

}  // end extern "C"

namespace putils {

struct Stopwatch {
  double acc;
  struct timespec ts;
  struct timespec tf;
  Stopwatch() : acc(0.) {}
  constexpr void clear() noexcept { acc = 0.; }
  void start() { clock_gettime(CLOCK_MONOTONIC, &ts); }
  void stop() {
    clock_gettime(CLOCK_MONOTONIC, &tf);
    acc += (tf.tv_sec - ts.tv_sec) + 1.e-9 * (tf.tv_nsec - ts.tv_nsec);
  }
  constexpr double elapsed_time() const noexcept { return acc; }
};

}  // end namespace putils
