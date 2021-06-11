/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_has_attributes.cpp
/// \brief Python bindings - Has_Attributes

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <sstream>

#include "./macros.h"
#include "ioda/Group.h"

namespace py = pybind11;
using namespace ioda;

void setupHasAttributes(pybind11::module& m) {
  py::class_<Has_Attributes> hatt(m, "Has_Attributes");
  hatt.doc() = "Container for this object's attributes";
  hatt.def("list", &Has_Attributes::list, "The names of all attributes")
    .def("exists", &Has_Attributes::exists, "Does an attribute exist with the specified name?",
         py::arg("name"))
    .def("remove", &Has_Attributes::remove, "Remove an attribute", py::arg("name"))
    .def("rename", &Has_Attributes::rename, "Rename an attribute", py::arg("oldname"),
         py::arg("newname"))
    .def("open", &Has_Attributes::open, "Open an attribute", py::arg("name"))
    .def("create", &Has_Attributes::_create_py, "Create an attribute", py::arg("name"),
         py::arg("dtype"), py::arg("dims") = std::vector<Dimensions_t>{1})
    // CLASS_TEMPLATE_FUNCTION_PATTERN(create, create, Has_Attributes,
    // CREATE_CLASS_TEMPLATE_FUNCTION_T) Convenience functions, add and read
    //.def("read", &Has_Attributes::read)
    .def("__repr__",
         [](const Has_Attributes& ha) {
           std::ostringstream out;
           auto names = ha.list();
           out << "<ioda.Has_Attributes: [ ";
           for (const auto& s : names) out << s << " ";
           out << "]>";

           return out.str();
         })
    .def("__str__",
         [](const Has_Attributes& ha) {
           std::ostringstream out;
           auto names = ha.list();
           out << "<ioda.Has_Attributes: [ ";
           for (const auto& s : names) out << s << " ";
           out << "]>";

           return out.str();
         })

    ;
}

/// \todo AttributeCreator store needs python bindings.
/// Either add in the necessary inheritance structures or clone the function bindings.
void setupAttCreator(pybind11::module& m) {
  py::class_<Attribute_Creator_Store> acs(m, "Attribute_Creator_Store");
  acs.doc() = "Parameters involved in creating new attributes";
}
