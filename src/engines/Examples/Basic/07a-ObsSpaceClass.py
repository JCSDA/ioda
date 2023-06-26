#
# (C) Copyright 2021 NOAA NWS NCEP EMC
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This example opens and reads a sample ioda output file (v2 format) using the ObsSpace class.

import os
import sys
import numpy as np

if os.environ.get('LIBDIR') is not None:
    sys.path.append(os.environ['LIBDIR'])

from pyioda import ioda_obs_space as ioda_ospace

# grab arguments
print(sys.argv)
InFile = sys.argv[1]

# create an obsspace object
obsspace = ioda_ospace.ObsSpace(InFile, mode='r', name='IODA Test ObsSpace')

# print some info about this file
print(f"INFO: nlocs={obsspace.nlocs}")
print(f"INFO: list of dimensions:{obsspace.dimensions}")
print(f"INFO: all available variables in {obsspace}: {obsspace.variables}")

# read a sample variable from the file
testvar = obsspace.Variable('HofX/brightnessTemperature')

# grab the actual values for this variable
testdata = testvar.read_data()

# print some stats from the testdata
print(f"INFO: HofX/brightnessTemperature min: {np.nanmin(testdata)}")
print(f"INFO: HofX/brightnessTemperature max: {np.nanmax(testdata)}")
print(f"INFO: HofX/brightnessTemperature mean: {np.nanmean(testdata)}")

# read the times
timevar = obsspace.Variable('MetaData/dateTime')
timedata = timevar.read_data()

# print the first and last times
print(f"INFO: MetaData/dateTime first: {timedata[0].strftime('%Y-%m-%d %H:%M:%S UTC')}")
print(f"INFO: MetaData/dateTime last: {timedata[-1].strftime('%Y-%m-%d %H:%M:%S UTC')}")
