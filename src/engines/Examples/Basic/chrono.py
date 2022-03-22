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

import ioda
import numpy as np

f = ioda.Engines.HH.createFile(
    name = "chrono-python.hdf5",
    mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)

# IODA does not yet support microseconds or nanoseconds.
# The Python / C++ conversions also drop time zone information.
# However, everything is assumed to be UTC.
# https://pybind11.readthedocs.io/en/stable/advanced/cast/chrono.html#provided-conversions
times = [ datetime.datetime.now(tz=datetime.timezone.utc).replace(microsecond=0) ]

# Create the variable and write the data
var = f.vars.create("now", ioda.Types.datetime, [1])

## Before writing the data, we need to set the epoch by setting the "units" attribute.
## The units attribute needs to be a fixed-length string for NetCDF.
units = "seconds since 1970-01-01T00:00:00Z"
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

