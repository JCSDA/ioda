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
 * \defgroup ioda_cxx_ex_flstrings Fixed-length strings
 * \brief Shows how to create and use fixed-length string types
 *
 * @{
 *
 * \file fixed_length_strings.cpp
 * \brief Shows how to create and use fixed-length string types
 *
 * Fixed-length string data types are supported by IODA. They can be significantly
 * faster than variable-length string types, particularly when reading or writing
 * very large numbers of strings. However, they are inefficient at storing
 * variable-length data and are somewhat more awkward to use.
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
    // Create a new file.
    Group g = Engines::constructFromCmdLine(argc, argv, "fixed_length_strings.hdf5");

    // Writing and reading fixed-length strings

    // You can use regular std::string objects for storage.
    vector<string> vsTest{{"Test 1"}, {"Test 2"}};

    // Fixed-length strings need custom types.
    // The "type provider" is a special class that is specific to each backend that
    // tells a backend how to construct a particular data type.

    // Fixed-length string lengths are defined as str.length(). They do **not**
    // include a trailing null, unlike C-style strings.

    // This string type has a length of six characters.
    Type type_fixed_str_6 = g.atts.getTypeProvider()->makeStringType(typeid(string), 6);

    // Attribute tests
    {
      // Create an attribute
      Attribute fs1 = g.atts.create("fixed-str-1",                                // Name
                                    type_fixed_str_6,                             // Type
                                    {static_cast<Dimensions_t>(vsTest.size())});  // Dimensions

      // Write some data to this attribute.
      fs1.write(vsTest);

      // Read the attribute
      vector<string> chk_vsTest;
      fs1.read(chk_vsTest);
      if (chk_vsTest.size() != vsTest.size())
        throw Exception("Size mismatch.", ioda_Here())
          .add("chk_vsTest.size()", chk_vsTest.size())
          .add("vsTest.size()", vsTest.size());
      for (size_t i = 0; i < vsTest.size(); ++i)
        if (vsTest[i] != chk_vsTest[i])
          throw Exception("Element mismatch.", ioda_Here())
            .add("i", i)
            .add("vsTest[i]", vsTest[i])
            .add("chk_vsTest[i]", chk_vsTest[i]);
    }

    // Variable tests
    {
      // Create a variable
      Variable fs2 = g.vars.create("fixed-str-2",                                // Name
                                   type_fixed_str_6,                             // Type
                                   {static_cast<Dimensions_t>(vsTest.size())});  // Dimensions

      // Write data to the variable
      fs2.write(vsTest);

      // Read the variable
      vector<string> chk_vsTest;
      fs2.read(chk_vsTest);
      if (chk_vsTest.size() != vsTest.size())
        throw Exception("Size mismatch.", ioda_Here())
          .add("chk_vsTest.size()", chk_vsTest.size())
          .add("vsTest.size()", vsTest.size());
      for (size_t i = 0; i < vsTest.size(); ++i)
        if (vsTest[i] != chk_vsTest[i])
          throw Exception("Element mismatch.", ioda_Here())
            .add("i", i)
            .add("vsTest[i]", vsTest[i])
            .add("chk_vsTest[i]", chk_vsTest[i]);
    }

    // Type system tests
    {
      Attribute test_fixed_length_att    = g.atts.open("fixed-str-1");
      Attribute test_variable_length_att = g.atts.create<std::string>("variable-str-1", {1});

      // Check that an object is a string (either a fixed-length or variable-length type).
      // Method 1
      if (!test_fixed_length_att.isA<std::string>())
        throw Exception("test_fixed_length_att is somehow not a string.", ioda_Here());
      if (!test_variable_length_att.isA<std::string>())
        throw Exception("test_variable_length_att is somehow not a string.", ioda_Here());
      // Method 2
      if (type_fixed_str_6.getClass() != TypeClass::String)
        throw Exception("type_fixed_str_6 is somehow not a string type.", ioda_Here());

      // To get a type of an attribute or a variable
      Type t = test_variable_length_att.getType();

      // Check that a type represents a fixed-length or a variable-length string.
      if (type_fixed_str_6.isVariableLengthStringType())
        throw Exception("type_fixed_str_6 should be a fixed-length string type.", ioda_Here());
      if (t.isVariableLengthStringType() == false)
        throw Exception("t should be a variable-length string type.", ioda_Here());

      // To get the size of a fixed-length string type.
      // This represents the size allocated for a string, in bytes. This does **not** always
      // match the **length** of the string, which is measured in number of characters.
      // These quantities can differ:
      // 1. for UTF-8 strings, where some characters are multi-byte characters.
      // 2. when reading strings, since not all strings will be the maximum length.
      // Note also that this size does not account for any NULL byte used to denote the
      // end of a C-style string.
      size_t sz = type_fixed_str_6.getSize();
      if (sz != 6) throw Exception("Bad size. Should be 6 bytes.", ioda_Here());

      // Get the character set of a string type.
      // This can be either ASCII or UTF-8.
      // For now in IODA, all strings are forcibly assumed to be UTF-8. Reads between
      // actual ASCII / UTF-8 data are handled transparently.
      if (type_fixed_str_6.getStringCSet() != StringCSet::UTF8)
        throw Exception("Unexpected character set.", ioda_Here());
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
