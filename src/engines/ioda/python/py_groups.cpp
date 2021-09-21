/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_groups.cpp
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

void setupGroups(pybind11::module& m) {
  py::class_<Group, std::shared_ptr<Group>> grp(m, "Group");
  grp.doc() = "A group";

  grp.def("list", &Group::list, "The names of all child groups")
    .def("listGroups", &Group::listObjects<ObjectType::Group>, "List the names of all groups",
        py::arg("recurse") = false)
    .def("listVars", &Group::listObjects<ObjectType::Variable>, "List the names of all variables",
        py::arg("recurse") = false)
    //.def("listObjects", static_cast< std::map<ObjectType, std::vector<std::string>>
    //     (Group::*)(ObjectType, bool)>(&Group::listObjects),
    //     "List all child objects (groups, variables), with or without recursion",
    //      py::arg("filter") = ObjectType::Ignored, py::arg("recurse") = false)
    .def("exists", &Group::exists, "Does a group exist with the specified name?", py::arg("name"))
    .def("create", &Group::create, "Create a group", py::arg("name"))
    .def("open", &Group::open, "Open a group", py::arg("name"))
    .def_readwrite("atts", &Group::atts, "Attributes for this group")
    .def_readwrite("vars", &Group::vars, "Variables in this group")
    .def("__repr__",
         [](const Group& g) {
           std::ostringstream out;
           auto names = g.list();
           out << "<ioda.Group at " << &g
               << ". Use list(), atts.list() and vars.list() to see contents.>";

           return out.str();
         })
    .def("__str__", [](const Group& g) {
      std::ostringstream out;
      auto names = g.list();
      out << "<ioda.Group: [ ";
      for (const auto& s : names) out << s << " ";
      out << "]>";

      return out.str();
    });
}
