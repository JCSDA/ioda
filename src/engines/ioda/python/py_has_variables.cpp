/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_has_variables.cpp
/// \brief Python bindings for the ioda / ioda-engines library.

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <sstream>

#include "./macros.h"
#include "ioda/Engines/HH.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/Group.h"
#include "ioda/Layout.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/ObsGroup.h"

namespace py = pybind11;
using namespace ioda;

void setupHasVariables(pybind11::module& m) {
  py::class_<Has_Variables> hvar(m, "Has_Variables");
  hvar.doc() = "Container for this group's variables";
  hvar
    .def("exists", &Has_Variables::exists, "Does a variable exist with the specified name?",
         py::arg("name"))
    .def("remove", &Has_Variables::remove, "Remove a variable", py::arg("name"))
    .def("open", &Has_Variables::open, "Open a variable", py::arg("name"))
    .def("list", &Has_Variables::list, "The names of all variables")
    .def("getTypeProvider",
         &Has_Variables::getTypeProvider,
         py::return_value_policy::reference_internal,
         "Get an interface for creating new data types for this backend")
    .def("create", &Has_Variables::_create_py, "Create a variable", py::arg("name"),
         py::arg("dtype"),
         py::arg("dimsCur") = std::vector<Dimensions_t>{1},
         py::arg("dimsMax") = std::vector<Dimensions_t>{},
         py::arg("scales")  = std::vector<Variable>{},
         py::arg("params")  = VariableCreationParameters()
         )
    .def("create",
         static_cast<Variable (Has_Variables::*)(const std::string&, const Type&, const std::vector<Dimensions_t>&, const std::vector<Dimensions_t>&, const VariableCreationParameters&)>(&Has_Variables::create),
         "Create a variable",
         py::arg("name"),
         py::arg("dtype"),
         py::arg("dimsCur") = std::vector<Dimensions_t>{1},
         py::arg("dimsMax") = std::vector<Dimensions_t>{},
         py::arg("params")  = VariableCreationParameters()
         )
    .def("__repr__",
         [](const Has_Variables& g) {
           std::ostringstream out;
           auto names = g.list();
           out << "<ioda.Has_Variables: [ ";
           for (const auto& s : names) out << s << " ";
           out << "]>";

           return out.str();
         })
    .def("__str__", [](const Has_Variables& g) {
      std::ostringstream out;
      auto names = g.list();
      out << "<ioda.Has_Variables: [ ";
      for (const auto& s : names) out << s << " ";
      out << "]>";

      return out.str();
    });
}
