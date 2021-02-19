/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_types.cpp
/// \brief Python bindings - Type system

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <sstream>

#include "./macros.h"
#include "ioda/Group.h"

namespace py = pybind11;
using namespace ioda;

void setupTypeSystem(pybind11::module& m) {
  py::enum_<BasicTypes> eTypes(m, "Types");
  // Surprisingly, can't set a docstring on an enum.
  // eTypes.doc() = "Basic types available in the C++/Python interface";
  eTypes
    .value("float", BasicTypes::float_,
           "Platform-defined single precision float: typically sign bit, 8 bits exponent, 23 bits "
           "mantissa")
    .value("double", BasicTypes::double_,
           "Platform-defined double precision float: typically sign bit, 11 bits exponent, 52 bits "
           "mantissa")
    .value("ldouble", BasicTypes::ldouble_, "Platform-defined extended-precision float")
    .value("char", BasicTypes::char_, "C character type")
    .value("short", BasicTypes::short_, "C short integer type (platform-defined)")
    .value("ushort", BasicTypes::ushort_, "C unsigned short integer type (platform-defined)")
    .value("int", BasicTypes::int_, "C integer type (platform-defined)")
    .value("uint", BasicTypes::uint_, "C unsigned integer type (platform-defined)")
    .value("lint", BasicTypes::lint_, "C long integer type (platform-defined)")
    .value("ulint", BasicTypes::ulint_, "C unsigned long integer type (platform-defined)")
    .value("llint", BasicTypes::llint_, "C long long type (platform-defined)")
    .value("ullint", BasicTypes::ullint_, "C unsigned long long type (platform-defined)")
    .value("int16", BasicTypes::int16_, "Integer (-32768 to 32767)")
    .value("uint16", BasicTypes::uint16_, "Unsigned integer (0 to 65535)")
    .value("int32", BasicTypes::int32_, "Integer (-2147483648 to 2147483647)")
    .value("uint32", BasicTypes::uint32_, "Unsigned integer (0 to 4294967295)")
    .value("int64", BasicTypes::int64_, "Integer (-9223372036854775808 to 9223372036854775807)")
    .value("uint64", BasicTypes::uint64_, "Unsigned integer (0 to 18446744073709551615)")
    .value("bool", BasicTypes::bool_, "Boolean (True or False) stored as a byte")
    .value("str", BasicTypes::str_, "Variable-length UTF-8 string");
}
