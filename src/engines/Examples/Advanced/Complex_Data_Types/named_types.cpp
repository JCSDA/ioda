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
 * \defgroup ioda_cxx_ex_named The Named Type interface
 * \brief Shows how to save and use named types
 *
 * @{
 *
 * \file named_types.cpp
 * \brief Shows how to save and use named types
 *
 * Types can be committed to IODA files. This lets them be referenced by name,
 * which helps ensure consistency across variables that use these types.
 * Compound and enumerated types only need to be constructed once and
 * may be used multiple times.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <string>  // We want strings
#include <vector>  // We want vectors

#include "ioda/Engines/Factory.h"  // Used to kickstart the Group engine.
#include "ioda/Exception.h"        // Exceptions and debugging
#include "ioda/Group.h"            // Groups have attributes.

int main(int argc, char** argv) {
  using namespace ioda;  // All of the ioda functions are here.
  using std::string;
  using std::vector;
  try {
    // Create a new file.
    Group g = Engines::constructFromCmdLine(argc, argv, "named_types.hdf5");

    // Let's make a few types.
    // Type creation is a bit clunky currently, but this is expected to change soon.

    // Fundamental types.

    // A regular float. Endianness and precision are system-specific.
    Type type_float = g.types.getTypeProvider()->makeFundamentalType(typeid(float));

    // A 32-bit signed integer. Endianness is system-specific.
    Type type_int32_t = g.types.getTypeProvider()->makeFundamentalType(typeid(int32_t));

    // Let's make a few more complicated types and save them directly to the file.

    // A variable-length string in UTF-8.
    Type type_var_str
      = g.types.getTypeProvider()->makeStringType(typeid(string), 0, StringCSet::UTF8);
    type_var_str.commitToBackend(g, "type_var_str_utf8");

    // A variable-length string in ASCII.
    Type type_var_str_ascii
      = g.types.getTypeProvider()->makeStringType(typeid(string), 0, StringCSet::ASCII);
    type_var_str_ascii.commitToBackend(g, "type_var_str_ascii");

    // A fixed-length string in ASCII.
    Type type_fixed_str_6
      = g.types.getTypeProvider()->makeStringType(typeid(string), 6, StringCSet::ASCII);
    type_fixed_str_6.commitToBackend(g, "type_fixed_str_6");

    // An fixed-length array of four doubles.
    Type type_array_4d
      = g.types.getTypeProvider()->makeArrayType({4}, typeid(double), typeid(double));
    type_array_4d.commitToBackend(g, "type_array_4d");

    // List all types
    vector<string> named_types = g.types.list();
    if (named_types.size() != 4) throw Exception("Expected 4 named types.", ioda_Here());

    // Check type existence
    if (!g.types.exists("type_array_4d")) throw Exception("type_array_4d not found.", ioda_Here());

    // Open a type
    Type varstr2 = g.types["type_var_str_utf8"];
    if (varstr2.getClass() != TypeClass::String) throw Exception("Wrong data type.", ioda_Here());

    // Remove a named type. Any variables or attributes are untouched. The linked name
    // is simply removed.
    g.types.remove("type_var_str_utf8");

    // The end file has three types: type_array_4d, type_fixed_str_6, and type_var_str_ascii.

    // Done!
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}

/// @}
/// @}
