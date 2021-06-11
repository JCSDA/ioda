/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_dlp.cpp
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

std::shared_ptr<const ioda::detail::DataLayoutPolicy> setupDLP(pybind11::module& mDLP) {
  using namespace ioda::detail;
  py::class_<DataLayoutPolicy, std::shared_ptr<DataLayoutPolicy>> dlp(mDLP, "DataLayoutPolicy");
  dlp.doc() = "Data layout policy for the ObsGroup";
  dlp.def("generate", &DataLayoutPolicy::_py_generate1, "Select policy by string");
  dlp.def("generate", &DataLayoutPolicy::_py_generate2, "Select policy by enum");
  dlp.def("doMap", &DataLayoutPolicy::doMap, "Map variable name to name used in underlying backend",
          py::arg("name"));
  dlp.def("__repr__", [](const DataLayoutPolicy& d) {
    std::ostringstream out;
    out << "<ioda.DLP.DataLayoutPolicy: " << d.name() << ">";
    return out.str();
  });
  dlp.def("__str__", [](const DataLayoutPolicy& d) {
    std::ostringstream out;
    out << "<ioda.DLP.DataLayoutPolicy: " << d.name() << ">";
    return out.str();
  });

  py::enum_<DataLayoutPolicy::Policies> pols(dlp, "Policies");
  // pols.doc() = "Supported policies"; // Can't set
  pols.value("None", DataLayoutPolicy::Policies::None)
    .value("ObsGroup", DataLayoutPolicy::Policies::ObsGroup);

  auto ld = detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroup);
  mDLP.attr("default") = ld;
  // mDLP.attr("default") = py::int_(-1);
  return ld;
}
