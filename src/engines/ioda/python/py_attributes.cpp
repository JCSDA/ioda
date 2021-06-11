/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_attributes.cpp
/// \brief Python bindings - Attributes

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <sstream>

#include "./macros.h"
#include "ioda/Group.h"

namespace py = pybind11;
using namespace ioda;

void setupAttributes(pybind11::module& m, pybind11::module& mDetail, pybind11::module& mPy) {
  using namespace ioda::detail;

  auto mAtt  = mPy.def_submodule("Attributes");
  mAtt.doc() = "Attribute binding helper classes";

  py::class_<python_bindings::AttributeIsA<Attribute>> is(mAtt, "isA");
  is.doc() = "Is the data the specified type?";
  is CLASS_TEMPLATE_FUNCTION_PATTERN(isA, python_bindings::AttributeIsA<Attribute>,
                                     ISA_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::AttributeReadSingle<Attribute>> rd(mAtt, "readDatum");
  rd.doc() = "Read a single value (a datum)";
  rd CLASS_TEMPLATE_FUNCTION_PATTERN(read, python_bindings::AttributeReadSingle<Attribute>,
                                     READ_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::AttributeReadVector<Attribute>> rv(mAtt, "readVector");
  rv.doc() = "Read data as a 1-D vector";
  rv CLASS_TEMPLATE_FUNCTION_PATTERN(read, python_bindings::AttributeReadVector<Attribute>,
                                     READ_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::AttributeReadNPArray<Attribute>> rNPA(mAtt, "readNPArray");
  rNPA.doc() = "Read data as a numpy array";
  rNPA CLASS_TEMPLATE_FUNCTION_PATTERN_NOSTR(read, python_bindings::AttributeReadNPArray<Attribute>,
                                             READ_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::AttributeWriteSingle<Attribute>> wd(mAtt, "writeDatum");
  wd.doc() = "Write a single value (a datum)";
  wd CLASS_TEMPLATE_FUNCTION_PATTERN(write, python_bindings::AttributeWriteSingle<Attribute>,
                                     WRITE_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::AttributeWriteVector<Attribute>> wv(mAtt, "writeVector");
  wv.doc() = "Write data as a 1-D vector";
  wv CLASS_TEMPLATE_FUNCTION_PATTERN(write, python_bindings::AttributeWriteVector<Attribute>,
                                     WRITE_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<python_bindings::AttributeWriteNPArray<Attribute>> wNPA(mAtt, "writeNPArray");
  wNPA.doc() = "Write data as a numpy array";
  wNPA CLASS_TEMPLATE_FUNCTION_PATTERN_NOSTR(
    write, python_bindings::AttributeWriteNPArray<Attribute>, WRITE_ATT_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<Attribute> att(m, "Attribute");
  att.doc() = "A small tag on a variable or group that describes how to interpret data.";
  att.def("isA2", &Attribute::_py_isA2, "Query the data type", py::arg("dtype"))
    .def_readwrite("isA", &Attribute::_py_isA, "Query the data type")
    .def_property_readonly("dims", &Attribute::getDimensions, "The dimensions of the attribute")
    .def_readwrite("readDatum", &Attribute::_py_readSingle, "Read a single value (a datum)")
    .def_readwrite("readVector", &Attribute::_py_readVector, "Read data as a 1-D vector")
    .def_readwrite("readNPArray", &Attribute::_py_readNPArray, "Read data as a numpy array")
    .def_readwrite("writeDatum", &Attribute::_py_writeSingle, "Write a single value (a datum)")
    .def_readwrite("writeVector", &Attribute::_py_writeVector, "Write data as a 1-D vector")
    .def_readwrite("writeNPArray", &Attribute::_py_writeNPArray, "Write data as a numpy array");
}
