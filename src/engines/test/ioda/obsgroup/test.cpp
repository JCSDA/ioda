/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cstdlib>
#include <iostream>
#include <numeric>

#include "Eigen/Dense"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/ObsGroup.h"
#include "ioda/defs.h"
#include "unsupported/Eigen/CXX11/Tensor"

int main(int argc, char** argv) {
  try {
    using namespace ioda;
    auto f = Engines::constructFromCmdLine(argc, argv, "test-obsgroup1.hdf5");

    // ATMS profiles. Generally 11 scan lines per granule,
    // 96 scan positions per line. CRTM retrieves along
    // 101 pressure levels, so 100 pressure layers.
    // Time is expressed as a unit of scan line, so it is not a
    // fundamental dimension here.

    // We create the base dimensions along with the ObsGroup.
    // Adding attributes to the dimensions is done later.
    const int atms_scanpos = 96;
    const int atms_numchannels = 22;
    const int crtm_numlevels = 101;
    const int crtm_numlayers = 100;
    const int atms_initlines = 66;  // Scan lines can vary, but for this example we write 11.
    // This function makes a brand new ObsGroup with pre-allocated dimensions.
    // For each dimension, we provide a name, whether it is Horizontal, Vertical, etc. (
    // for Locations and GeoVaLs stuff), an initial size, a maximum size, and
    // a "hint" for setting chunking properties of derived variables.

    auto og = ObsGroup::generate(
      f, {NewDimensionScale<int>("ScanPosition", atms_scanpos, atms_scanpos, atms_scanpos),
          NewDimensionScale<int>("ScanLine", atms_initlines, -1, 11),
          NewDimensionScale<int>("Level", crtm_numlevels, crtm_numlevels, crtm_numlevels),
          NewDimensionScale<int>("Layer", crtm_numlayers, crtm_numlayers, crtm_numlayers),
          NewDimensionScale<int>("Channel", atms_numchannels, atms_numchannels,
                                                   atms_numchannels)});

    // We want to use variable chunking and turn on GZIP compression.
    ioda::VariableCreationParameters params;
    params.chunk = true;
    params.compressWithGZIP();

    // We want to set a default fill value.
    // TODO(rhoneyager): See if we always want to set this to a sensible default.
    auto params_double = params;
    params_double.setFillValue<double>(-999);
    auto params_float = params;
    params_float.setFillValue<float>(-999);
    auto params_int = params;
    params_int.setFillValue<int>(-999);

    /*
    // TODO: Make this automatic.

    // Let's deliberately fill in the scan position, scan line and channel numbers so
    /// that Panoply plots look nice.
    Eigen::ArrayXi eScanPos(atms_scanpos);
    eScanPos.setLinSpaced(1, atms_scanpos);
    og.all_dims["ScanPosition"].writeWithEigenRegular(eScanPos);

    Eigen::ArrayXi eScanLine(atms_initlines);
    eScanLine.setLinSpaced(1, atms_initlines);
    og.all_dims["ScanLine"].writeWithEigenRegular(eScanLine);

    std::array<int, atms_numchannels> vChannel{ 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    17,18,19,20,21,22 };
    og.all_dims["Channel"].write<int>(vChannel);
    */

    // Writing in the data

    std::array<std::string, atms_numchannels> vCentrFreq{
      "23.8",  "31.4",     "50.3",     "51.76",    "52.8",     "53.596",   "54.40",    "54.94",
      "55.50", "57.29034", "57.29034", "57.29034", "57.29034", "57.29034", "57.29034", "88.20",
      "165.5", "183.31",   "183.31",   "183.31",   "183.31",   "183.31"};
    og.vars.createWithScales<std::string>("CenterFreq@MetaData", {og.vars["Channel"]}, params)
      .write<std::string>(vCentrFreq)
      .atts.add<std::string>("long_name", std::string("Center frequency of instrument channel"))
      .add("units", std::string("GHz"));

    std::array<int, atms_numchannels> vPol{0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1};
    og.vars.createWithScales<int>("MetaData/Polarization", {og.vars["Channel"]}, params_int)
      .write<int>(vPol)
      .atts.add<std::string>("long_name", std::string("Polarization of instrument channel"))
      .add<int>("valid_range", {0, 6});

    // Let's assume a magical swath that has a bottom corner of (0,0), with 0.5 degree spacing
    // in both latitude and longitude.
    // Let's also make the TBs smoothly varying for each channel. This is total garbage data,
    // but it plots nicely.

    Eigen::ArrayXXf eLat(atms_initlines, atms_scanpos);
    Eigen::ArrayXXf eLon(atms_initlines, atms_scanpos);
    // Note the different index order for a 3-D tensor. This is to align Eigen and
    // the row-major conventions.
    Eigen::Tensor<float, 3, Eigen::RowMajor> eBTs(atms_initlines, atms_scanpos, atms_numchannels);
    for (int i = 0; i < atms_initlines; ++i)
      for (int j = 0; j < atms_scanpos; ++j) {
        eLat(i, j) = static_cast<float>(i) / 2.f;
        eLon(i, j) = (static_cast<float>(j) / 2.f) + (static_cast<float>(i) / 6.f);
        for (int k = 0; k < atms_numchannels; ++k) {
          const float degtorad = 3.141592654f / 180.f;
          eBTs(i, j, k) =
            150 + (150 * sin(degtorad * static_cast<float>(i))) +
            (100 * cos(degtorad * static_cast<float>(15 + (4 * j) + (8 * k)))) +
            (15 * sin(2 * degtorad * static_cast<float>(i)) * cos(4 * degtorad * static_cast<float>(j)));
        }
      }

    og.vars
      .createWithScales<float>("Latitude@MetaData", {og.vars["ScanLine"], og.vars["ScanPosition"]},
                               params_float)
      .writeWithEigenRegular(eLat)
      .atts.add<std::string>("long_name", std::string("Latitude"))
      .add<std::string>("units", std::string("degrees_north"))
      .add<float>("valid_range", {-90, 90});

    og.vars
      .createWithScales<float>("Longitude@MetaData", {og.vars["ScanLine"], og.vars["ScanPosition"]},
                               params_float)
      .writeWithEigenRegular(eLon)
      .atts.add<std::string>("long_name", std::string("Longitude"))
      .add<std::string>("units", std::string("degrees_east"))
      .add<float>("valid_range", {-360, 360});

    // Note again the ordering.
    og.vars
      .createWithScales<float>("ObsValue/Inst_brightnessTemperature_Uncorrected",
                               {og.vars["ScanLine"], og.vars["ScanPosition"], og.vars["Channel"]},
                               params_float)
      .writeWithEigenTensor(eBTs)
      .atts.add<std::string>("long_name", std::string("Raw instrument brightness temperature"))
      .add<std::string>("units", std::string("K"))
      .add<float>("valid_range", {120, 500})
      // Default display settings
      .add<std::string>("coordinates", std::string("Longitude Latitude Channel"));

    // Some tests
    Expects(!og.vars["ScanLine"].getDimensionScaleName().empty());
    Expects(og.vars["ObsValue/Inst_brightnessTemperature_Uncorrected"].isDimensionScaleAttached(
      0, og.vars["ScanLine"]));
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
