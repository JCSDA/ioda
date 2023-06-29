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
 * \defgroup ioda_cxx_ex_5a Ex 5a: ObsGroups
 * \brief Constructing ObsGroups
 * \see 05a-ObsGroup.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 05a-ObsGroup.cpp
 * \brief ObsGroup
 *
 * The ObsGroup class is derived from the Group class and provides some help in
 * organizing your groups, variables, attributes and dimension scales into a cohesive
 * structure intended to house observation data. In this case "structure" refers to the
 * hierarchical layout of the groups and the proper management of dimension scales
 * associated with the variables.
 *
 * The ObsGroup and underlying layout policies (internal to ioda-engines) present a stable
 * logical group hierarchical layout to the client while keeping the actual layout implemented
 * in the backend open to change. The logical "frontend" layout appears to the client to be
 * as shown below:
 *
 *   layout                                    notes
 *
 *     /                                   top-level group
 *      Location                           dimension scales (variables, coordinate values)
 *      Channel
 *      ...
 *      ObsValue/                          group: observational measurement values
 *               brightnessTemperature    variable: Tb, 2D, Location X Channel
 *               air_temperature           variable: T, 1D, Location
 *               ...
 *      ObsError/                          group: observational error estimates
 *               brightnessTemperature
 *               air_temperature
 *               ...
 *      PreQC/                             group: observational QC marks from data provider
 *               brightnessTemperature
 *               air_temperature
 *               ...
 *      MetaData/                          group: meta data associated with locations
 *               latitude
 *               longitude
 *               datetime
 *               ...
 *      ...
 *
 * It is intended to keep this layout stable so that the client interface remains stable.
 * The actual layout used in the various backends can optionally be organized differently
 * according to their needs.
 *
 * The ObsGroup class also assists with the management of dimension scales. For example, if
 * a dimension is resized, the ObsGroup::resize function will resize the dimension scale
 * along with all variables that use that dimension scale.
 *
 * The basic ideas is to dimension observation data with Location as the first dimension, and
 * allow Location to be resizable so that it's possible to incrementally append data along
 * the Location (1st) dimension. For data that have rank > 1, the second through nth dimensions
 * are of fixed size. For example, brightnessTemperature can be store as 2D data with
 * dimensions (Location, Channel).
 *
 * \author Stephen Herbener (stephenh@ucar.edu), Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <array>     // Arrays are fixed-length vectors.
#include <iostream>  // We want I/O.
#include <string>    // We want strings
#include <valarray>  // Like a vector, but can also do basic element-wise math.
#include <vector>    // We want vectors

#include "Eigen/Dense"             // Eigen Arrays and Matrices
#include "ioda/Engines/EngineUtils.h"  // Used to kickstart the Group engine.
#include "ioda/Exception.h"        // Exceptions and debugging
#include "ioda/Group.h"            // Groups have attributes.
#include "ioda/ObsGroup.h"
#include "unsupported/Eigen/CXX11/Tensor"  // Eigen Tensors

int main(int argc, char** argv) {
  using namespace ioda;  // All of the ioda functions are here.
  using std::cerr;
  using std::endl;
  using std::string;
  using std::vector;
  try {
    // Create the backend. For this code we are using a factory function,
    // constructFromCmdLine, made for testing purposes, which allows one to specify
    // a backend from the command line using the "--ioda-engine-options" option.
    Group g = Engines::constructFromCmdLine(argc, argv, "Example-05a.hdf5");

    // Create an ObsGroup object using the ObsGroup::generate function. This function
    // takes a Group argument (the backend we just created above) and a vector of dimension
    // creation specs.
    const int numLocs  = 40;
    const int numChans = 30;

    // The NewDimensionsScales_t is a vector, that holds specs for one dimension scale
    // per element. An individual dimension scale spec is held in a NewDimensionsScale
    // object, whose constructor arguments are:
    //       1st - dimension scale name
    //       2nd - size of dimension. May be zero.
    //       3rd - maximum size of dimension
    //              resizeable dimensions are said to have "unlimited" size, so there
    //              is a built-in variable ("Unlimited") that can be used to denote
    //              unlimited size. If Unspecified (the default), we assume that the
    //              maximum size is the same as the initial size (the previous parameter).
    //       4th - suggested chunk size for dimension (and associated variables).
    //              This defaults to the initial size. This parameter must be nonzero. If
    //              the initial size is zero, it must be explicitly specified.
    //
    ioda::NewDimensionScales_t newDims;
    newDims.push_back(NewDimensionScale<int>("Location", numLocs, Unlimited, numLocs));
    newDims.push_back(NewDimensionScale<int>("Channel", numChans, numChans, numChans));

    // Construct an ObsGroup object, with 2 dimensions Location, Channel, and attach
    // the backend we constructed above. Under the hood, the ObsGroup::generate function
    // initializes the dimension coordinate values to index numbering 1..n. This can be
    // overwritten with other coordinate values if desired.
    ObsGroup og = ObsGroup::generate(g, newDims);

    // We now have the top-level group containing the two dimension scales. We need
    // Variable objects for these dimension scales later on for creating variables so
    // build those now.
    ioda::Variable LocationVar  = og.vars["Location"];
    ioda::Variable ChannelVar = og.vars["Channel"];

    // Next let's create the variables. The variable names should be specified using the
    // hierarchy as described above. For example, the variable brightnessTemperature
    // in the group ObsValue is specified in a string as "ObsValue/brightnessTemperature".
    string tbName  = "ObsValue/brightnessTemperature";
    string tmName = "Metadata/dateTime";
    string latName = "Metadata/latitude";
    string lonName = "Metadata/longitude";

    // Set up the creation parameters for the variables. All three variables in this case
    // are float types, so they can share the same creation parameters object.
    ioda::VariableCreationParameters float_params;
    float_params.chunk = true;               // allow chunking
    float_params.compressWithGZIP();         // compress using gzip
    float_params.setFillValue<float>(-999);  // set the fill value to -999

    // Create the variables. Note the use of the createWithScales function. This should
    // always be used when working with an ObsGroup object.
    Variable tbVar  = og.vars.createWithScales<float>(tbName, {LocationVar, ChannelVar}, float_params);
    Variable tmVar  = og.vars.createWithScales<float>(tmName, {LocationVar}, float_params);
    Variable latVar = og.vars.createWithScales<float>(latName, {LocationVar}, float_params);
    Variable lonVar = og.vars.createWithScales<float>(lonName, {LocationVar}, float_params);

    // Add attributes to variables. In this example, we are adding enough attribute
    // information to allow Panoply to be able to plot the ObsValue/brightnessTemperature
    // variable. Note the "coordinates" attribute on tbVar. It is sufficient to just
    // give the variable names (without the group structure) to Panoply (which apparently
    // searches the entire group structure for these names). If you want to follow this
    // example in your code, just give the variable names without the group prefixes
    // to insulate your code from any subsequent group structure changes that might occur.
    tbVar.atts.add<std::string>("coordinates", {"longitude latitude Channel"}, {1})
      .add<std::string>("long_name", {"ficticious brightness temperature"}, {1})
      .add<std::string>("units", {"K"}, {1})
      .add<float>("valid_range", {100.0, 400.0}, {2});
    tmVar.atts.add<std::string>("units", {"seconds since 2021-12-20"}, {1});
    latVar.atts.add<std::string>("long_name", {"latitude"}, {1})
      .add<std::string>("units", {"degrees_north"}, {1})
      .add<float>("valid_range", {-90.0, 90.0}, {2});
    lonVar.atts.add<std::string>("long_name", {"longitude"}, {1})
      .add<std::string>("units", {"degrees_east"}, {1})
      .add<float>("valid_range", {-360.0, 360.0}, {2});

    // Let's create some data for this example.
    Eigen::ArrayXXf tbData(numLocs, numChans);
    std::vector<float> lonData(numLocs);
    std::vector<float> latData(numLocs);
    float midLoc  = static_cast<float>(numLocs) / 2.0f;
    float midChan = static_cast<float>(numChans) / 2.0f;
    for (std::size_t i = 0; i < numLocs; ++i) {
      lonData[i] = static_cast<float>(i % 8) * 3.0f;
      // We use static code analysis tools to check for potential bugs
      // in our source code. On the next line, the clang-tidy tool warns about
      // our use of integer division before casting to a float. Since there is
      // no actual bug, we indicate this with NOLINT.
      latData[i] = static_cast<float>(i / 8) * 3.0f;  // NOLINT(bugprone-integer-division)
      for (std::size_t j = 0; j < numChans; ++j) {
        float del_i  = static_cast<float>(i) - midLoc;
        float del_j  = static_cast<float>(j) - midChan;
        tbData(i, j) = 250.0f + sqrt(del_i * del_i + del_j * del_j);
      }
    }

    // Write the data into the variables.
    tbVar.writeWithEigenRegular(tbData);
    latVar.write(latData);
    lonVar.write(lonData);

    // Done!
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}

/// @}
/// @}
