#
# (C) Copyright 2020-2021 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

#  The ObsGroup class is derived from the Group class and provides some help in
#  organizing your groups, variables, attributes and dimension scales into a cohesive
#  structure intended to house observation data. In this case "structure" refers to the
#  hierarchical layout of the groups and the proper management of dimension scales
#  associated with the variables.
#  
#  The ObsGroup and underlying layout policies (internal to ioda-engines) present a stable
#  logical group hierarchical layout to the client while keeping the actual layout implemented
#  in the backend open to change. The logical "frontend" layout appears to the client to be
#  as shown below:
# 
#    layout                                    notes
# 
#      /                                   top-level group
#       Location                           dimension scales (variables, coordinate values)
#       Channel
#       ...
#       ObsValue/                          group: observational measurement values
#                brightnessTemperature    variable: Tb, 2D, Location X Channel
#                air_temperature           variable: T, 1D, Location
#                ...
#       ObsError/                          group: observational error estimates
#                brightnessTemperature
#                air_temperature
#                ...
#       PreQC/                             group: observational QC marks from data provider
#                brightnessTemperature
#                air_temperature
#                ...
#       MetaData/                          group: meta data associated with locations
#                latitude
#                longitude
#                datetime
#                ...
#       ...
# 
#  We intend to keep this layout stable so that the client interface remains stable.
#  The actual layout used in the various backends can optionally be organized differently
#  according to their needs.
# 
#  The ObsGroup class also assists with the management of dimension scales. For example, if
#  a dimension is resized, the ObsGroup::resize function will resize the dimension scale
#  along with all variables that use that dimension scale.
# 
#  The basic ideas is to dimension observation data with Location as the first dimension, and
#  allow Location to be resizable so that it's possible to incrementally append data along
#  the Location (1st) dimension. For data that have rank > 1, the second through nth dimensions
#  are of fixed size. For example, brightnessTemperature can be store as 2D data with
#  dimensions (Location, Channel).

import os
import sys

from pyioda import ioda
import numpy as np

g = ioda.Engines.HH.createFile(
    name = "Example-05a-python.hdf5",
    mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)

# We have opened the file, but now we want to turn it into an ObsGroup.
# To do this, we need to first specify our dimensions. Then, we call
# the ObsGroup.generate function.


numLocs = 40
numChans = 30

# This is a list of the dimensions that we want in our ObsGroup.
# Both dimensions can be represented as 32-bit integers.
# They are called 'Location' and 'Channel'.
#
# The NewDimensionScale.* functions take four parameters:
#  1. The name of the dimension scale.
#  2. The initial length (size) of this scale. If you have 40 locations,
#     then the Location scale's initial size should be 40.
#  3. The maximum length of the scale. By setting this to ioda.Unlimited,
#     you can resize all of the variables depending on a scale and
#     append new data without recreating the ObsSpace. This doesn't make
#     sense for all possible dimensions... instrument channels are usually
#     fixed, so the max length should match the initial length in those cases.
#  4. A "hint" for the chunking size used by Variables that depend on this
#     scale. When reading data from a disk or in memory, ioda stores data in
#     "blocks". The chunking size determines the size of these blocks.
#     For details on block sizes see https://support.hdfgroup.org/HDF5/doc/Advanced/Chunking/
# 
# Let's make a list of the new dimension scales.
newDims = [ioda.NewDimensionScale.int32('Location', numLocs, ioda.Unlimited, numLocs),
          ioda.NewDimensionScale.int32('Channel', numChans, numChans, numChans)]

# ObsGroup.generate takes a Group argument (the backend we just created
# above) and a list of dimension creation scales. It then creates the dimensions
# and performs initialization of an ObsGroup.
og = ioda.ObsGroup.generate(g, newDims)

# You can open the scales using the names that you provided in the
# NewDimensionScale calls.
LocationVar = og.vars.open('Location')
ChannelVar = og.vars.open('Channel')

# Just setting some sensible defaults: compress the stored data and use
# a fill value of -999.
p1 = ioda.VariableCreationParameters()
p1.compressWithGZIP()
p1.setFillValue.float(-999)

#  Next let's create the variables. The variable names should be specified using the
#  hierarchy as described above. For example, the variable brightnessTemperature
#  in the group ObsValue is specified in a string as "ObsValue/brightnessTemperature".
tbName = "ObsValue/brightnessTemperature"
latName = "MetaData/latitude"
lonName = "MetaData/longitude"

# We create three variables to store brightness temperature, latitude and longitude.
# We attach the appropriate scales with the scales option.
tbVar = g.vars.create(tbName, ioda.Types.float, scales=[LocationVar, ChannelVar], params=p1)
latVar = g.vars.create(latName, ioda.Types.float, scales=[LocationVar], params=p1)
lonVar = g.vars.create(lonName, ioda.Types.float, scales=[LocationVar], params=p1)

# Let's set some attributes on the variables to describe what they mean,
# their ranges, their units, et cetera.
tbVar.atts.create("coordinates", ioda.Types.str, [1]).writeDatum.str("longitude latitude Channel")
tbVar.atts.create("long_name", ioda.Types.str, [1]).writeDatum.str("fictional brightness temperature")
tbVar.atts.create("units", ioda.Types.str, [1]).writeDatum.str("K")
tbVar.atts.create("valid_range", ioda.Types.float, [2]).writeVector.float([100.0, 400.0])

latVar.atts.create("long_name", ioda.Types.str, [1]).writeDatum.str("latitude")
latVar.atts.create("units", ioda.Types.str, [1]).writeDatum.str("degrees_north")
latVar.atts.create("valid_range", ioda.Types.float, [2]).writeVector.float([-90, 90])

lonVar.atts.create("long_name", ioda.Types.str, [1]).writeDatum.str("degrees_east")
lonVar.atts.create("units", ioda.Types.str, [1]).writeDatum.str("degrees_north")
lonVar.atts.create("valid_range", ioda.Types.float, [2]).writeVector.float([-360, 360])

# Let's create some data for this example.
# We are generating filler data.
# Perhaps consult numpy.fromfunction's documentation.

midLoc = numLocs / 2
midChan = numChans / 2

# Latitude and longitude are 1-D numpy arrays, with length numLocs.
# Each array index is fed into a lambda function that generates a value
# at each location.
lonData = np.fromfunction(lambda i: 3 * (i % 8), (numLocs,), dtype=float)
latData = np.fromfunction(lambda i: 3 * (i // 8), (numLocs,), dtype=float)
# Brightness temperatures are two-dimensional. They have dimensions of
# location and channel, so the lambda function takes two arguments.
tbData = np.fromfunction(lambda iloc, ich: 250 + (((iloc - midLoc)**2 + (ich-midChan)**2))**0.5, (numLocs, numChans), dtype=float)

# Write the data into the variables.
lonVar.writeNPArray.float(lonData)
latVar.writeNPArray.float(latData)
tbVar.writeNPArray.float(tbData)
