/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_vcps.cpp
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

///  \todo AttributeCreatorStore needs python bindings
void setupVCPs(pybind11::module& m, pybind11::module& mDetail, pybind11::module& mPy) {
  using namespace ioda::detail;

  auto mVCP  = mPy.def_submodule("VariableCreationParameters");
  mVCP.doc() = "VariableCreationParameters binding helper classes";

  py::class_<python_bindings::VariableCreationFillValues<VariableCreationParameters>> sf(mVCP,
                                                                                         "setFill");
  sf.doc() = "Set the fill value";
  sf CLASS_TEMPLATE_FUNCTION_PATTERN(
    setFillValue, python_bindings::VariableCreationFillValues<VariableCreationParameters>,
    SETFILL_CLASS_TEMPLATE_FUNCTION_T);

  py::class_<VariableCreationParameters> vcps(m, "VariableCreationParameters");
  vcps.doc() = "Additional parameters that can be set at variable creation time";
  vcps.def(py::init<>())
    .def_readwrite("chunk", &VariableCreationParameters::chunk, "Use chunking")
    .def_readwrite("chunks", &VariableCreationParameters::chunks, "Chunk sizes")
    .def("noCompress", &VariableCreationParameters::noCompress, "Do not compress")
    .def("compressWithGZIP", &VariableCreationParameters::compressWithGZIP, "Use GZIP compression",
         py::arg("level") = 6)
    .def("compressWithSZIP", &VariableCreationParameters::compressWithSZIP,
         "Use SZIP compression (see H5_SZIP_EC_OPTION_MASK in hdf5.h)",
         py::arg("PixelsPerBlock") = 16, py::arg("options") = 4)
    .def_readwrite("setFillValue", &VariableCreationParameters::_py_setFillValue, "Set fill value")
    .def_readwrite("atts", &VariableCreationParameters::atts, "Attributes");
}
