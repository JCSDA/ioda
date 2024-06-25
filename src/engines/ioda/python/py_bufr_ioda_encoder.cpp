/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>

#include "bufr/DataContainer.h"
#include "bufr/encoders/Description.h"

#include "ioda/Engines/Bufr/Encoder.h"


namespace py = pybind11;

using bufr::DataContainer;
using bufr::encoders::Description;

void setupBufrIodaEncoder(py::module& m)
{
  auto mBufr  = m.def_submodule("Bufr");
  
  py::class_<ioda::Engines::Bufr::Encoder>(mBufr, "Encoder")
    .def(py::init<const std::string&>())
    .def(py::init<const Description&>())
    .def("encode", [](ioda::Engines::Bufr::Encoder& self,
                   const std::shared_ptr<DataContainer>& container) -> std::map<py::tuple, ioda::ObsGroup>
      {
        auto encodedData = self.encode(container);

        // std::vector<std::string> are not hashable in python (can't make a dict), so lets convert
        // it to a tuple instead
        std::map<py::tuple, ioda::ObsGroup> pyEncodedData;
        for (auto& [key, value] : encodedData)
        {
          pyEncodedData[py::cast(key)] = value;
        }

        return pyEncodedData;
      },
      py::arg("container"),
      "Get the class to encode the dataset");
}
