/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_ioda.cpp
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

void setupTypeSystem(pybind11::module& m);

void setupDimensions(pybind11::module& m);

void setupAttributes(pybind11::module& m, pybind11::module& mDetail, pybind11::module& mPy);

void setupHasAttributes(pybind11::module& m);

void setupAttCreator(pybind11::module& m);

void setupSelections(pybind11::module& m);

void setupVariables(pybind11::module& m, pybind11::module& mDetail, pybind11::module& mPy);

void setupVCPs(pybind11::module& m, pybind11::module& mDetail, pybind11::module& mPy);

void setupHasVariables(pybind11::module& m);

void setupGroups(pybind11::module& m);

void setupEngines(pybind11::module& m);

std::shared_ptr<const ioda::detail::DataLayoutPolicy> setupDLP(pybind11::module& mDLP);

void setupNewDimensionScales(pybind11::module& m);

void setupObsGroup(pybind11::module& m,
                   std::shared_ptr<const ioda::detail::DataLayoutPolicy> default_dlp);

using namespace ioda;
PYBIND11_MODULE(_ioda_python, m) {
  namespace py = pybind11;

  m.doc() = "Python bindings for ioda";

  auto mDetail  = m.def_submodule("detail");
  mDetail.doc() = "Implementation details";

  auto mDLP  = m.def_submodule("DLP");
  mDLP.doc() = "Data layout policies";

  auto mPy  = mDetail.def_submodule("python_bindings");
  mPy.doc() = "Python binding helper classes";

  setupTypeSystem(m);
  setupDimensions(m);
  setupAttributes(m, mDetail, mPy);
  setupHasAttributes(m);
  setupAttCreator(m);
  setupSelections(m);
  setupVariables(m, mDetail, mPy);
  setupVCPs(m, mDetail, mPy);
  setupHasVariables(m);
  setupGroups(m);
  setupEngines(m);
  auto default_dlp = setupDLP(mDLP);
  setupNewDimensionScales(m);
  setupObsGroup(m, default_dlp);
}
