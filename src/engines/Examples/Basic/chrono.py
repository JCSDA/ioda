#
# (C) Copyright 2022 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This example shows how to read and write Python datetime
# objects into a IODA file. This file is a rough translation
# of the chrono.cpp example.

import datetime
import sys

from pyioda import ioda
import numpy as np

f = ioda.Engines.HH.createFile(
    name = "chrono-python.hdf5",
    mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)

# Create an epoch, Jan 1, 1970 0Z
epoch = datetime.datetime(1970, 1, 1, 0, 0, 0, tzinfo=datetime.timezone.utc)

# Create a fill value (absolute datetime), Jan 1, 2200 0Z
fillVal = datetime.datetime(2200, 1, 1, 0, 0, 0, tzinfo=datetime.timezone.utc)

# IODA does not yet support microseconds or nanoseconds.
# The Python / C++ conversions also drop time zone information.
# However, everything is assumed to be UTC.
# https://pybind11.readthedocs.io/en/stable/advanced/cast/chrono.html#provided-conversions
dtNow = datetime.datetime.now(tz=datetime.timezone.utc).replace(microsecond=0)
times = [ dtNow, fillVal ]

# Create the variable and write the data
params = ioda.VariableCreationParameters()
params.setFillValue.duration(fillVal - epoch)
var = f.vars.create(name="now", dtype=ioda.Types.datetime, dimsCur=[2], dimsMax=[2], params=params)

## Before writing the data, we need to set the epoch by setting the "units" attribute.
## The units attribute needs to be a fixed-length string for NetCDF.
epochString = epoch.strftime("%Y-%m-%dT%H:%M:%SZ")
units = "seconds since " + epochString
units_fixed_type = f.vars.getTypeProvider().makeStringType(stringLength = len(units))
var.atts.create("units", units_fixed_type, [1]).writeDatum.str(units)
## Write the data
var.writeVector.datetime(times)

# Read and check the data
read_times = var.readVector.datetime()
read_times_utc = list(map(lambda toutc: toutc.replace(tzinfo=datetime.timezone.utc), read_times))

for i, j in zip(times, read_times_utc):
    if (i != j):
        raise Exception("Mismatched values", i, j)

