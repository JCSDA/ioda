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
 * \defgroup ioda_cxx_ex_2 Ex 2: Attribute manipulation
 * \brief Shows how to manipulate Attributes and an introduction to
 * the type system.
 * \see 02-Attributes.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 02-Attributes.cpp
 * \brief Shows how to manipulate Attributes and an introduction to
 * the type system.
 *
 * Attributes are metadata that help describe a Group or a Variable.
 * Good examples of attributes include descriptive labels about
 * the source of your data, a description or long name of a variable,
 * and it's valid range (which is the interval where the data are valid).
 *
 * ioda's attributes are flexible. They can be single points, or they can
 * be 1-D arrays. They may be integers, or floats, or doubles, strings,
 * complex numbers, or really any type that you can think of. We will go
 * through the attribute creation, listing, opening, reading and writing
 * functions in this example.
 *
 * This example creates an HDF5 file, "Example-02.hdf5" using the HDF5 backend.
 * This file may be viewed with the "h5dump" of "ncdump" commands. You can
 * also use "hdfview" if you want a gui.
 *
 * Note also: This example introduces Attributes. Variables are very similar to
 * this but have more features. Variables are the focus of the next few tutorials.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <array>     // Arrays are fixed-length vectors.
#include <iostream>  // We want I/O.
#include <string>    // We want strings
#include <valarray>  // Like a vector, but can also do basic element-wise math.
#include <vector>    // We want vectors

#include "ioda/Engines/EngineUtils.h"  // Used to kickstart the Group engine.
#include "ioda/Exception.h"        // Exceptions and debugging
#include "ioda/Group.h"            // Groups have attributes.

int main(int argc, char** argv) {
  using namespace ioda;  // All of the ioda functions are here.
  using std::cerr;
  using std::endl;
  using std::string;
  using std::vector;
  try {
    // We want to open a new file, usually backed by HDF5.
    // We open this file as a root-level Group.
    Group g = Engines::constructFromCmdLine(argc, argv, "Example-02.hdf5");

    // All of the attribute information for a Group or a Variable (see later
    // tutorials) may be accessed by the ".atts" member object.
    // i.e. g.atts;

    // ".atts" is an instance of the "Has_Attributes" class.
    // This tutorial shows you how to *use* this object.
    // If you want to *understand* how it is implemented, read the next paragraph.
    // If not, then skip.

    // You can find Has_Attributes's definition
    // in "include/ioda/Attributes/Has_Attributes.h". The class definition is not trivial.
    // Has_Attributes is a derived class that inherits from detail::Has_Attributes_Base,
    // which is a *template* class. If that's confusing, then just think that everything
    // in Has_Attributes_Base is also available in Has_Attributes.
    // We use this really weird pattern in writing the code because we share class
    // structures between the frontend that you use (Has_Attributes) and the backends
    // that implement the functionality (which all derive from Has_Attributes_Backend).
    // When reading any of the ioda-engines header files, try to keep this in mind.

    // Since we just created a new, empty file, we don't have any attributes yet.
    // Let's create some.

    // The fundamental creation function is .create<Type>(name, {dimensions})
    // What this does is is creates a new attribute, called "int-att-1".
    // This attribute holds a single ({1}) integer (<int>).
    // The function returns the new attribute. No data is yet written to this attribute.
    ioda::Attribute intatt1 = g.atts.create<int>("int-att-1", {1});
    // Write a single integer
    intatt1.write<int>(5);

    // Let's create and write an attribute that stores two integers.
    ioda::Attribute intatt2 = g.atts.create<int>("int-att-2", {2});
    intatt2.write<int>({1, 2});
    // The {} notation is used to define a C++ initializer list. Dimensions ( { 2 } )
    // are specified using initializer lists. These lists can be multi-dimensional,
    // but support for this depends on the backend.
    // We just wrote the data {1,2} using an initializer list also.

    // We can always re-write an attribute with different data.
    intatt2.write<int>({3,4});
    // ioda is flexible enough to support reading and writing attributes when the
    // data storage type is different than the data access type. For example, intatt2
    // is stored as a 32-bit integer, but we can read and write 16-bit ints.
    intatt2.write<int16_t>({5,6});

    // For convenience, Has_Attributes also provides a .add function that combines
    // .create and .write.

    // This creates an int attribute that holds 3 elements, and assigns the values 1,2,3.
    g.atts.add<int>("int-att-3", {1, 2, 3}, {3});

    // You might wonder about creating multi-dimensional attributes.
    // Something like:
    // g.atts.add<int>("int-att-3a", { 1,2,3,-1,-2,-3 }, { 3,2 });
    // This does work, but not all backends support it for now.
    // NetCDF4 does not support this yet, so "ncdump" will not understand a file with
    // a multi-dimensional attribute. Multi-dimensional variables, though, are okay.

    // One last case:
    // If you want to make an attribute that stores only a single element,
    // yo do not need to specify its dimensions.
    g.atts.add<int>("int-att-4", 42);

    // Let's write some more complicated data using a gsl::span container.
    // The gsl stands for the Guideline Support Library, which contains a set of
    // templated recommended by the
    // [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).
    // The gsl::span class is accepted into C++20, and ioda will be switched to using
    // std::span once it is widely available.
    //
    // gsl::span is a replacement for (pointer, length) pairs to refer to a sequence
    // of contiguous objects. It can be thought of as a pointer to an array, but unlike
    // a pointer, it knows its bounds.
    //
    // For example, a span<int, 7> refers to a sequence of seven contiguous integers.
    // A span *does not own* the elements it points to. It is not a container like an
    // array or a vector, it is a *view* into the contents of such a container.
    //
    // See https://github.com/isocpp/CppCoreGuidelines/blob/master/docs/gsl-intro.md
    // for details of why we want to prefer it over regular pointers.

    // We can make a span from std::vector, std::array, std::valarray quite trivially.
    // We can also make a span from a C-style array, or from std::begin, std::end,
    // or from iterators.
    std::vector<int> v_data_5{1, 2, 3, 4, 5, 6, 7, 8, 9};  // A vector of data.
    std::array<int, 6> a_data_6{1, 2, 3, 4, 5, 6};         // A fixed-length array of data.
    std::valarray<int> va_data_7{1, 2, 3, 4};              // A basic math-supporting vector.
    const size_t sz_ca_data_8 = 7;
    const int ca_data_8[sz_ca_data_8]
      = {1, 2, 3, 4, 5, 6, 7};  // NOLINT: (Humans should ignore this comment.)

    g.atts.add<int>("int-att-5", gsl::make_span(v_data_5));
    g.atts.add<int>("int-att-6", gsl::make_span(a_data_6));
    g.atts.add<int>("int-att-7", gsl::make_span(std::begin(va_data_7), std::end(va_data_7)));
    g.atts.add<int>("int-att-8", gsl::make_span(ca_data_8, sz_ca_data_8));

    // Expanations for the above lines:
    //
    // For all: gsl::make_span encodes the size of the data, so
    //   you do not have to specify the attribute's dimensions. It gets passed through
    //   to the .create function transparently.
    //
    // int-att-5 was made from a vector. To convert a vector to a span, just
    //   use gsl::make_span(your_vector_here).
    // int-att-6 was made from a std::array. Same treatment as a std::vector.
    // int-att-7 is slightly harder. std::valarray does not provide .begin and .end functions,
    //   so we use std::begin and std::end when constructing the span.
    // int-att-8's data comes from a C-stype array. To make the span, we provide the
    //   pointer to the start of the data, and the length.

    // DATA TYPES

    // By now, I assume that you are bored with just writing integers.
    // Writing doubles, floats, strings, and so on are easy.
    g.atts.add<float>("float-1", {3.1159f, 2.78f}, {2});
    g.atts.add<double>("double-1", {1.1, 2.2, 3.3, 4.4}, {4});
    // Write a single, variable-length string:
    g.atts.add<std::string>("str-1", std::string("This is a test."));
    // You need the std::string() wrapper because otherwise the C++
    // compiler may assume that you want to do this:
    // g.atts.add<char>("char-1", {"This is a test"}, {15});
    // This is a fixed-length set of characters, which is _completely_ different.

    // Notes on the type system:
    //
    // C++11 defines many different *fundamental* types, which are:
    // bool, short int, unsigned short int, int, unsigned int,
    // long int, unsigned long int, long long int, unsigned long long int,
    // signed char, unsigned char, char, wchar_t, char16_t, char32_t,
    // float, double and long double.
    // These fundamental types are _distinct_ _types_ in the language. We support them all.
    //
    // We also support variable-length and fixed-length array types, usually
    // to implement strings but sometimes also to implement more complex data storage.
    // Perhaps you might want to store complex numbers or tuples or a datetime object.
    // That's under development, but the real takeaway is that the "type" of an
    // attribute and its "dimensions" are separate things.

    // Listing, opening and querying attributes

    // Listing
    // This is easy. We return a vector instead of a set because one day we might care
    // about ordering.
    std::vector<std::string> attList = g.atts.list();
    if (attList.size() != 11)
      throw ioda::Exception("Unexpected attribute count.")
      .add("Expected", 11)
      .add("Actual", attList.size());

    // Opening
    // Also easy. We can use the .open() function, or use square brackets.
    ioda::Attribute f1 = g.atts["float-1"];
    ioda::Attribute d1 = g.atts.open("double-1");

    // Get dimensions
    // Returns a ioda::Dimensions structure. This structure is shared with Variables.
    ioda::Dimensions f1_dims = f1.getDimensions();
    // Dimensionality refers to the number of dimensions the attribute has.
    Expects(f1_dims.dimensionality == 1);
    // dimsCur is the current dimensions of the attribute. For Attributes, these
    // are fixed at creation time and always agree with dimsMax. (Variables are different.)
    // Attributes are not expandable and have no unlimited dimensions.
    // dimsCur and dimsMax are vectors with "dimensionality" elements.
    Expects(f1_dims.dimsCur[0] == 2);
    Expects(f1_dims.dimsMax[0] == 2);
    // numElements is the product of all of the dimsCur elements.
    Expects(f1_dims.numElements == 2);

    // Check type
    // With the frontend / backend pattern, it is really hard to "get" the type into any
    // form that C++ can intrinsically understand.
    // It's much better to check if the Attribute's type matches a type in C++.
    Expects(g.atts["int-att-1"].isA<int>() == true);

    // Reading attributes

    // Opening and then reading an attribute with a single element.
    int int1val = g.atts["int-att-1"].read<int>();
    Expects(int1val == 5);
    // This can instead be written using a convenience function to do both at once.
    Expects(g.atts.read<int>("int-att-1") == 5);

    // Read into any type of span.
    // For the general case, we need to make sure that the span's size matches the
    // number of elements in the attribute. An error will be thrown otherwise.
    std::array<float, 2> check_float_1;  // NOLINT: (Humans should ignore this comment.)
    g.atts.read<float>("float-1", gsl::make_span(check_float_1));

    // Reading into a vector is special. A vector is resizable, and an overload of
    // the read function does this for us.
    std::vector<double> check_double_1;
    g.atts.read<double>("double-1", check_double_1);  // no gsl::span for the vector read.

    // A valarray is like a vector. We can resize it automatically, and thus also specialize it.
    std::valarray<double> check_double_1_valarray;
    g.atts.read<double>("double-1", check_double_1_valarray);

    // Type conversions are implicit
    std::valarray<int16_t> check_intatt2;
    g.atts.read<int16_t>("int-att-2", check_intatt2);
    Expects(check_intatt2[0] == 5);
    Expects(check_intatt2[1] == 6);

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}

/// @}
/// @}
