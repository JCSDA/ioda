/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file py_engines.cpp
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
#include "ioda/config.h"

namespace py = pybind11;
using namespace ioda;


#if bufr_query_FOUND
void setupBufrIodaEncoder(py::module& m);
#endif


void setupEngines(pybind11::module& m) {
  using namespace ioda::Engines;
  auto mEngines  = m.def_submodule("Engines");
  mEngines.doc() = "Backend endinges that power Groups, Variables and Attributes";

  py::enum_<BackendCreateModes> bcm(mEngines, "BackendCreateModes");
  // bcm.doc() = "File creation modes"; // Can't set
  bcm.value("Truncate_If_Exists", BackendCreateModes::Truncate_If_Exists)
    .value("Fail_If_Exists", BackendCreateModes::Fail_If_Exists);

  py::enum_<BackendOpenModes> bco(mEngines, "BackendOpenModes");
  // bco.doc() = "File opening modes"; // Can't set
  bco.value("Read_Only", BackendOpenModes::Read_Only)
    .value("Read_Write", BackendOpenModes::Read_Write);

  // Engine creators go here
  auto mEnginesHH  = mEngines.def_submodule("HH");
  mEnginesHH.doc() = "HDF5 engines (powered by HDFforHumans)";

  py::enum_<ioda::Engines::HH::HDF5_Version> hver(mEnginesHH, "HDF5_Version");
  hver.value("Earliest", ioda::Engines::HH::HDF5_Version::Earliest)
    .value("V18", ioda::Engines::HH::HDF5_Version::V18)
    .value("V110", ioda::Engines::HH::HDF5_Version::V110)
    .value("V112", ioda::Engines::HH::HDF5_Version::V112)
    .value("Latest", ioda::Engines::HH::HDF5_Version::Latest);

  mEnginesHH.def("genUniqueName", ioda::Engines::HH::genUniqueName, "Generates a unique id.");
  mEnginesHH.def(
    "createFile", ioda::Engines::HH::createFile, "Creates a new HDF5 file.", py::arg("name"),
    py::arg("mode"),
    py::arg("compat_range") = ioda::Engines::HH::defaultVersionRange());
  mEnginesHH.def(
    "openFile", ioda::Engines::HH::openFile, "Opens an existing HDF5 file.", py::arg("name"),
    py::arg("mode"),
    py::arg("compat_range") = ioda::Engines::HH::defaultVersionRange());
  mEnginesHH.def(
    "createMemoryFile", ioda::Engines::HH::createMemoryFile, "Creates a ioda file in memory.",
    py::arg("name") = "",
    py::arg("mode") = ioda::Engines::BackendCreateModes::Truncate_If_Exists,
    py::arg("flush_on_close") = false,
    py::arg("increment_len_bytes") = 1000000,
    py::arg("compat_range") = ioda::Engines::HH::defaultVersionRange());

  auto mEnginesObsStore  = mEngines.def_submodule("ObsStore");
  mEnginesObsStore.doc() = "Default in-memory engine. MPI capable.";
  mEnginesObsStore.def("createRootGroup", ioda::Engines::ObsStore::createRootGroup,
                       "Create a new ObsStore-backed group.");

#if bufr_query_FOUND
  setupBufrIodaEncoder(mEngines);
#endif
}
