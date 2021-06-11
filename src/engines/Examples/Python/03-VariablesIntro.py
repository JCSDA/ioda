#
# (C) Copyright 2020 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

#  Variables store data. They are generally treated as an extension of
#  Attributes; everything an Attribute can do, a Variable can do better.
#  Variables are resizable, chunkable and compressible. They fully support
#  multidimensional data. They may have attached
#  dimension scales*, which gives their dimensions meaning. Variables
#  may be read and written to using Eigen objects, xTensors, and other
#  user-friendly classes for data manipulation (i.e. replacement ObsDataVectors).

import os
import sys

if os.environ.get('LIBDIR') is not None:
    sys.path.append(os.environ['LIBDIR'])

import ioda
# TODO: Uncomment once we are sure that numpy is a required dependency!
#import numpy # numpy is very useful when handling multidimensional data.

g = ioda.Engines.HH.createFile(
    name = "Example-03-python.hdf5",
    mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)

#  You can access all the variables in a group using the ".vars" member object.
#  i.e. g.vars;

#  ".vars" is an instance of the "Has_Variables" class. This class is defined
#  in "include/ioda/Variables/Has_Variables.h". Like with Has_Attributes, the
#  definition is slightly convoluted, but the key point to remember is that
#  everything in Has_Varables_Base is available to Has_Variables.
#  Also remember to check the Doxygen documentation if you get confused.

#  Let's make some Variables.

# Basic variable creation and writing

# The create function is the same as for Attributes.
intvar1 = g.vars.create(name='var-1', dtype=ioda.Types.int32, dimsCur=[2,3])
# The writeVector functions are likewise identical.
intvar1.writeVector.int32([1,2,3,4,5,6])
# Note: we seldom have variables that have only one element. So,
# variables have no readDatum and writeDatum functions.

var2 = g.vars.create(name='var-2', dtype=ioda.Types.float, dimsCur=[2,3,4])
var2.writeVector.float([1.1,2.2,3.14159,4,5,6,7,8,9,10,11.5,12.6,13,14,15,16,17,18,19,20,21,22,23,24])

# Creating and writing multidimensional objects:
# TODO: needs numpy in the example environment.
#  The writeNPArray functions can write any type of numpy array.
#var_md = g.vars.create(name='var-md', dtype=ioda.Types.float, dimsCur=[2,3])
#var_md_data = np.fromfunction(lambda i, j: i + (j / 2), (2,3), dtype=float)
#var_md.writeNPArray.float(var_md_data)

# Advanced:
# Making a chunked, resizable, compressed variable with default element values.
#  Chunking expresses how contiguous subsets of data are stored on-disk
#  and in-memory. It is needed to enable internal data compression.
#  Usually, ioda sets sensible defaults for chunk sizes
#   when you create a variable using the ObsGroup interface (Example 5).
#   But, you might want to override it.
#  Likewise, ioda usually sets a sensible default for the "fill value"
#   of a variable (the value used to express missing or unwritten data).
#  All of these variable creation properties can be controlled by passing
#  a customized VariableCreationParameters object on function creation.

p1 = ioda.VariableCreationParameters()
p1.chunk = True
p1.chunks = [200, 3]
p1.setFillValue.int32(-999)
p1.compressWithGZIP()
g.vars.create(name='var-3', dtype=ioda.Types.int32, dimsCur=[200,3], dimsMax=[2000,3], params=p1)

# Listing variables and checking existence
# Same as with attributes.

g.vars.list() # Returns a list

if len(g.vars.list()) > 5:
    raise Exception("Way too many variables were created.")

if g.vars.exists('var-2') == False:
    raise Exception("Missing var-2")

# Create and remove a variable
g.vars.create('removable-int', ioda.Types.int32, [1])
g.vars.remove('removable-int')

# Opening a variable
reopened_v3 = g.vars.open('var-3')

# Querying type and dimensions

reopened_v3.dims # Trivial. Returns dimensions.

if reopened_v3.isA2(dtype=ioda.Types.float) == True:
    raise Exception("var-3 should be a set of int32s, not single-precision floats!")

# Reading data... almost the same as with an Attribute.
# You can use readVector or readNPArray.
# The data type is used when creating the resultant Python / numPy object.
vals2 = var2.readVector.float()

