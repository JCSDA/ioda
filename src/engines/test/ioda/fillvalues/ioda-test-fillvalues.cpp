/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <iostream>
#include <vector>

#include "ioda/Engines/Factory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

template <typename T>
bool test_equal(const T& a, const T& b) {
  return a == b;
}

template <>
bool test_equal(const float& a, const float& b) {
  return std::abs(a - b) < 0.00001f;
}

template <>
bool test_equal(const double& a, const double& b) {
  return std::abs(a - b) < 0.00001f;
}

template <>
bool test_equal(const long double& a, const long double& b) {
  return std::abs(a - b) < 0.00001f;
}

template <class T>
bool testVarFill(ioda::Group& f, const std::string& varname,
                 const T& filldata)  // NOLINT(google-runtime-references)
{
  try {
    std::cout << "Testing variable " << varname << std::endl;
    std::cout << "\tCreating..." << std::endl;
    using namespace ioda;
    VariableCreationParameters params;
    params.setFillValue(filldata);
    f.vars.create<T>(varname, {1}, {1}, params);
    std::cout << "\t\tSuccess.\n\tReading..." << std::endl;

    std::vector<T> checkdata;
    f.vars[varname].read(checkdata);
    if (checkdata.empty()) {
      std::cout << "\t\tFailed to read." << std::endl;
      return false;
    }
    std::cout << "\t\tSuccess." << std::endl;
    std::cout << "\tChecking read value with reference..." << std::endl;
    if (!test_equal<T>(filldata, checkdata.at(0))) {
      std::cout << "\t\tFailed check. Ref is '" << filldata << "' and data is '" << checkdata.at(0) << "'."
                << std::endl;
      return false;
    }
    std::cout << "\t\tSuccess." << std::endl;

    std::cout << "\tCheck that the variable has a fill value." << std::endl;
    if (f.vars[varname].hasFillValue()) {
      std::cout << "\t\tSuccess." << std::endl;
    } else {
      std::cout << "\t\tFailed." << std::endl;
      return false;
    }

    std::cout << "\tCheck fill value read." << std::endl;
    auto fv = f.vars[varname].getFillValue();
    if (!fv.set_) {
      std::cout << "\t\tfv.set_ is false. Failed." << std::endl;
      return false;
    }

    if (!test_equal<T>(filldata, ioda::detail::getFillValue<T>(fv))) {
      std::cout << "\t\tFailure. Ref is " << filldata << " and fv is " << ioda::detail::getFillValue<T>(fv)
                << "." << std::endl;
      return false;
    }

    std::cout << "\t\tSuccess." << std::endl;

    return true;
  } catch (...) {
    std::cout << "\t\tFailed with exception." << std::endl;
    return false;
  }
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-fills.hdf5");

    // These tests try to read and write fill values.
    // We try basic numbers, fixed-length strings and variable-length
    // strings.
    int good = 0, bad = 0;

    (testVarFill<std::string>(f, "varlen-string-test", "This is a test")) ? good++ : bad++;
    (testVarFill<std::string>(f, "varlen-empty_string-test", "")) ? good++ : bad++;

    (testVarFill<int>(f, "int-test", -999)) ? good++ : bad++;
    (testVarFill<int32_t>(f, "int32_t-test", -99)) ? good++ : bad++;
    (testVarFill<uint32_t>(f, "uint32_t-test", 99)) ? good++ : bad++;
    (testVarFill<int16_t>(f, "int16_t-test", -99)) ? good++ : bad++;
    (testVarFill<uint16_t>(f, "uint16_t-test", 99)) ? good++ : bad++;
    (testVarFill<int64_t>(f, "int64_t-test", -99)) ? good++ : bad++;
    (testVarFill<uint64_t>(f, "uint64_t-test", 99)) ? good++ : bad++;
    (testVarFill<char>(f, "char-test", 'a')) ? good++ : bad++;
    (testVarFill<float>(f, "float-test", 3.14f)) ? good++ : bad++;
    (testVarFill<double>(f, "double-test", 2.7)) ? good++ : bad++;
    (testVarFill<long double>(f, "long-double-test", 1.428571428571429L)) ? good++ : bad++;

    // TODO(rhoneyager): Fixed-length string test.

    std::cout << "\n\nSuccesses: " << good << "\nFailures: " << bad << std::endl;
    return (bad) ? 1 : 0;
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e, cout);
    return 1;
  }
}
