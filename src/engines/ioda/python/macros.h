#pragma once
/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file macros.h
/// \brief Python bindings - macros

#include <chrono>
#include <string>
#include <vector>

#include <pybind11/chrono.h>
#include <pybind11/stl.h>

#ifdef _MSC_FULL_VER
#pragma warning(disable : 4267)  // Python interface
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

#define TYPE_TEMPLATE(x)   \
  x<std::string>();        \
  x<float>();              \
  x<double>();             \
  x<long double>();        \
  x<char>();               \
  x<unsigned char>();      \
  x<signed char>();        \
  x<wchar_t>();            \
  x<char16_t>();           \
  x<char32_t>();           \
  x<bool>();               \
  x<short int>();          \
  x<unsigned short int>(); \
  x<int>();                \
  x<unsigned int>();       \
  x<long int>();           \
  x<unsigned long int>();  \
  x<long long int>();      \
  x<unsigned long long int>();

#define SETFILL_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T) \
  .def(funcnamestr, &classname ::funcname<T>)

#define VARCREATE_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T)                       \
  .def(funcnamestr,                                                                                    \
       (Variable(classname::*)(const std::string&, const std::vector<Dimensions_t>&,                   \
                               const std::vector<Dimensions_t>&, const VariableCreationParameters&)) & \
         classname ::funcname<T>,                                                                      \
       "Create a variable", py::arg("name"), py::arg("dimensions") = std::vector<Dimensions_t>{1},     \
       py::arg("max_dimensions") = std::vector<Dimensions_t>{},                                        \
       py::arg("params") = VariableCreationParameters())

#define VARCREATESCALES_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T) \
  .def(funcnamestr,                                                                    \
       (Variable(classname::*)(const std::string&, const std::vector<Variable>&,       \
                               const VariableCreationParameters&)) &                   \
         classname ::funcname<T>,                                                      \
       "Create a variable", py::arg("name"), py::arg("dimension_scales"),              \
       py::arg("params") = VariableCreationParameters())

#define ISA_ATT_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T) \
  .def(funcnamestr, &classname ::funcname<T>, "Is " #T " the storage type of the data?")

#define READ_ATT_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T) \
  .def(funcnamestr, &classname ::funcname<T>, "Read as type " #T)

#define WRITE_ATT_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T) \
  .def(funcnamestr, &classname ::funcname<T>, "Write as type " #T)

#define READ_VAR_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T)                              \
  .def(funcnamestr, &classname ::funcname<T>, "Read as type " #T, py::arg("mem_selection") = Selection::all, \
       py::arg("file_selection") = Selection::all)

#define WRITE_VAR_CLASS_TEMPLATE_FUNCTION_T(funcnamestr, funcname, classname, T)    \
  .def(funcnamestr, &classname ::funcname<T>, "Write as type " #T, py::arg("vals"), \
       py::arg("mem_selection") = Selection::all, py::arg("file_selection") = Selection::all)

#define CLASS_TEMPLATE_FUNCTION_PATTERN_NOALIASES(actualname, classname, PATTERN) \
  PATTERN("float", actualname, classname, float)                                  \
  PATTERN("double", actualname, classname, double)                                \
  PATTERN("long_double", actualname, classname, long double)                      \
  PATTERN("int32", actualname, classname, int32_t)                                \
  PATTERN("uint32", actualname, classname, uint32_t)                              \
  PATTERN("int16", actualname, classname, int16_t)                                \
  PATTERN("uint16", actualname, classname, uint16_t)                              \
  PATTERN("int64", actualname, classname, int64_t)                                \
  PATTERN("uint64", actualname, classname, uint64_t)
/*
// bool has some problems
PATTERN("bool", funcname, classname, bool)   \
*/

#define CLASS_TEMPLATE_FUNCTION_PATTERN_NOSTR(actualname, classname, PATTERN) \
  CLASS_TEMPLATE_FUNCTION_PATTERN_NOALIASES(actualname, classname, PATTERN)   \
  PATTERN("short", actualname, classname, short int)                          \
  PATTERN("ushort", actualname, classname, unsigned short int)                \
  PATTERN("int", actualname, classname, int)                                  \
  PATTERN("uint", actualname, classname, unsigned int)                        \
  PATTERN("lint", actualname, classname, long int)                            \
  PATTERN("ulint", actualname, classname, unsigned long int)                  \
  PATTERN("llint", actualname, classname, long long int)                      \
  PATTERN("ullint", actualname, classname, unsigned long long int)            \
  PATTERN("datetime", actualname, classname, std::chrono::time_point<std::chrono::system_clock>)

#define CLASS_TEMPLATE_FUNCTION_PATTERN(actualname, classname, PATTERN) \
  PATTERN("str", actualname, classname, std::string)                    \
  PATTERN("char", actualname, classname, char)                          \
  CLASS_TEMPLATE_FUNCTION_PATTERN_NOSTR(actualname, classname, PATTERN)
