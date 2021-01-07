#
# (C) Copyright 2020 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This first example shows you how to make and manipulate Groups.
#
# IODA is the Interface for Observation Data Access.
#
#  The objective of the IODA is to provide uniform access to
#  observation data across the whole forecasting chain from
#  observation pre-processing to data assimilation to diagnostics.
# 
#  IODA isolates the scientific code from the underlying data
#  structures holding the data. Of course, any user needs to have
#  a basic understanding of how the data are laid out. How are the
#  data grouped, how can you access the data, how can you
#  interpret variable names, and how can you read dimensions
#  and meta data?
# 
#  Data in IODA are stored in a structure Groups, Variables, and
#  Attributes. A Group is like a folder. It is a logical
#  collection of Variables and Attributes that describes some
#  portion of the overall data. A Variable stores bulk data.
#  An Attribute stores smaller quantities of metadata, and can
#  be attached to either a Group or a Variable.
# 
#  In ioda-engines, we separate out how the data are stored
#  (the backend engine) from how an end user can access it (in
#  the frontend). Different backends all provide the same general
#  interface, but may support slightly different features and will
#  have different performance characteristics.
# 
#  This example shows you how to create Groups. It creates an
#  HDF5 file, "Example-01-python.hdf5" using the HDF5 backend. Later examples
#  will use groups to store and read data.
# 

# How to view the output of these examples:
#
# These examples also double as CTest tests. So, we have a tiny
# bit of a setup preamble at the beginning of each example that helps
# us to load in the ioda Python library.
#
# If you want to run the example, try using ctest (i.e.
#   ctest -V -R Example_ioda_engines_01_python).

# Note: After you build and run this example, you can view the contents of this
# HDF5 file with either the "h5dump" or "ncdump" commands.
# Since we are only using a subset of HDF5's full feature set, netCDF conveniently
# recognizes this as a valid netCDF-4 file!
#
# If you run this example in a Jupyter kernel, you might need
# to shutdown the kernel to convince Python to release its resouce
# locks and finish writing the output file. Every Group, Attribute,
# and Variable returned by ioda is actually a "resource handle". Every
# open handle must be closed for the file to finish writing. Or, track
# down every ioda Python object and "delete" it
# (e.g. "del grpFromFile", "del g1", "del g2", ad nauseam).

import os
import sys

if os.environ.get('LIBDIR') is not None:
    sys.path.append(os.environ['LIBDIR'])

# Importing ioda is easy.

import ioda

# We want to open a new file, backed by the default engine (HDF5).
# We open this file as a root-level Group.
grpFromFile = ioda.Engines.HH.createFile(
	name = "Example-01-python.hdf5",
	mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)

# The only time that you need to be concerned about the
# backend (HDF5) is when you create or open a root-level Group.
# All Variables and Attributes within a Group transparently
# use the same backend.
#
# Groups can contain other Groups!
# To create a new group, use the .create() method.
# The new group is a child group of the object that is used to
# create it. It shares the same underlying backend as its parent.

g1 = grpFromFile.create('g1')
g2 = grpFromFile.create("g2")

# Groups can form a tree structure.
g3 = g1.create("g3")
g4 = g3.create("g4")
g5 = g4.create("g5")
g6 = g4.create("g6")
g8 = g6.create("g7/g8")

# Your tree looks like this:
#
# / - g1 - g3 - g4 - g5
#   |    |
#   g2   g6 - g7 - g8
# 
     
# Besides creating Groups, we can also check if a particular
# group exists, list them and open them.

# Checking existance
if g1.exists('g3') != True:
	raise Exception("g3 does not exist in /g1")

# Nesting
# We can use '/' as a path separator.
if g1.exists('g3/g4') != True:
	raise Exception("g1/g3/g4 does not exist")

# Listing the groups contained within a group.
# The .list() function returns a list of strings
# listing all immediate (one-level) child groups.
# This function DOES NOT list variables and attributes. For those,
# you would need g3.vars.list() and g3.atts.list(), which are
# discussed in subsequent tutorials.
g3.list()

# These lists are regular Python lists. Nothing special about them.
if len(g3.list()) != 1:
	raise Exception("Wrong size for g3!")

if len(g4.list()) != 2:
	raise Exception("Wrong number of children in g4!")

# C++ and Python exceptions can coexist!
# C++ exceptions are converted into Python exceptions.

# Opening groups
# This is also really easy. Use the .open function.
# It also obeys nesting criteria, and throws on an error.
opened_g3 = g1.open("g3")
opened_g6 = opened_g3.open("g4/g6")

# Groups g3 and opened_g3 are handles that point to the same object.
# Groups g6 and opened_g6 also point to the same object.
# Any changes that you make in one of these groups will
# be instantly visible to the other.

# Note: we make no guarantees about concurrent access using
# threads. That is a detail left up to the backend, and is
# an area of future work.

# What about closing groups?
# These Group objects can go out of scope, and they release
# their resource locks when they destruct. So, there is no
# specific close method.
# If you _really_ want to close an object, do this:
del opened_g3

# If all references to a specific backend instance are closed,
# then it is released and does its cleanup tasks.

# What about Attributes and Variables?
# See tutorial 2 for Attributes.
# Variables are covered in tutorials 3-4.

# What are ObsGroups?
# These extend Groups, and they are covered in tutorial 5. 

# Thanks for reading!


