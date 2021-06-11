#
# (C) Copyright 2020 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This example shows how to manipulate Attributes and other metadata
# that describe Groups and Variables.

# Attributes are metadata that help describe a Group or a Variable.
# Good examples of attributes include descriptive labels about
# the source of your data, a description or long name of a variable,
# and it's valid range (which is the interval where the data are valid).
#
# ioda's attributes are flexible. They can be single points, or they can
# be 1-D arrays. They may be integers, or floats, or doubles, strings,
# complex numbers, or really any type that you can think of. We will go
# through the attribute creation, listing, opening, reading and writing
# functions in this example.
#
# This example creates an HDF5 file, "Example-02.hdf5" using the HDF5 backend.
# This file may be viewed with the "h5dump" of "ncdump" commands. You can
# also use "hdfview" if you want a gui.
#
# Note also: This example introduces Attributes. Variables are very similar to
# this but have more features. Variables are the focus of the next few tutorials.

import os
import sys

if os.environ.get('LIBDIR') is not None:
    sys.path.append(os.environ['LIBDIR'])

import ioda

# All of the attribute information for a Group or a Variable (see later
# tutorials) may be accessed by the ".atts" member object.
# i.e. g.atts;

# ".atts" is an instance of the "Has_Attributes" class.
# This tutorial shows you how to *use* this object.
# If you want to *understand* how it is implemented, read the next paragraph.
# If not, then skip.

# You can find Has_Attributes's definition
# in "include/ioda/Attributes/Has_Attributes.h". The class definition is not trivial.
# Has_Attributes is a derived class that inherits from detail::Has_Attributes_Base,
# which is a *template* class. If that's confusing, then just think that everything
# in Has_Attributes_Base is also available in Has_Attributes.
# We use this really weird pattern in writing the code because we share class
# structures between the frontend that you use (Has_Attributes) and the backends
# that implement the functionality (which all derive from Has_Attributes_Backend).
# When reading any of the ioda-engines header files, try to keep this in mind.

# Since we just created a new, empty file, we don't have any attributes yet.
# Let's create some.

g = ioda.Engines.HH.createFile(
    name = "Example-02-python.hdf5",
    mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)


# The fundamental creation function is .create(name, datatype, dimensions)
# What this does is is creates a new attribute, called "int-att-1".
# This attribute holds a single ({1}) integer (int32).
# The dtype parameter is one of several enumerated types in ioda.Types.
#   These types match the types in C++. int32 is a 32-bit integer.
#   Other types include float, double, int16, uint64, et cetera.
# The dims parameter is a list representing the dimensions of the attribute.
#   Attributes can have a single dimension. Variables (discussed later)
#   can be multidimensional.
# The function returns the new attribute. No data is yet written to this attribute.
intatt1 = g.atts.create(name="int-att-1", dtype=ioda.Types.int32, dims=[1])
# Write a single integer to the attribute using the writeDatum function.
# writeDatum writes just one value. If the attribute contains more than one
# value, then writeDatum will fail.
#
# Note the calling convention for the Python interface.
#  [object].writeDatum.[type](). Python does not have C++'s expressive
#  template support, so we have to define separate functions that
#  implement a single C++ template (i.e. write<DataType>(...)).
# Additionally, Python types do not align perfectly with C++ types.
#  So, we need to specify the C++ type, and the interface converts the
#  values if necessary.
#  write*.int32 means:
#   first convert the data to int32_t,
#   then write the data.
intatt1.writeDatum.int32(1)

# Create another attribute. This time, it stores two values.
intatt2 = g.atts.create("int-att-2", ioda.Types.int32, [2])
# You can pass a list of values to write to the writeVector function.
# writeVector takes in any one-dimensional Python list and copies its
# data into the Attribute.
intatt2.writeVector.int32([1, 2])

# Same as above.
intatt3 = g.atts.create("int-att-3", ioda.Types.int32, [3])
intatt3.writeVector.int32([1, 2, 3])

# Create and write a C++-style float. A float has less precision than
# a double.
float1 = g.atts.create("float-1", ioda.Types.float, [2])
float1.writeVector.float([3.1159, 2.78])

# Same for a double.
g.atts.create("double-1", ioda.Types.double, [4]).writeVector.double([1.1,2.2,3.3,4.4])

# Write a variable-length, UTF-8 string.
# All strings in ioda are assumed to be this type of string.
g.atts.create("str-1", ioda.Types.str, [1]).writeVector.str(["This is a test."])

# To list all attributes attached to an object (a Group or Variable),
# use the atts.list() function. This returns a Python list.
attlist = g.atts.list()

if len(attlist) != 6:
    raise Exception("Wrong size for g.atts.list!")

# Opening an attribute
f1 = g.atts.open('float-1')
d1 = g.atts.open('double-1')

# The dimensions of an attribute are set at creation time and are
# fixed. An attribute cannot be resized.
# This dimensions class is shared with variables, so not all members
# are relevant to attributes.
# The dimensions object has four members:
# 1) the overall dimensionality (for an Attribute, this is always one).
# 2) the current dimensions (a list)
# 3) the maximum dimensions (pertains only to resizable objects; a list)
# 4) the total number of elements (a scalar integer)

f1_dims = f1.dims

if f1_dims.dimensionality != 1:
    raise Exception("Unexpected dimensionality")

if f1_dims.dimsCur[0] != 2:
    raise Exception("Unexpected size")

if f1_dims.dimsMax[0] != 2:
    raise Exception("Unexpected size")

if f1_dims.numElements != 2:
    raise Exception("Unexpected size")

# To open an attribute:
a2_reopened = g.atts.open('int-att-2')

# To check the storage type of an object:
#   There are two very similar ways to do this, with isA and isA2.
#   The differences are in the templating and the parameters.
#   Prefer isA unless you are testing multiple types dynamically.

if float1.isA.float() != True:
    raise Exception("Unexpected type")

if g.atts.open('int-att-1').isA2(ioda.Types.int32) != True:
    raise Exception("Unexpected type")

# Reading attributes:
# readDatum and readVector are complimentary calls to
# writeDatum and writeVector.

# readDatum returns a single element.
# readVector returns a list.

i2vals = a2_reopened.readVector.int32()
if len(i2vals) != 2:
    raise Exception("Wrong length")

# That's everything about attributes!

