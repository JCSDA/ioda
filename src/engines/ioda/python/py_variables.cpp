/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_variables.cpp
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

void setupVariables(pybind11::module& m, pybind11::module& mDetail, pybind11::module& mPy) {
  using namespace ioda::detail;

  auto mVar = mPy.def_submodule("Variables");
  mVar.doc() = "Variable binding helper classes";

  py::class_<python_bindings::VariableIsA<Variable>> is(mVar, "isA");
  is.doc() = "Is the data the specified type?";
  is CLASS_TEMPLATE_FUNCTION_PATTERN(isA, python_bindings::VariableIsA<Variable>,
                                     ISA_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::VariableReadVector<Variable>> rv(mVar, "readVector");
  rv.doc() = "Read data as a 1-D vector";
  rv CLASS_TEMPLATE_FUNCTION_PATTERN(read, python_bindings::VariableReadVector<Variable>,
                                     READ_VAR_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::VariableReadNPArray<Variable>> rNPA(mVar, "readNPArray");
  rNPA.doc() = "Read data as a numpy array";
  rNPA CLASS_TEMPLATE_FUNCTION_PATTERN_NOSTR(read, python_bindings::VariableReadNPArray<Variable>,
                                             READ_VAR_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::VariableWriteVector<Variable>> wv(mVar, "writeVector");
  wv.doc() = "Write data as a 1-D vector";
  wv CLASS_TEMPLATE_FUNCTION_PATTERN(write, python_bindings::VariableWriteVector<Variable>,
                                     WRITE_VAR_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::VariableWriteNPArray<Variable>> wNPA(mVar, "writeNPArray");
  wNPA.doc() = "Write data as a numpy array";
  wNPA CLASS_TEMPLATE_FUNCTION_PATTERN_NOSTR(write, python_bindings::VariableWriteNPArray<Variable>,
                                             WRITE_VAR_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::VariableScales<Variable>> scales(mVar, "scales");
  scales.doc() = "Dimension scales";
  scales
    .def("attach", &python_bindings::VariableScales<Variable>::attach,
         "Attach a dimension scale to a variable", py::arg("DimensionNumber"), py::arg("scale"))
    .def("detach", &python_bindings::VariableScales<Variable>::detach, "Detach a dimension scale",
         py::arg("DimensionNumber"), py::arg("scale"))
    .def("set", &python_bindings::VariableScales<Variable>::set, "Set dimension scales", py::arg("scales"))
    .def("isScale", &python_bindings::VariableScales<Variable>::isScale,
         "Is this variable a dimension scale?")
    .def("setIsScale", &python_bindings::VariableScales<Variable>::setIsScale,
         "Make this variable a dimension scale", py::arg("scale_name"))
    .def("getScaleName", &python_bindings::VariableScales<Variable>::getScaleName,
         "Get the name of this dimension scale")
    .def("isDimensionScaleAttached", &python_bindings::VariableScales<Variable>::isAttached,
         "Is a certain scale attached along the specified axis?", py::arg("DimensionNumber"),
         py::arg("scale"));

  py::class_<Variable>(m, "Variable")
    .def_readwrite("atts", &Variable::atts)
    .def_property_readonly("dims", &Variable::getDimensions, "The current dimensions of the variable")
    .def_readwrite("isA", &Variable::_py_isA, "Query the data type")
    .def("isA2", &Variable::_py_isA2, "Query the data type", py::arg("dtype"))
    .def_readwrite("scales", &Variable::_py_scales, "Manipulate this variable's dimension scales")
    .def_readwrite("readVector", &Variable::_py_readVector, "Read data as a 1-D vector")
    .def_readwrite("readNPArray", &Variable::_py_readNPArray, "Read data as a numpy array")
    .def_readwrite("writeVector", &Variable::_py_writeVector, "Write data as a 1-D vector")
    .def_readwrite("writeNPArray", &Variable::_py_writeNPArray, "Write data as a numpy array")
    .def("resize", &Variable::resize, "Resize a variable", py::arg("newdims"));
}
