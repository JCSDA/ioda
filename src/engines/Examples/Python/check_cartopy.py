#
# (C) Copyright 2020 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This just checks that numPy, Matplotlib and Cartopy are available.
# If they are not, then the tests that depend on them will be skipped.

import numpy as np
import matplotlib.pyplot as plt
import cartopy.crs as ccrs
