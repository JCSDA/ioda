# (C) Copyright 2021 NOAA NWS NCEP EMC
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

# This example creates a file with various attribute and variable data types,
# then reads those objects back in from the file that was just written. Ie, the
# attribute/variable read/write functions are exercised with different data types.
# All of this is done via the ObsSpace class.

import os
import sys
import numpy as np
import datetime

if os.environ.get('LIBDIR') is not None:
    sys.path.append(os.environ['LIBDIR'])

import ioda_obs_space as ioda_ospace

testFile = "Example-07b-python.hdf5"

intAttrName = "int_attr"
intAttrValue = np.array([1,], dtype=np.int32)
longAttrName = "long_attr"
longAttrValue = np.array([2,], dtype=np.int64)
floatAttrName = "float_attr"
floatAttrValue = np.array([1.5,], dtype=np.float32)
doubleAttrName = "double_attr"
doubleAttrValue = np.array([2.5,], dtype=np.float64)
stringAttrName = "string_attr"
stringAttrValue = "Hello World"

# Open up the test file for writing
DimDict = { "Location": 5 }
obsspace = ioda_ospace.ObsSpace(testFile, mode='w', dim_dict=DimDict)

# Write some global attributes
obsspace.write_attr(intAttrName, intAttrValue)
obsspace.write_attr(longAttrName, longAttrValue)
obsspace.write_attr(floatAttrName, floatAttrValue)
obsspace.write_attr(doubleAttrName, doubleAttrValue)
obsspace.write_attr(stringAttrName, stringAttrValue)

# Read the global attributes back and check them
testIntAttrValue = obsspace.read_attr(intAttrName)
testLongAttrValue = obsspace.read_attr(longAttrName)
testFloatAttrValue = obsspace.read_attr(floatAttrName)
testDoubleAttrValue = obsspace.read_attr(doubleAttrName)
testStringAttrValue = obsspace.read_attr(stringAttrName)

if (testIntAttrValue != intAttrValue):
    errMsg =  "Global attribute " + intAttrName + " did not match expected value\n"
    errMsg += "    test value: " + str(testIntAttrValue) + "\n"
    errMsg += "    expected value: " + str(intAttrValue[0])
    raise Exception(errMsg)
if (testLongAttrValue != longAttrValue):
    errMsg =  "Global attribute " + longAttrName + " did not match expected value\n"
    errMsg += "    test value: " + str(testLongAttrValue) + "\n"
    errMsg += "    expected value: " + str(longAttrValue[0])
    raise Exception(errMsg)
if (testFloatAttrValue != floatAttrValue):
    errMsg =  "Global attribute " + floatAttrName + " did not match expected value\n"
    errMsg += "    test value: " + str(testFloatAttrValue) + "\n"
    errMsg += "    expected value: " + str(floatAttrValue[0])
    raise Exception(errMsg)
if (testDoubleAttrValue != doubleAttrValue):
    errMsg =  "Global attribute " + doubleAttrName + " did not match expected value\n"
    errMsg += "    test value: " + str(testDoubleAttrValue) + "\n"
    errMsg += "    expected value: " + str(doubleAttrValue[0])
    raise Exception(errMsg)
if (testStringAttrValue != stringAttrValue):
    errMsg =  "Global attribute " + stringAttrName + " did not match expected value\n"
    errMsg += "    test value: '" + str(testStringAttrValue) + "'\n"
    errMsg += "    expected value: '" + str(stringAttrValue) + "'"
    raise Exception(errMsg)

# write some variables
VarDims = [ "Location" ]
VarFillValue = -999

intVarName = "TestGroup/intVar"
intVarValues = np.array([1, 2, 3, 4, 5], dtype=np.int32)
longVarName = "TestGroup/longVar"
longVarValues = np.array([2, 4, 6, 8, 10], dtype=np.int32)
floatVarName = "TestGroup/floatVar"
floatVarValues = np.array([1.5, 2.5, 3.5, 4.5, 5.5], dtype=np.float32)
doubleVarName = "TestGroup/doubleVar"
doubleVarValues = np.array([2.5, 4.5, 6.5, 8.5, 10.5], dtype=np.float64)
stringVarName = "TestGroup/stringVar"
stringVarValues = np.array(["a", "b", "c", "d", "e"], dtype=object)

# IODA does not yet support microseconds or nanoseconds.
# The Python / C++ conversions also drop time zone information.
# However, everything is assumed to be UTC.
# https://pybind11.readthedocs.io/en/stable/advanced/cast/chrono.html#provided-conversions
refDt = datetime.datetime(1970, 1, 1, 0, 0, 0, tzinfo=datetime.timezone.utc)
refDt.replace(microsecond=0)
dtimeVarName = "MetaData/dateTime"
dtimeVarValues = np.array([
    refDt,
    refDt + datetime.timedelta(hours=1),
    refDt + datetime.timedelta(hours=2),
    refDt + datetime.timedelta(hours=3),
    refDt + datetime.timedelta(hours=4) ])

obsspace.create_var(intVarName, dtype=np.int32, dim_list=VarDims, fillval=VarFillValue)
intVar = obsspace.Variable(intVarName)
intVar.write_data(intVarValues)
obsspace.create_var(longVarName, dtype=np.int64, dim_list=VarDims, fillval=VarFillValue)
longVar = obsspace.Variable(longVarName)
longVar.write_data(longVarValues)
obsspace.create_var(floatVarName, dtype=np.float32, dim_list=VarDims, fillval=VarFillValue)
floatVar = obsspace.Variable(floatVarName)
floatVar.write_data(floatVarValues)
obsspace.create_var(doubleVarName, dtype=np.float64, dim_list=VarDims, fillval=VarFillValue)
doubleVar = obsspace.Variable(doubleVarName)
doubleVar.write_data(doubleVarValues)
obsspace.create_var(stringVarName, dtype=object, dim_list=VarDims, fillval="")
stringVar = obsspace.Variable(stringVarName)
stringVar.write_data(stringVarValues)
obsspace.create_var(dtimeVarName, dtype=object, dim_list=VarDims, fillval="")
dtimeVar = obsspace.Variable(dtimeVarName)
dtimeVar.write_data(dtimeVarValues)

# Read the variables back in and check values

testIntVarValues = intVar.read_data()
testLongVarValues = longVar.read_data()
testFloatVarValues = floatVar.read_data()
testDoubleVarValues = doubleVar.read_data()
testStringVarValues = stringVar.read_data()
testDtimeVarValues = dtimeVar.read_data()

if (not np.array_equal(testIntVarValues, intVarValues)):
    errMsg =  "Variable " + intVarName + " did not match expected value\n"
    errMsg += "    test value: " + str(testIntVarValues) + "\n"
    errMsg += "    expected value: " + str(intVarValues)
    raise Exception(errMsg)

if (not np.array_equal(testLongVarValues, longVarValues)):
    errMsg =  "Variable " + longVarName + " did not match expected value\n"
    errMsg += "    test value: " + str(testLongVarValues) + "\n"
    errMsg += "    expected value: " + str(longVarValues)
    raise Exception(errMsg)

if (not np.allclose(testFloatVarValues, floatVarValues)):
    errMsg =  "Variable " + floatVarName + " did not match expected value\n"
    errMsg += "    test value: " + str(testFloatVarValues) + "\n"
    errMsg += "    expected value: " + str(floatVarValues)
    raise Exception(errMsg)

if (not np.allclose(testDoubleVarValues, doubleVarValues)):
    errMsg =  "Variable " + doubleVarName + " did not match expected value\n"
    errMsg += "    test value: " + str(testDoubleVarValues) + "\n"
    errMsg += "    expected value: " + str(doubleVarValues)
    raise Exception(errMsg)

if (not np.array_equal(testStringVarValues, stringVarValues)):
    errMsg =  "Variable " + stringVarName + " did not match expected value\n"
    errMsg += "    test value: " + str(testStringVarValues) + "\n"
    errMsg += "    expected value: " + str(stringVarValues)
    raise Exception(errMsg)

if (not np.array_equal(testDtimeVarValues, dtimeVarValues)):
    errMsg =  "Variable " + dtimeVarName + " did not match expected value\n"
    errMsg += "    test value: " + str(testDtimeVarValues) + "\n"
    errMsg += "    expected value: " + str(dtimeVarValues)
    raise Exception(errMsg)

