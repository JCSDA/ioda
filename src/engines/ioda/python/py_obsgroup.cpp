/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_obsgroup.cpp
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

void setupObsGroup(pybind11::module& m,
                   std::shared_ptr<const ioda::detail::DataLayoutPolicy> default_dlp) {
  py::class_<ObsGroup, Group, std::shared_ptr<ObsGroup>> obs(m, "ObsGroup");

  obs.doc() = "The main class for manipulating data";
  obs
    .def(py::init<Group, std::shared_ptr<const detail::DataLayoutPolicy>>(), py::arg("group"),
         py::arg("layout") = nullptr)
    .def(py::init())
    .def_static("generate", &ObsGroup::generate, "Create a new ObsGroup", py::arg("group"),
                py::arg("fundamentalDims"), py::arg("layout") = default_dlp)
    .def("resize", &ObsGroup::resize, "Resize this ObsGroup", py::arg("newSizes"));
}
