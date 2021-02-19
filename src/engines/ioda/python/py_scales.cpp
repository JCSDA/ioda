/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_scales.cpp
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

void setupNewDimensionScales(pybind11::module& m) {
  // NewDimensionScale will be in a Python namespace because it
  // is too polluting to have it at top-level, as Python lacks
  // class template support.

  // ioda::Unlimited
  m.attr("Unlimited") = py::int_(-1);

  auto mNDS = m.def_submodule("NewDimensionScale");
  mNDS.doc()
    = "Classes and methods for defining dimension "
      "scales in a new ObsSpace";
  py::class_<NewDimensionScale_Base, std::shared_ptr<NewDimensionScale_Base>> nds(mNDS, "Base");
  nds.doc()
    = "Base class for new dimension scales. Do not use "
      "directly. Use a derived class.";
  // No direct initialization. Go through the derived classes.
  nds
    .def(py::init<const std::string, const std::type_index&, Dimensions_t, Dimensions_t,
                  Dimensions_t>())
    .def_readwrite("name", &NewDimensionScale_Base::name_)
    .def_readonly("dataType", &NewDimensionScale_Base::dataType_)
    .def_readwrite("size", &NewDimensionScale_Base::size_)
    .def_readwrite("maxSize", &NewDimensionScale_Base::maxSize_)
    .def_readwrite("chunkingSize", &NewDimensionScale_Base::chunkingSize_)
    .def("writeInitialData", &NewDimensionScale_Base::writeInitialData);

#define InstTmpl(typen, an, cn, typ)                                                               \
  {                                                                                                \
    py::class_<NewDimensionScale<typ>, NewDimensionScale_Base,                                     \
               std::shared_ptr<NewDimensionScale<typ>>>                                            \
      newnds(mNDS, typen);                                                                         \
    newnds.doc() = "New dimension scale of type " typen;                                           \
    newnds                                                                                         \
      .def(py::init<const std::string, Dimensions_t, Dimensions_t, Dimensions_t>(),                \
           py::arg("name"), py::arg("size"), py::arg("maxSize"), py::arg("chunkingSize"))          \
      .def("getShared", &NewDimensionScale<typ>::getShared)                                        \
      .def_readwrite("initdata", &NewDimensionScale<typ>::initdata_);                              \
  }

  CLASS_TEMPLATE_FUNCTION_PATTERN_NOALIASES(NDS, NDS, InstTmpl);
}
