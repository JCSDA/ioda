/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_ex_types
 *
 * @{
 *
 * \ingroup ioda_cxx_ex_flstrings
 * @{
 *
 * \file fixed_length_strings_client.cpp
 * \brief Client-side fixed-length string demonstration.
 *
 * C++ has no standard class for fixed-length string data.
 * This example tests IODA's capability to convert fixed and
 * variable-length string data by writing an array of characters
 * (fixed-length) to a variable-length string data type.
 * 
 * This is a rare case in the IODA code. This example proves that
 * IODA can do this.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <array>     // Arrays are fixed-length vectors.
#include <iostream>  // We want I/O.
#include <string>    // We want strings
#include <valarray>  // Like a vector, but can also do basic element-wise math.
#include <vector>    // We want vectors

#include "ioda/Engines/Factory.h"  // Used to kickstart the Group engine.
#include "ioda/Exception.h"        // Exceptions and debugging
#include "ioda/Group.h"            // Groups have attributes.

int main(int argc, char** argv) {
  using namespace ioda;  // All of the ioda functions are here.
  using std::cerr;
  using std::endl;
  using std::string;
  using std::vector;
  try {
    // Create a new file.
    Group g = Engines::constructFromCmdLine(argc, argv, "fixed_length_strings_client.hdf5");

    // Writing and reading fixed-length strings

    // A set of four six-character strings.
    // We could also use std::array, or std::string to store these.
    //const char data[] = {"Test 1Test 2Test 3Test 4"};
    const std::string data {"Test 1Test 2Test 3Test 4"};

    // This string type has a length of six characters.
    Type type_fixed_str_6 = g.atts.getTypeProvider()->makeStringType(typeid(string), 6);

    // Attribute tests
    {
        // Main test


        // Create a variable-length string attribute
        Attribute att_vlen_str = g.atts.create<std::string>("att_vlen_str", {4});

        // Write some data to this attribute.
        att_vlen_str.write(data, type_fixed_str_6);

        // Read the attribute's data and check.
        std::vector<char> chk(data.size());
        att_vlen_str.read(chk, type_fixed_str_6);

        if (chk.size() != data.size()) throw Exception("Bad read. Size mismatch.", ioda_Here())
            .add("chk.size()", chk.size()).add("data.size()", data.size());

        string str_chk(chk.begin(), chk.end());
        if (str_chk != data) throw Exception("String mismatch.", ioda_Here())
            .add("data", data).add("str_chk", str_chk);



        // Another test. Fixed-length string on both ends.

        // Create, write, read, and check.
        Attribute att_flen_str = g.atts.create("att_flen_str", type_fixed_str_6, {4});
        att_flen_str.write(data, type_fixed_str_6);
        att_flen_str.read(chk, type_fixed_str_6);
        if (chk.size() != data.size()) throw Exception("Bad read. Size mismatch.", ioda_Here())
            .add("chk.size()", chk.size()).add("data.size()", data.size());
        string str_chk2(chk.begin(), chk.end());
        if (str_chk2 != data) throw Exception("String mismatch.", ioda_Here())
            .add("data", data).add("str_chk2", str_chk2);
    }

    // Variable tests
    {
        // Main test

        // Create a variable-length string variable
        Variable var_vlen_str = g.vars.create<std::string>("var_vlen_str", {4});

        // Write some data to this attribute.
        var_vlen_str.write(data, type_fixed_str_6);

        // Read the variable's data and check.
        std::vector<char> chk(data.size());
        var_vlen_str.read(chk, type_fixed_str_6);

        if (chk.size() != data.size()) throw Exception("Bad read. Size mismatch.", ioda_Here())
            .add("chk.size()", chk.size()).add("data.size()", data.size());

        string str_chk(chk.begin(), chk.end());
        if (str_chk != data) throw Exception("String mismatch.", ioda_Here())
            .add("data", data).add("str_chk", str_chk);




        // Another test. Fixed-length string on both ends.
        
        // Create, write, read, and check.
        Variable var_flen_str = g.vars.create("var_flen_str", type_fixed_str_6, {4});
        var_flen_str.write(data, type_fixed_str_6);
        var_flen_str.read(chk, type_fixed_str_6);
        if (chk.size() != data.size()) throw Exception("Bad read. Size mismatch.", ioda_Here())
            .add("chk.size()", chk.size()).add("data.size()", data.size());
        string str_chk2(chk.begin(), chk.end());
        if (str_chk2 != data) throw Exception("String mismatch.", ioda_Here())
            .add("data", data).add("str_chk2", str_chk2);
    }

    // Done!

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}

/// @}
/// @}
