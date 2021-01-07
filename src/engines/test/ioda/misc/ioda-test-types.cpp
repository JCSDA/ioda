/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <array>
#include <iostream>
#include <string>

#include "ioda/Types/Type.h"

int main(int, char**) {
  using namespace std;
  using namespace ioda::detail;

  const char c[] = "This is a test.";  // NOLINT
  array<char, 50> a1;                  // NOLINT
  array<char, 10> a2;                  // NOLINT

  size_t c1 = COMPAT_strncpy_s(a1.data(), a1.size(), c, sizeof(c));

  size_t c2 = COMPAT_strncpy_s(a2.data(), a2.size(), c, a2.size());

  string s1(a1.data());
  string s2(a2.data());

  cout << c1 << "\t" << s1 << endl;
  cout << c2 << "\t" << s2 << endl;

  if (c1 != 15) return 1;
  if (c2 != 9) return 2;

  if (s1 != string("This is a test.")) return 3;
  if (s2 != string("This is a")) return 4;

  return 0;
}
