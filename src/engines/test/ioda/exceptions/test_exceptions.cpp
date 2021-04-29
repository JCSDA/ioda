/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <exception>
#include <iostream>
#include <string>

#include "ioda/Exception.h"

void throws_exception_inner() {
  throw ioda::Exception("This is the inner exception.", ioda_Here())
    .add<int>("some-value", 24)
    .add<double>("pi", 3.141592654)
    .add<std::string>("another-string", "test");
}

void throws_exception_trivial_rethrow() {
  try {
    throws_exception_inner();
  } catch (std::exception) {
    throw;
  }
}

void throws_exception_nesting() {
  try {
    throws_exception_inner();
  } catch (...) {
    std::throw_with_nested(ioda::Exception("Caught and encapsulated an exception.", ioda_Here()));
  }
}

int main(int, char**) {
  using namespace std;
  const int requiredPasses = 3;
  int passes               = 0;

  cout << "Single exception test.\n" << endl;
  try {
    throws_exception_inner();
  } catch (const exception& e) {
    ioda::unwind_exception_stack(e, cout);
    passes++;
  }

  cout << "\n\n\nRethrow exception test. Output should be same as above.\n" << endl;
  try {
    throws_exception_trivial_rethrow();
  } catch (const exception& e) {
    ioda::unwind_exception_stack(e, cout);
    passes++;
  }

  cout << "\n\n\nNested exception test. Should return two exceptions.\n" << endl;
  try {
    throws_exception_nesting();
  } catch (const exception& e) {
    ioda::unwind_exception_stack(e, cout);
    passes++;
  }

  return (passes == requiredPasses) ? 0 : 1;
}
