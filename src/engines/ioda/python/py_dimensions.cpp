/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_dimensions.cpp
/// \brief Python bindings - Dimensions

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <iostream>
#include <sstream>

#include "./macros.h"
#include "ioda/Group.h"

namespace py = pybind11;
using namespace ioda;

void setupDimensions(pybind11::module& m) {
  py::class_<Dimensions> dims(m, "Dimensions");
  dims.doc() = "Dimensions of an attribute or variable";
  dims.def_readwrite("dimensionality", &Dimensions::dimensionality, "The dimensionality")
    .def_readwrite("numElements", &Dimensions::numElements, "The number of elements")
    .def_readwrite("dimsCur", &Dimensions::dimsCur, "The current dimensions")
    .def_readwrite("dimsMax", &Dimensions::dimsMax, "The maximum possible dimensions")
    .def("__repr__",
         [](const Dimensions& dims) {
           std::ostringstream out;
           out << "<ioda.Dimensions object:\n"
               << "\tDimensionality: " << dims.dimensionality << "\n"
               << "\tNumber of elements: " << dims.numElements << "\n"
               << "\tCurrent dimensions: ";
           for (size_t i = 0; i < dims.dimsCur.size(); ++i) {
             if (i) out << " x ";
             out << dims.dimsCur[i];
           }
           out << "\n\tMax dimensions: ";
           for (size_t i = 0; i < dims.dimsMax.size(); ++i) {
             if (i) out << " x ";
             out << dims.dimsMax[i];
           }
           out << "\n\t>";

           return out.str();
         })
    .def("__str__", [](const Dimensions& dims) {
      std::ostringstream out;
      out << "<ioda.Dimensions object with current dimensions ";
      if (dims.dimensionality) {
        for (size_t i = 0; i < gsl::narrow<size_t>(dims.dimensionality); ++i) {
          if (i) out << " x ";
          out << dims.dimsCur[i];
        }
      } else
        out << "null";
      out << " >";
      return out.str();
    });
}
