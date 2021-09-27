/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_ex_basic
 *
 * @{
 *
 * \defgroup ioda_cxx_ex_4 Ex 4: Variables, Attributes, and Dimension Scales
 * \brief Variables, Attributes, and Dimension Scales
 * \see 04-VariablesAttributesAndDimensions.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 04-VariablesAttributesAndDimensions.cpp
 * \brief Variables, Attributes, and Dimension Scales
 *
 * Variables store data, but how should this data be interpreted? This is the
 * purpose of attributes. Attributes are bits of metadata that can describe groups
 * and variables. Good examples of attributes include tagging the units of a variable,
 * giving it a descriptive range, listing a variable's valid range, or "coding" missing
 * or invalid values.
 *
 * Basic manipulation of attributes was already discussed in Tutorial 2. Now, we want to
 * focus instead on good practices with tagging your data.
 *
 * Supplementing attributes, we introduce the concept of adding "dimension scales" to your
 * data. Basically, your data have dimensions, but we want to attach a "meaning" to each
 * axis of your data. Typically, the first axis corresponds to your data's Location.
 * A possible second axis for brightness temperature data might be "instrument channel", or
 * maybe "pressure level". This tutorial will show you how to create new dimension scales and
 * attach them to new Variables.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <array>     // Arrays are fixed-length vectors.
#include <iostream>  // We want I/O.
#include <string>    // We want strings
#include <valarray>  // Like a vector, but can also do basic element-wise math.
#include <vector>    // We want vectors

#include "Eigen/Dense"                     // Eigen Arrays and Matrices
#include "ioda/Engines/Factory.h"          // Used to kickstart the Group engine.
#include "ioda/Exception.h"                // Exceptions and debugging
#include "ioda/Group.h"                    // Groups have attributes.
#include "unsupported/Eigen/CXX11/Tensor"  // Eigen Tensors

int main(int argc, char** argv) {
  using namespace ioda;  // All of the ioda functions are here.
  using std::cerr;
  using std::endl;
  using std::string;
  using std::vector;
  try {
    // We want to open a new file, backed by HDF5.
    // We open this file as a root-level Group.
    Group g = Engines::constructFromCmdLine(argc, argv, "Example-04.hdf5");

    // Let's start with dimensions and Dimension Scales.

    // ioda stores data using Variables, and you can view each variable as a
    // multidimensional matrix of data. This matrix has dimensions.
    // A dimension may be used to represent a real physical dimension, for example,
    // time, latitude, longitude, or height. A dimension might also be used to
    // index more abstract quantities, for example, color-table entry number,
    // instrument number, station-time pair, or model-run ID
    //
    // A dimension scale is simply another variable that provides context, or meaning,
    // to a particular dimension. For example, you might have ATMS brightness
    // temperature information that has dimensions of location by channel number. In ioda,
    // we want every "axis" of a variable to be associated with an explanatory dimension.

    // Let's create a few dimensions... Note: when working with an already-existing Obs Space,
    // (covered later), these dimensions may already be present.

    // Create two dimensions, "Location", and "ATMS Channel". Set distinct values within
    // these dimensions.
    const int num_locs     = 3000;
    const int num_channels = 23;
    ioda::Variable dim_location
      = g.vars.create<int>("Location", {num_locs})
          .writeWithEigenRegular(Eigen::ArrayXi(num_locs).setLinSpaced(1, num_locs))
          .setIsDimensionScale("Location");
    ioda::Variable dim_channel
      = g.vars.create<int>("ATMS Channel", {num_channels})
          .writeWithEigenRegular(Eigen::ArrayXi(num_channels).setLinSpaced(1, num_channels))
          .setIsDimensionScale("ATMS Channel Number");

    // Now that we have created dimensions, we can create new variables and attach the
    // dimensions to our data.

    // But first, a note about attributes:
    // Attributes provide metadata that describe our variables.
    // In IODA, we at least must to keep track of each variable's:
    // - Units (in SI; we follow CF conventions)
    // - Long name
    // - Range of validity. Data outside of this range are automatically rejected for
    //   future processing.

    // Let's create variables for Latitude, Longitude and for
    // ATMS Observed Brightness Temperature.

    // There are two ways to define a variable that has attached dimensions.
    // First, we can explicitly create a variable and set its dimensions.

    // Longitude has dimensions of Location. It has units of degrees_east, and has
    // a valid_range of (-180,180).
    ioda::Variable longitude = g.vars.create<float>("Longitude", {num_locs});
    longitude.setDimScale(dim_location);
    longitude.atts.add<float>("valid_range", {-180, 180})
      .add<std::string>("units", std::string("degrees_east"))
      .add<std::string>("long_name", std::string("Longitude"));

    // The above method is a bit clunky because you have to make sure that the new variable's
    // dimensions match the sizes of each dimension.
    // Alternatively, there is a convenience function, ".createWithScales", that
    // condenses this a bit for you.

    // Latitude has units of degrees_north, and a valid_range of (-90,90).
    ioda::Variable latitude = g.vars.createWithScales<float>("Latitude", {dim_location});
    latitude.atts.add<float>("valid_range", {-90, 90})
      .add<std::string>("units", std::string("degrees_north"))
      .add<std::string>("long_name", std::string("Latitude"));

    // The ATMS Brightness Temperature depends on both location and instrument channel number.
    ioda::Variable tb
      = g.vars.createWithScales<float>("Brightness Temperature", {dim_location, dim_channel});
    tb.atts.add<float>("valid_range", {100, 500})
      .add<std::string>("units", std::string("K"))
      .add<std::string>("long_name",
                        std::string("ATMS Observed (Uncorrected) Brightness Temperature"));

    // Advanced topic:

    // Variable Parameter Packs
    // When creating variables, you can also provide an optional
    // VariableCreationParameters structure. This struct lets you specify
    // the variable's fill value (a default value that is a placeholder for unwritten data).
    // It also lets you specify whether you want to compress the data stored in the variable,
    // and how you want to store the variable (contiguously or in chunks).

    VariableCreationParameters params;

    // Fill values:
    // The "fill value" for a dataset is the specification of the default value assigned
    // to data elements that have not yet been written.
    // When you first create a variable, it is empty. If you read in a part of a variable that
    // you have not filled with data, then ioda has to return fake, filler data.
    // A fill value is a "bit pattern" that tells ioda what to return.
    params.setFillValue<float>(-999);

    // Variable storage: contiguous or chunked
    // See https://support.hdfgroup.org/HDF5/doc/Advanced/Chunking/ and
    // https://www.unidata.ucar.edu/blogs/developer/en/entry/chunking_data_why_it_matters
    // for detailed explanations.
    // To tell ioda that you want to chunk a variable:
    params.chunk  = true;   // Turn chunking on
    params.chunks = {100};  // Each chunk is a size 100 block of data.

    // Compression
    // If you are using chunked storage, you can tell ioda that you want to compress
    // the data using ZLIB or SZIP.
    // - ZLIB / GZIP:
    params.compressWithGZIP();
    // - SZIP
    // params.compressWithSZIP(int pixelsPerBlock);

    // Let's create one final variable, "Solar Zenith Angle", and let's use
    // out new variable creation parameters.
    auto SZA = g.vars.createWithScales<float>("Solar Zenith Angle", {dim_location}, params);

    SZA.atts.add<float>("valid_range", {-90, 90}).add<std::string>("units", std::string("degrees"));

    // Done!
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}

/// @}
/// @}
