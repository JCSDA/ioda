/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_attributes.cpp
/// \brief Python bindings - Attributes

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <sstream>

#include "./macros.h"
#include "ioda/Group.h"

namespace py = pybind11;
using namespace ioda;

/// \todo Finish!
void setupSelections(pybind11::module& m) {
  /*
  py::enum_<SelectionOperator>(m, "Selection Operator")
          .value("SET", SelectionOperator::SET)
          .value("OR", SelectionOperator::OR)
          .value("AND", SelectionOperator::AND)
          .value("XOR", SelectionOperator::XOR)
          .value("NOT_B", SelectionOperator::NOT_B)
          .value("NOT_A", SelectionOperator::NOT_A)
          .value("APPEND", SelectionOperator::APPEND)
          .value("PREPEND", SelectionOperator::PREPEND)
          ;
  py::enum_<SelectionState>(m, "Selection State")
          .value("ALL", SelectionState::ALL)
          .value("NONE", SelectionState::NONE)
          ;
  py::class_<Selection::SingleSelection>(m, "SingleSelection")
          .def_readwrite("op_", &Selection::SingleSelection::op_)
          .def_readwrite("start_", &Selection::SingleSelection::start_)
          .def_readwrite("count_", &Selection::SingleSelection::count_)
          .def_readwrite("stride_", &Selection::SingleSelection::stride_)
          .def_readwrite("block_", &Selection::SingleSelection::block_)
          .def_readwrite("points_", &Selection::SingleSelection::points_)
          ;
          */
  py::class_<Selection> sel(m, "Selection");
  sel.doc() = "Selections of data in variables";
  sel
    /*
    .def_readwrite("default_", &Selection::default_)
    .def_readwrite("actions_", &Selection::actions_)
    .def_readwrite("offset_", &Selection::offset_)
    .def_readwrite("extent", &Selection::extent_)
    */
    .def_readonly_static("all", &Selection::all)
    .def_readonly_static("none", &Selection::none);
}
