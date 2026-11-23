/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>

#include "ioda/core/ioda_c_utils.hpp"

extern "C" {

void fatal_error() { throw std::exception(); }

char *Strdup(const char *s) {
  char *r = strdup(s);
  if (r == nullptr) {
    fprintf(stderr, "Strdup failed %s\n", strerror(errno));
    fatal_error();
  }
  return r;
}

void *Malloc(size_t n) {
  void *p = malloc(n);
  if (p) return p;
  fprintf(stderr, "malloc failed for size %lu\n", n);
  fatal_error();
  return 0x0;
}

}  // extern "C"
