/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_ex Examples
 * \brief C++ Usage Examples
 * \ingroup ioda_cxx
 *
 * @{
 * \dir Basic
 * \brief Basic C++ ioda-engines usage tutorials
 *
 * \defgroup ioda_cxx_ex_1 Ex 1: Groups and ObsSpaces
 * \brief Group manipulation using the C++ interface
 * \see 01-GroupsAndObsSpaces.cpp for the example.
 *
 * @{
 *
 * \file 01-GroupsAndObsSpaces.cpp
 * \brief First example showing how to make a group (or an ObsSpace).
 *
 * IODA is the Interface for Observation Data Access.
 *
 *  The objective of the IODA is to provide uniform access to
 *  observation data across the whole forecasting chain from
 *  observation pre-processing to data assimilation to diagnostics.
 *
 *  IODA isolates the scientific code from the underlying data
 *  structures holding the data. Of course, any user needs to have
 *  a basic understanding of how the data are laid out. How are the
 *  data grouped, how can you access the data, how can you
 *  interpret variable names, and how can you read dimensions
 *  and meta data?
 *
 *  Data in IODA are stored in a structure Groups, Variables, and
 *  Attributes. A Group is like a folder. It is a logical
 *  collection of Variables and Attributes that describes some
 *  portion of the overall data. A Variable stores bulk data.
 *  An Attribute stores smaller quantities of metadata, and can
 *  be attached to either a Group or a Variable.
 *
 *  In ioda-engines, we separate out how the data are stored
 *  (the backend engine) from how an end user can access it (in
 *  the frontend). Different backends all provide the same general
 *  interface, but may support slightly different features and will
 *  have different performance characteristics.
 *
 *  This example shows you how to create Groups. It creates an
 *  HDF5 file, "Example-01.hdf5" using the HDF5 backend. Later examples
 *  will use groups to store and read data.
 *
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <iostream>  // We want I/O.
#include <string>    // We want strings
#include <vector>    // We want vectors

#include "ioda/Engines/Factory.h"  // Used to kickstart the Group engine.
#include "ioda/Group.h"            // We are manipulating ioda::Groups.

int main(int argc, char** argv) {
  using namespace ioda;  // All of the ioda functions are here.
  using std::cerr;
  using std::endl;
  using std::string;
  using std::vector;
  try {
    // We want to open a new file, backed by the default engine (HDF5).
    // We open this file as a root-level Group.
    Group grpFromFile = Engines::constructFromCmdLine(argc, argv, "Example-01.hdf5");
    // Note: After you build and run this example, you can view the contents of this
    // HDF5 file with either the "h5dump" or "ncdump" commands.
    // Since we are only using a subset of HDF5's full feature set, netCDF conveniently
    // recognizes this as a valid netCDF-4 file!

    // The only time that you need to be concerned about the
    // backend (HDF5) is when you create or open a root-level Group.
    // All Variables and Attributes within a Group transparently
    // use the same backend.

    // Groups can contain other Groups!
    // To create a new group, use the .create() method.
    // The new group is a child group of the object that is used to
    // create it. It shares the same underlying backend as its parent.
    Group g1 = grpFromFile.create("g1");
    Group g2 = grpFromFile.create("g2");

    // Groups can form a tree structure.
    Group g3 = g1.create("g3");
    Group g4 = g3.create("g4");
    Group g5 = g4.create("g5");
    Group g6 = g4.create("g6");
    Group g8 = g6.create("g7/g8");

    // Your tree looks like this:
    //
    // / - g1 - g3 - g4 - g5
    //   |                |
    //   g2               g6 - g7 - g8
    //

    // Besides creating Groups, we can also check if a particular
    // group exists, list them and open them.

    // Checking existance
    bool g3exists = g1.exists("g3");
    if (!g3exists) throw;  // jedi_throw.add("Reason", "g3 does not exist.");

    // Nesting
    // We can use '/' as a path separator.
    bool g4exists = g1.exists("g3/g4");
    if (!g4exists) throw;  // jedi_throw.add("Reason", "g4 does not exist.");

    // Listing the groups contained within a group.
    // The .list() function returns a vector of strings
    // listing all immediate (one-level) child groups.
    vector<string> g3children = g3.list();  // Should be { "g4" }.
    if (g3children.size() != 1)
      throw;                                /* jedi_throw
                 .add("Reason", "g3 contents are unexpected.")
                 .add("size", g3children.size()); */
    vector<string> g4children = g4.list();  // Should be { "g5", "g6" }.
    if (g4children.size() != 2)
      throw; /* jedi_throw
.add("Reason", "g4 contents are unexpected.")
.add("size", g4children.size()); */

    // When an error is encountered, you'll notice that
    // we did a "throw jedi_throw" statement. This throws an error
    // exception that is implemented in the jedi code.
    // "jedi_throw" is a macro that tags the current source file,
    // current function, and the line # in the current file, and
    // creates an "xThrow" object. You can then use the
    // .add(key, value) function to add in diagnostic information.
    // You can add any information that you could write using an
    // iostream like cout.
    // So, you can pass values like strings and numbers without
    // difficulty. Complex objects like vectors or sets would
    // need more work.

    // Opening groups
    // This is also really easy. Use the .open function.
    // It also obeys nesting criteria, and throws on an error.
    Group opened_g3 = g1.open("g3");
    Group opened_g6 = opened_g3.open("g4/g6");

    // Groups g3 and opened_g3 point to the same object.
    // Groups g6 and opened_g6 also point to the same object.
    // Any changes that you make in one of these groups will
    // be instantly visible to the other.

    // Note: we make no guarantees about concurrent access using
    // threads. That is a detail left up to the backend, and is
    // an area of future work.

    // What about closing groups?
    // These Group objects can go out of scope, and they release
    // their resource locks when they destruct. So, there is no
    // specific close method.
    // If you _really_ want to close an object, just reassign it.
    opened_g3 = Group{};  // opened_g3 now points elsewhere.

    // If all references to a specific backend instance are closed,
    // then it is released and does its cleanup tasks.

    // What about Attributes and Variables?
    // See tutorial 2 for Attributes.
    // Variables are covered in tutorials 3-7.

    // Thanks for reading!
  } catch (const std::exception& e) {
    cerr << "An error occurred.\n\n" << e.what() << endl;
    return 1;
  }
  return 0;
}

/// @}
/// @}
