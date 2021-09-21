#
# (C) Copyright 2020-2021 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This example supplements the previous discussion on variables.
#
#  Variables store data, but how should this data be interpreted? This is the
#  purpose of attributes. Attributes are bits of metadata that can describe groups
#  and variables. Good examples of attributes include tagging the units of a variable,
#  describing a variable, listing a variable's valid range, or "coding" missing
#  or invalid values. Other examples include tagging the source of your data,
#  or recording when you ingested / converted data into the ioda format.
# 
#  Basic manipulation of attributes was already discussed in Tutorial 2. Now, we want to
#  focus instead on good practices with tagging your data.
# 
#  Supplementing attributes, we introduce the concept of adding "dimension scales" to your
#  data. Basically, your data have dimensions, but we want to attach a "meaning" to each
#  axis of your data. Typically, the first axis corresponds to your data's Location.
#  A possible second axis for brightness temperature data might be "instrument channel", or
#  maybe "pressure level". This tutorial will show you how to create new dimension scales and
#  attach them to new Variables.

import os
import sys

import ioda

g = ioda.Engines.HH.createFile(
    name = "Example-04-python.hdf5",
    mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)

#  Let's start with dimensions and Dimension Scales.
#
#  ioda stores data using Variables, and you can view each variable as a
#  multidimensional matrix of data. This matrix has dimensions.
#  A dimension may be used to represent a real physical dimension, for example,
#  time, latitude, longitude, or height. A dimension might also be used to
#  index more abstract quantities, for example, color-table entry number,
#  instrument number, station-time pair, or model-run ID
# 
#  A dimension scale is simply another variable that provides context, or meaning,
#  to a particular dimension. For example, you might have ATMS brightness
#  temperature information that has dimensions of location by channel number. In ioda,
#  we want every "axis" of a variable to be associated with an explanatory dimension.
#
#  Let's create a few dimensions... Note: when working with an already-existing Obs Space,
#  (covered later), these dimensions may already be present.
#
#  Create two dimensions, "nlocs", and "ATMS Channel". Set distinct values within
#  these dimensions.

num_locs = 3000
num_channels = 23

dim_location = g.vars.create('nlocs', ioda.Types.int32, [ num_locs ])
dim_location.scales.setIsScale('nlocs')

dim_channel = g.vars.create('ATMS Channel', ioda.Types.int32, [num_channels])
dim_channel.scales.setIsScale('ATMS Channel')

#  Now that we have created dimensions, we can create new variables and attach the
#  dimensions to our data.
#
#  But first, a note about attributes:
#  Attributes provide metadata that describe our variables.
#  In IODA, we at least must to keep track of each variable's:
#  - Units (in SI; we follow CF conventions)
#  - Long name
#  - Range of validity. Data outside of this range are automatically rejected for
#    future processing.
#
#  Let's create variables for Latitude, Longitude and for
#  ATMS Observed Brightness Temperature.
#
#  There are two ways to define a variable that has attached dimensions.
#  First, we can explicitly create a variable and set its dimensions.
#
#  Longitude has dimensions of nlocs. It has units of degrees_east, and has
#  a valid_range of (-180,180).

longitude = g.vars.create('MetaData/Longitude', ioda.Types.float, [num_locs])
longitude.scales.set([dim_location])
longitude.atts.create('valid_range', ioda.Types.float, [2]).writeVector.float([-180, 180])
longitude.atts.create('units', ioda.Types.str).writeVector.str(['degrees_east'])
longitude.atts.create('long_name', ioda.Types.str).writeVector.str(['Longitude'])

#  The above method is a bit clunky because you have to make sure that the new variable's
#  dimensions match the sizes of each dimension.
# Here, we do the same variable creation, but instead use dimension scales to determine sizes.
latitude = g.vars.create('MetaData/Latitude', ioda.Types.float, scales=[dim_location])
# Latitude has units of degrees_north, and a valid_range of (-90,90).
latitude.atts.create('valid_range', ioda.Types.float, [2]).writeVector.float([-90,90])
latitude.atts.create('units', ioda.Types.str).writeVector.str(['degrees_north'])
latitude.atts.create('long_name', ioda.Types.str).writeVector.str(['Latitude'])

# The ATMS Brightness Temperature depends on both location and instrument channel number.
tb = g.vars.create('ObsValue/Brightness_Temperature', ioda.Types.float, scales=[dim_location, dim_channel])
tb.atts.create('valid_range', ioda.Types.float, [2]).writeVector.float([100,500])
tb.atts.create('units', ioda.Types.str).writeVector.str(['K'])
tb.atts.create('long_name', ioda.Types.str).writeVector.str(['ATMS Observed (Uncorrected) Brightness Temperature'])


#  Variable Parameter Packs
#  When creating variables, you can also provide an optional
#  VariableCreationParameters structure. This struct lets you specify
#  the variable's fill value (a default value that is a placeholder for unwritten data).
#  It also lets you specify whether you want to compress the data stored in the variable,
#  and how you want to store the variable (contiguously or in chunks).
p1 = ioda.VariableCreationParameters()

#  Variable storage: contiguous or chunked
#  See https://support.hdfgroup.org/HDF5/doc/Advanced/Chunking/ and
#  https://www.unidata.ucar.edu/blogs/developer/en/entry/chunking_data_why_it_matters
#  for detailed explanations.
#  To tell ioda that you want to chunk a variable:
p1.chunk = True
p1.chunks = [100]

#  Fill values:
#  The "fill value" for a dataset is the specification of the default value assigned
#  to data elements that have not yet been written.
#  When you first create a variable, it is empty. If you read in a part of a variable that
#  you have not filled with data, then ioda has to return fake, filler data.
#  A fill value is a "bit pattern" that tells ioda what to return.
p1.setFillValue.float(-999)

#  Compression
#  If you are using chunked storage, you can tell ioda that you want to compress
#  the data using ZLIB or SZIP.
#  - ZLIB / GZIP:
p1.compressWithGZIP()


#  Let's create one final variable, "Solar Zenith Angle", and let's use
#  out new variable creation parameters.
sza = g.vars.create(name='ObsValue/Solar Zenith Angle', dtype=ioda.Types.float, scales=[dim_location], params=p1)


