## Needed for now because of how pybind11 calls localtime.
## See https://github.com/pybind/pybind11/blob/master/include/pybind11/chrono.h#L120
import os
import time
os.environ['TZ'] = 'UTC'
time.tzset()

from ._ioda_python import *
