/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*!
 * \ingroup ioda_cxx_ex_math
 *
 * @{
 *
 * \file variables_math.cpp
 * \details
 * This example shows how to use the Math and Units APIs when manipulating variables.
 * Lines 1-75 generate some sample data. The relevant part starts around line 76.
 * 
 * We read two variables, perform missing-value-aware and unit-aware math, and
 * then write the result to the output file.
 **/

#include <Eigen/Dense>
#include <iostream>

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/MathOps.h"
#include "ioda/ObsGroup.h"
#include "ioda/Units.h"

int main(int argc, char** argv) {
  using namespace ioda;
  using std::cerr;
  using std::cout;
  using std::endl;
  try {
    // Make a sample output file
    Group g            = Engines::constructFromCmdLine(argc, argv, "variables_math.hdf5");
    const int numLocs  = 40;
    const int numChans = 30;
    NewDimensionScales_t newDims;
    newDims.push_back(NewDimensionScale<int>("Location", numLocs, Unlimited, numLocs));
    ObsGroup og       = ObsGroup::generate(g, newDims);
    Variable Location = og.vars["Location"];

    // Make sample variables
    Variable lat = og.vars.createWithScales<float>("Metadata/latitude", {Location});
    lat.atts.add<std::string>("units", {"degrees_north"});
    Variable lon = og.vars.createWithScales<float>("Metadata/longitude", {Location});
    lon.atts.add<std::string>("units", {"degrees_east"});
    Variable u = og.vars.createWithScales<float>("Metadata/windEastward", {Location});
    u.atts.add<std::string>("units", {"m/s"});
    Variable v = og.vars.createWithScales<float>("Metadata/windNorthward", {Location});
    v.atts.add<std::string>("units", {"m/s"});

    // Fill sample variables with sample data
    {
      std::vector<float> lonData(numLocs), latData(numLocs), uData(numLocs), vData(numLocs);
      float midLoc  = static_cast<float>(numLocs) / 2.0f;
      float midChan = static_cast<float>(numChans) / 2.0f;
      for (std::size_t i = 0; i < numLocs; ++i) {
        lonData[i] = static_cast<float>(i % 8) * 3.0f;
        // We use static code analysis tools to check for potential bugs
        // in our source code. On the next line, the clang-tidy tool warns about
        // our use of integer division before casting to a float. Since there is
        // no actual bug, we indicate this with NOLINT.
        latData[i] = static_cast<float>(i / 8) * 3.0f;  // NOLINT(bugprone-integer-division)
        uData[i]   = static_cast<float>(i / 8) * 4.0f;
        vData[i]   = static_cast<float>(i % 8) * 4.0f;
      }

      lon.write(lonData);
      lat.write(latData);
      u.write(uData);
      v.write(vData);
    }

    // Now for the real part of the example.

    // Read the u and v wind components.
    // The readForMath function gathers the data, missing values, and units, and
    // encapsulates these three parameters into ioda's EigenMath class (ioda/MathOps.h).
    const auto mU = u.readForMath<Eigen::ArrayXf>();
    const auto mV = v.readForMath<Eigen::ArrayXf>();

    // See basic_math.cpp for all of the other math operations you could perform.
    auto mWindMag = ((mU * mU) + (mV * mV)).root(2);

    // Create a new variable with particular units. We implicitly use the
    // NetCDF-4 default fill value.
    Variable windmag = og.vars.createWithScales<float>("Metadata/windMagnitude", {Location});
    windmag.atts.add<std::string>("units", {"mile / hour"});

    // Write the data. Unit and missing value conversions occur automatically.
    windmag.writeFromMath(mWindMag);

  } catch (const std::exception& e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}
