#
# (C) Copyright 2020-2021 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This example plots the outputs of Example 5a using Cartopy.
# For this to successfully run, you need Cartopy, Matplotlib and NumPy.

import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import cartopy.crs as ccrs

from pyioda import ioda

g = ioda.Engines.HH.openFile(
    name = "Example-05a-python.hdf5",
    mode = ioda.Engines.BackendOpenModes.Read_Only)
og = ioda.ObsGroup(g)

tbName = "ObsValue/brightnessTemperature"
latName = "MetaData/latitude"
lonName = "MetaData/longitude"

tbVar = og.vars.open(tbName)
latVar = og.vars.open(latName)
lonVar = og.vars.open(lonName)

tbData = tbVar.readNPArray.float()
latData = latVar.readVector.float()
lonData = lonVar.readVector.float()

#fig = plt.figure(figsize=(10, 5))
ax = plt.axes(projection=ccrs.PlateCarree())
#ax = fig.add_subplot(1, 1, 1, projection=ccrs.PlateCarree())
ax.set_extent([-20, 40, -20, 45], crs=ccrs.PlateCarree())
ax.stock_img()
ax.scatter(lonData, latData, c=tbData[:,0], transform=ccrs.PlateCarree(), cmap='nipy_spectral')
ax.coastlines()
plt.savefig('plotting-Example-05a-python.jpg') # Has to be called before show().
plt.show()

# Ignore this line. Used for uploads of test results to cdash.jcsda.org.
print('<DartMeasurementFile name=\"plotting-05-ObsGroup-example.jpg\" type=\"image/jpeg\">' + os.getcwd() + '/plotting-Example-05a-python.jpg</DartMeasurementFile>')
