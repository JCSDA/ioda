#
# (C) Copyright 2020-2021 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This example opens and reads a sample ioda output file (v2 format).

import os
import sys
import numpy as np

import ioda

# grab arguments
print(sys.argv)
InFile = sys.argv[1]

# First task is to open the ioda file for reading. This is accomplished by constructing
# an ObsGroup object. This is done in two steps:
#   1. create a "backend" which is based on a file (ioda.Engines.HH.openFile call)
#   2. construct an ObsGroup based on the backend created in step 1.
#
g = ioda.Engines.HH.openFile(
    name = InFile,
    mode = ioda.Engines.BackendOpenModes.Read_Only)
og = ioda.ObsGroup(g)

# The radiance data is organized as a 2D array (locations X channels). The dimension
# names are "Location" and "Channel" for locataion and channels respectively.

# You can access the dimension variables to get coordinate values
locsDimName = "Location"
chansDimName = "Channel"

locsDimVar = og.vars.open(locsDimName)
chansDimVar = og.vars.open(chansDimName)

locsCoords = locsDimVar.readVector.int()
chansCoords = chansDimVar.readVector.int()

numLocs = len(locsCoords)
numChans = len(chansCoords)

print("INFO: locations dimension: ", locsDimName, " (", numLocs, ")")
print("INFO:     coordinates: ")
for i in range(numLocs):
    print("INFO:        ", i, " --> ", locsCoords[i])
print("")

print("INFO: channels dimension: ", chansDimName, " (", numChans, ")")
print("INFO:     coordinates: ")
for i in range(numChans):
    print("INFO:        ", i, " --> ", chansCoords[i])
print("")

# We are interested in the following variables for diagnostics:
#      ObsValue/brightnessTemperature --> y
#      HofX/brightnessTemperature --> H(x)
#      MetaData/latitude
#      MeatData/lognitude

tbName = "ObsValue/brightnessTemperature"
hofxName = "HofX/brightnessTemperature"
latName = "MetaData/latitude"
lonName = "MetaData/longitude"

tbVar = og.vars.open(tbName)
hofxVar = og.vars.open(hofxName)
latVar = og.vars.open(latName)
lonVar = og.vars.open(lonName)

tbData = tbVar.readNPArray.float()       # produces a numpy 2D array
hofxData = hofxVar.readNPArray.float()
latData = latVar.readVector.float()      # produces a python list
lonData = lonVar.readVector.float()

print("INFO: input Tb variable: ", tbName, " (", tbData.shape, ")")
print("INFO: output Tb H(x) variable: ", hofxName, " (", hofxData.shape, ")")
print("INFO: latitude variable: ", latName, " (", len(latData), ")")
print("INFO: longitude variable: ", lonName, " (", len(lonData), ")")
print("")

# Grab channel 7 data from Tb and H(x) data
chanIndex = chansCoords.index(7)

tbDataCh7 = tbData[:, chanIndex]
hofxDataCh7 = hofxData[:, chanIndex]

print("INFO: Channel 7 is located at channel index: ", chanIndex)
print("INFO: input Tb variable, channel 7: ", tbName, " (", tbDataCh7.shape, ")")
print("INFO: output Tb H(x) variable, channel 7: ", hofxName, " (", hofxDataCh7.shape, ")")
print("")

