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
 * \defgroup ioda_cxx_ex_3 Ex 3: Introduction to Variables
 * \brief Basic usage of Variables
 * \see 03-VariablesIntro.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 03-VariablesIntro.cpp
 * \brief Basic usage of Variables
 *
 * Variables store data. They are generally treated as an extension of
 * Attributes; everything an Attribute can do, a Variable can do better.
 * Variables are resizable, chunkable and compressible. They fully support
 * multidimensional data. They may have attached
 * *dimension scales*, which gives their dimensions meaning. Variables
 * may be read and written to using Eigen objects, xTensors, and other
 * user-friendly classes for data manipulation (i.e. replacement ObsDataVectors).
 *
 * This example creates an HDF5 file, "Example-03.hdf5" using the HDF5 backend.
 * This file may be viewed with the "h5dump" of "ncdump" commands. You can
 * also use "hdfview" if you want a gui.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <hdf5.h>  // Turn off hdf5 auto-error handling. Needed for the example.

#include <array>     // Arrays are fixed-length vectors.
#include <iostream>  // We want I/O.
#include <string>    // We want strings
#include <valarray>  // Like a vector, but can also do basic element-wise math.
#include <vector>    // We want vectors

#include "Eigen/Dense"                     // Eigen Arrays and Matrices
#include "ioda/Engines/EngineUtils.h"          // Used to kickstart the Group engine.
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
    Group g = Engines::constructFromCmdLine(argc, argv, "Example-03.hdf5");

    // You can access all the variables in a group using the ".vars" member object.
    // i.e. g.vars;

    // ".vars" is an instance of the "Has_Variables" class. This class is defined
    // in "include/ioda/Variables/Has_Variables.h". Like with Has_Attributes, the
    // definition is slightly convoluted, but the key point to remember is that
    // everything in Has_Varables_Base is available to Has_Variables.
    // Also remember to check the Doxygen documentation if you get confused.

    // Let's make some Variables.

    // The most basic creation function is .create<Type>(name, {dimensions}). Same as with creating
    // an attribute.
    ioda::Variable intvar1 = g.vars.create<int32_t>("var-1", {2, 3});
    // The above creates a 2x3 variable that contains integers.
    // * First difference from attributes: multidimensional data is fully supported. You can create
    // points, 1-D, 2-D, 3-D, ..., n-dimensional data.

    // Writing a small amount of data is also easy.
    intvar1.write<int32_t>({1, 2, 3, 4, 5, 6});
    // Just like with Attributes, you can use initializer lists and spans to write data.
    // Unlike with attributes, there is no ".add" function, so you always have to
    // use ".create" and ".write". This is deliberate, because variable creation can become much
    // more complicated than attribute creation.

    // You can overwrite data easily. Also, you do not need to match the variable's storage type
    // with the type used to store data in memory. ioda is smart enough to perform this
    // conversion automatically.
    intvar1.write<int16_t>({-1, -2, -3, -4, -5, -6});

    //return 0;
    // However, you can still chain operations:
    g.vars.create<float>("var-2", {2, 3, 4}).write<float>({1.1f, 2.2f, 3.14159f, 4,  5,     6,
                                                           7,    8,    9,        10, 11.5f, 12.6f,
                                                           13,   14,   15,       16, 17,    18,
                                                           19,   20,   21,       22, 23,    24});

    // * The second difference: variables can be resized.
    // The create function also can take a few other parameters, such as maximum dimensions,
    // attachable dimension scales, and information about chunking and compression.
    // We won't cover this in this tutorial, but full details are available in Tutorial 6
    // (Variable Creation Properties).
    // In that tutorial, you'll learn to create variables using expressions like these:
    {
      ioda::VariableCreationParameters p1;
      p1.chunk  = true;
      p1.chunks = {200, 3};  // "Chunk" every 600 elements together.
      p1.setFillValue<int>(-999);
      p1.compressWithGZIP();
      // Make a 200x3 variable, that can be expanded up to have 2000x3 dimensions,
      // with a fill value of -999, that is compressed with GZIP.
      g.vars.create<int>("var-3", {200, 3}, {2000, 3}, p1);
    }

    // Basic writing of data.

    // The vector, valarray and span constructors work the same way as with Attributes.
    // We can make a span from std::vector, std::array, std::valarray quite trivially.
    // We can also make a span from a C-style array, or from std::begin, std::end,
    // or from iterators.
    std::vector<int> v_data_4{1, 2, 3, 4, 5, 6, 7, 8, 9};  // A vector of data.
    std::array<int, 6> a_data_5{1, 2, 3, 4, 5, 6};         // A fixed-length array of data.
    std::valarray<int> va_data_6{1, 2, 3, 4};              // A basic math-supporting vector.
    const size_t sz_ca_data_7 = 7;
    const int ca_data_7[sz_ca_data_7]
      = {1, 2, 3, 4, 5, 6, 7};  // NOLINT: (Humans should ignore this comment)

    g.vars.create<int>("var-4", {gsl::narrow<ioda::Dimensions_t>(v_data_4.size())})
      .write<int>(gsl::make_span(v_data_4));
    g.vars.create<int>("var-5", {gsl::narrow<ioda::Dimensions_t>(a_data_5.size())})
      .write<int>(gsl::make_span(a_data_5));
    g.vars.create<int>("var-6", {gsl::narrow<ioda::Dimensions_t>(va_data_6.size())})
      .write<int>(gsl::make_span(std::begin(va_data_6), std::end(va_data_6)));
    // A variable in a Group.
    g.vars.create<int>("exgroup/var-7", {sz_ca_data_7})
      .write<int>(gsl::make_span(ca_data_7, sz_ca_data_7));
    // You should notice that the creation and writing are a bit "inelegant" in that we seem to
    // specify the size twice, when creating and when writing the data. There are two
    // reasons for this:
    // 1) We aren't specifying the _size_ when creating the variable. We are
    //    specifying the _dimensions_.
    //    These examples all happen to be 1-D data. Vectors, arrays, etc. all
    //    lack an understanding of data dimensions.
    // 2) The amount of data being written is very small. Most of the time, your data
    //    will be much larger, you might want to separate the variable creation / writing
    //    logic, and you might not even need to read or write the _entire_ variable.

    // Note: you can specify nested paths using slashes. "exgroup/var-7" refers to a
    // variable ("var-7") that is in a group ("exgroup"). The following three lines
    // are thus equivalent.
    Expects(g.exists("exgroup"));
    Expects(g.open("exgroup").vars.exists("var-7"));
    Expects(g.vars.exists("exgroup/var-7"));

    // What happens if you write the wrong type of data to a variable?
    // ioda-engines assumes that this is an error, and throws.
    try {
      auto bad_1 = g.vars.create<int>("bad-int-1", {1});
      bad_1.write<float>(std::vector<float>{2.2f});
    } catch (const std::exception&) {
    }

    // Writing Eigen objects

    // Eigen (http://eigen.tuxfamily.org/) is a is a high-level C++ library of template headers
    // for linear algebra, matrix and vector operations, geometrical transformations, numerical
    // solvers and related algorithms.
    // You can use it to read data from / write data into ioda while preserving the data's
    // dimensions. You can use Eigen's containers to **do math** in a natural manner, without
    // constantly iterating over array indices. We support i/o with all Eigen objects.

    // Here is a 30x30 block of integers.
    const int num_i = 30, num_j = 30;
    Eigen::ArrayXXi x(num_i, num_j);
    // Let's set some initial data.
    for (int i = 0; i < num_i; ++i)
      for (int j = 0; j < num_j; ++j) x(i, j) = j + 3 * i;
    // To write this data, we have a special function called .writeWithEigenRegular.
    ioda::Variable ioda_x = g.vars.create<int>("var-x", {num_i, num_j});
    ioda_x.writeWithEigenRegular(x);
    // The .writeWithEigenRegular function is used to write Eigen Arrays, Matrices, Blocks, etc.
    // There is also a .writeWithEigenTensor function that can write Eigen::Tensor objects.

    // Doing math with Eigen:

    Eigen::ArrayXXi y(num_i, num_j);  // Define another variable, y.
    for (int i = 0; i < num_i; ++i)
      for (int j = 0; j < num_j; ++j) y(i, j) = (i * i) - j;

    // We've defined x and y. How about z = 2*y - x?
    Eigen::ArrayXXi z = (2 * y) - x;
    // Literally, it's that simple.

    g.vars.create<int>("var-y", {num_i, num_j}).writeWithEigenRegular(y);
    g.vars.create<int>("var-z", {num_i, num_j}).writeWithEigenRegular(z);

    // Eigen::Tensors are a multidimensional storage container.
    Eigen::Tensor<int, 3, Eigen::RowMajor> data_4d(3, 3, 3);
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 3; ++k) data_4d(i, j, k) = i + j - k;
    g.vars.create<int>("var-4d", {3, 3, 3}).writeWithEigenTensor(data_4d);

    // For more information regarding how to use Eigen properly, see the tutorial at
    // http://eigen.tuxfamily.org/dox/group__TutorialMatrixClass.html

    // Listing, opening and querying variables

    // Listing
    // This is easy. We return a vector instead of a set because one day we might care
    // about ordering.
    std::vector<std::string> varList = g.vars.list();
    if (varList.size() != 11)
      throw ioda::Exception("Unexpected variable count.", ioda_Here())
        .add("Expected", 11)
        .add("Actual", varList.size());

    // Checking variable existence and removing.
    if (!g.vars.exists("var-2"))
      throw ioda::Exception("Variable var-2 does not exist.", ioda_Here());
    g.vars.create<int>("removable-int-1", {1});
    g.vars.remove("removable-int-1");

    // Opening
    // As with attributes, we can use the .open() function, or use square brackets.
    ioda::Variable y1 = g.vars["var-y"];
    ioda::Variable z1 = g.vars.open("var-z");

    // You should check that a variable exists before opening it.
    // If the variable does not exist, ioda-engines will throw an error.
    try {
      // Note: Using an HDF5-specific function here because we are deliberately triggering an error.
      H5Eset_auto(H5E_DEFAULT, NULL, NULL);  // Turn off HDF5 automatic error handling.
      ioda::Variable z2;
      z2 = g.vars.open("var-z-2");
    } catch (const std::exception&) {  // This is expected.
    }

    // Get dimensions
    // Returns a ioda::Dimensions structure. This structure is shared with Variables.
    ioda::Dimensions y1_dims = y1.getDimensions();
    // Dimensionality refers to the number of dimensions the attribute has.
    Expects(y1_dims.dimensionality == 2);
    // dimsCur is the current dimensions.
    Expects(y1_dims.dimsCur[0] == num_i);
    Expects(y1_dims.dimsCur[1] == num_j);
    // dimsMax are the maximum dimensions.
    // Many variables are resizable, in which case dimsMax's elements will not equal those of
    // dimsCur. We will discuss creating resizable and chunked variables in a later
    // tutorial.
    Expects(y1_dims.dimsMax[0] == num_i);

    // Check type
    // With the frontend / backend pattern, it is really hard to "get" the type into any
    // form that C++ can intrinsically understand.
    // It's much better to check if the Attribute's type matches a type in C++.
    Expects(y1.isA<int>() == true);

    // Reading an entire variable

    // Into a vector
    std::vector<int> v_data_1_check;
    g.vars["var-1"].read<int>(v_data_1_check);
    Expects(v_data_1_check[0] == -1);
    Expects(v_data_1_check[1] == -2);
    Expects(v_data_1_check[2] == -3);
    Expects(v_data_1_check[3] == -4);
    Expects(v_data_1_check[4] == -5);
    Expects(v_data_1_check[5] == -6);

    // Check type conversion. The internal storage
    // type is int32_t, and we are reading into a vector of int64_t.
    std::vector<int64_t> v_data_1_check_type_conversion;
    g.vars["var-1"].read<int64_t>(v_data_1_check_type_conversion);
    Expects(v_data_1_check_type_conversion[3] == -4);

    // Into a valarray
    std::valarray<int> va_data_4_check;
    g.vars["var-4"].read<int>(va_data_4_check);
    Expects(va_data_4_check[3] == 4);

    // Into a span
    std::array<int, 6> check_a_data_5;  // NOLINT: (Humans should ignore this comment.)
    g.vars["var-5"].read<int>(gsl::make_span(check_a_data_5));
    Expects(check_a_data_5[3] == 4);

    // With Eigen
    Eigen::ArrayXXi y1_check;
    y1.readWithEigenRegular(y1_check);
    Expects(y1_check(0, 0) == 0);  // i^2 - j
    Expects(y1_check(2, 1) == 3);
    Expects(y1_check(1, 2) == -1);
    Expects(y1_check(2, 2) == 2);

    ioda::Dimensions v4d_dims = g.vars["var-4d"].getDimensions();
    Eigen::Tensor<int, 3, Eigen::RowMajor> data_4d_check(v4d_dims.dimsCur[0], v4d_dims.dimsCur[1],
                                                         v4d_dims.dimsCur[2]);
    g.vars["var-4d"].readWithEigenTensor(data_4d_check);
    Expects(data_4d_check(0, 0, 0) == 0);  // i + j - k
    Expects(data_4d_check(1, 0, 0) == 1);
    Expects(data_4d_check(0, 1, 0) == 1);
    Expects(data_4d_check(0, 0, 1) == -1);

    // If you try to read data into an object that has a truly incompatible
    // storage type, such as reading ints into a string, then ioda-engines will complain.
    try {
      std::vector<std::string> y1_check_bad;
      y1.read<std::string>(y1_check_bad); // This will fail
    } catch (...) {
      // Silently catch in this example
    }

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}

/// @}
/// @}
