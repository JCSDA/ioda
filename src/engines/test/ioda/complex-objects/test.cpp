/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <iostream>
#include <vector>

#include "ioda/Engines/Factory.h"
#include "ioda/Group.h"

// These tests really need a better check system.
// Boost unit tests would have been excellent here.

template <class T>
void check_equal(const std::string& name, const std::vector<T>& vals, const std::vector<T>& ref) {
  std::ostringstream outerr;
  outerr << "Check of " << name << " failed. Error: ";

  try {
    if (vals.size() != ref.size()) {
      outerr << "check_equal failed. vals.size() = " << vals.size() << ", and ref.size() = " << ref.size()
             << ".\n";
      throw;
    }
    for (size_t i = 0; i < vals.size(); ++i) {
      if (vals[i] != ref[i]) {
        outerr << "check_equal failed at index " << i << ". vals[" << i << "] = " << vals[i] << ", and ref["
               << i << "] = " << ref[i] << ".\n";
        throw;
      }
    }
  } catch (...) {
    throw std::logic_error(outerr.str().c_str());
  }
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    // HDF5 file backend
    auto f = Engines::constructFromCmdLine(argc, argv, "test-complex-objects.hdf5");

    // These tests try to read and write non-fundamental datatypes.
    // These types may be enums, complex structures, strings or arrays.

    // We try to both write and read these with both attributes and variables.

    // Array types
    // TODO(rhoneyager): Marshalling.h needs better template expansion to handle these types automatically.
    /*
    {
            const int adata[2]{ 1,2 };
            f.atts.create<int[2], Types::GetType_Wrapper<int,2>>("c_array_int_2")
                    .write<int[2], ioda::Object_Accessor<int[2]>, Types::GetType_Wrapper<int, 2>>(adata);
    }
    {
            int check_adata[2]{ 0,0 };
            auto a = f.atts.open("c_array_int_2");
            a.read<int[2], ioda::Object_Accessor<int[2]>, Types::GetType_Wrapper<int, 2>>(check_adata);
            check_equal("c_array_int_2", std::vector<int>{check_adata[0], check_adata[1]}, { 1,2 });
    }
    {
            const array<int, 2> adata{ 3,4 };
            f.atts.add<array<int, 2>>("std_array_int_2", adata); // 3, 4 is a single datum.

            f.vars.create<array<int, 2>>("v_std_array_int_2", { 1 }).write<array<int, 2>>(adata);
    }
    {
            // Check the attribute read
            int check_adata[2]; // Can't use std::array<int,2> because I need to expand. See a.read<int[2]...>
            auto a = f.atts.open("std_array_int_2");
            a.read<int[2], ioda::Object_Accessor<int[2]>, Types::GetType_Wrapper<int, 2>>(check_adata);
            check_equal("std_array_int_2", std::vector<int>{check_adata[0], check_adata[1]}, { 3,4 });

            // Check the string read
            auto v = f.vars["v_std_array_int_2"];
            v.read<int[2], ioda::Object_Accessor<int[2]>, Types::GetType_Wrapper<int, 2>>(check_adata);
            check_equal("v_std_array_int_2", std::vector<int>{check_adata[0], check_adata[1]}, { 3,4 });
    }

    */

    // String types

    // Note the curly braces around "String 1". Needed because we are constructing a
    // string from a character array, and we don't want to clash with the span-based
    // function signature.
    f.atts.add<string>("Str_1", {"String 1"});
    f.atts.add<string>("Str_2", {"Hi Steve!", "Hi Ryan!"});
    {
      vector<string> v_data;
      f.atts.read<string>("Str_1", v_data);
      check_equal("Str_1", v_data, {"String 1"});

      f.atts.read<string>("Str_2", v_data);
      check_equal("Str_2", v_data, {"Hi Steve!", "Hi Ryan!"});
    }

    f.vars.create<string>("v_Str_1", {1}).write<string>(std::vector<std::string>{"var String 1"});
    f.vars.create<string>("v_Str_2", {2})
      .write<string>(std::vector<std::string>{"var String 2.1", "var String 2.2"});
    f.vars.create<string>("v_Str_3", {2, 2})
      .write<string>(std::vector<std::string>{"var String 3 [0,0]", "var String 3 [0,1]",
                                              "var String 3 [1,0]", "var String 3 [1,1]"});
    {
      vector<string> v_data;
      f.vars["v_Str_1"].read(v_data);
      check_equal("v_Str_1", v_data, {"var String 1"});

      f.vars["v_Str_2"].read(v_data);
      check_equal("v_Str_2", v_data, {"var String 2.1", "var String 2.2"});

      f.vars["v_Str_3"].read(v_data);
      check_equal("v_Str_3", v_data,
                  {"var String 3 [0,0]", "var String 3 [0,1]", "var String 3 [1,0]", "var String 3 [1,1]"});
    }

    // Check if string variable gets initialized to the fill value
    VariableCreationParameters params;
    string fillString("I_am_fill");
    params.setFillValue<string>(fillString);
    f.vars.create<string>("Str_w_fill", {2, 2}, {2, 2}, params);
    {
      vector<string> v_data;
      f.vars["Str_w_fill"].read(v_data);
      check_equal("Str_w_fill", v_data, {"I_am_fill", "I_am_fill", "I_am_fill", "I_am_fill"});
    }
  } catch (const std::exception& e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}
