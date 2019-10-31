#!/usr/bin/env python

#
# (C) Copyright 2019 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
#

from __future__ import print_function
import sys
import os
import glob
import argparse
import numpy as np
import netCDF4 as nc

###############################
# SUBROUTINES
###############################

################################################################
# This routine checks the values associated with Var for:
#    1) proper usage of missing marks
#    2) existence of invalid numerical values
#
def CheckVarValues(Var):
    BadMissingVals = False;
    HasInvalidVals = False;

    # Only do the check for numeric types (skip strings for example)
    if (np.issubdtype(Var.dtype, np.number)):
        VarData = Var[:].data.flatten() # no need to preserve array shape
        VarMask = Var[:].mask.flatten()
        if (VarMask.size == 1):
            VarMask = np.full(VarData.shape, VarMask)
        for i in range(Var.shape[0]):
            # Check for invalid numeric values (nan, inf and -inf)
            # first, since these will trigger the bad missing values
            # check.
            if ((np.isnan(VarData[i])) or (np.isinf(VarData[i]))):
                HasInvalidVals = True
            else:
                # Valid numeric data.
                #
                # The indices that the mask (VarMask[i]) is True are the
                # locations that the netcdf fill value was used, don't
                # check these locations. Of the other locations, if the
                # absolute value of the data is > 1e8, then flag this
                # as an incorrect missing value mark.
                if ((not VarMask[i]) and (np.fabs(VarData[i]) > 1e8)):
                    BadMissingVals = True

    return (BadMissingVals, HasInvalidVals)

############################################################
# This routine will walk through the list of files from
# the command line arguments and create a list of netcdf
# files to check. The items can be either files or directories.
# If a directory, find all the netcdf files under that directory
# and add them to the list.
def GenNcFileList(ArgList):
    # Expand the argument list into all files
    TempFileList = [ ]
    for Item in ArgList:
        if (os.path.isdir(Item)):
            for RootPath, Dirs, Files in os.walk(Item):
                for File in Files:
                    TempFileList.append(os.path.join(RootPath, File))
        else:
            TempFileList.append(Item)

    # Select out only the netcdf files (suffix = '.nc' or '.nc4')
    NcFileList = [ ]
    for Item in TempFileList:
        if (Item.endswith('.nc') or (Item.endswith('.nc4'))):
            NcFileList.append(Item)

    return NcFileList

############################################################
# This routine will check the contents of one file for
# compliance with the ioda netcdf conventions. This routine
# will return an error count for each of the three categories
# outlined below.
#
def CheckNcFile(NcFileName, Verbose):
    # Open the netcdf file and check for three conventions:
    #   1. Variable data type
    #        - No doubles
    #        - Variables in the group PreQC are integer
    #        - All other variables are allowed to remain what they
    #          are declared in the file (as long as they are not doubles)
    #
    #   2. Missing value marks
    #        - netcdf fill values are used for missing values
    #        - phasing out the use of large absolute values numbers
    #
    #   3. Invalid numerical values (inf, -inf, nan)
    #        - Don't use these in the input file.

    NcRootGroup = nc.Dataset(NcFileName, 'r')
    print("Checking netcdf file for ioda conventions: ")
    print("  {0:s}".format(NcFileName))

    # Walk through the variables (assume they all live in the root group)
    # and check the data types, etc.
    MissingGroupMsg = "  Variable: {0:s} " + \
                      "needs to specify a group name (@<group_name> suffix)"
    DataTypeMsg = "  Variable: {0:s} " + \
                  "has unexpected data type " + \
                  "({1} instead of {2})"
    MissingValMsg = "  Variable: {0:s} " + \
                    "needs to use netcdf fill values for missing marks"
    InvalidNumMsg = "  Variable: {0:s} " + \
                    "needs to remove invalid numeric values " + \
                    "(nan, inf, -inf)"

    MissingGroupErrors = 0
    DataTypeErrors = 0
    MissingValErrors = 0
    InvalidNumErrors = 0
    for Vname in NcRootGroup.variables:
        Var = NcRootGroup.variables[Vname]
        (VarName, Dummy, GroupName) = Vname.partition('@')

        # Check that the group name is defined.
        if (GroupName == ""):
            if (Verbose):
                print(MissingGroupMsg.format(Vname))
            MissingGroupErrors += 1

        # Check the variable data type (VarType)
        # Expected var type matches what is in the file, except when
        # GroupName is PreQC which is int32, and float64 is not allowed.
        VarType = Var.dtype.name
        if (GroupName == "PreQC"):
            ExpectedVarType = "int32"
        elif (VarType == "float64"):
            ExpectedVarType = "float32"
        else:
            ExpectedVarType = VarType

        if (VarType != ExpectedVarType):
            if (Verbose):
                print(DataTypeMsg.format(Vname, VarType, ExpectedVarType))
            DataTypeErrors += 1

        # Check the variable values for incorrect missing marks, and
        # for invalid numeric values.
        (BadMissingVals, HasInvalidVals) = CheckVarValues(Var)
        if (BadMissingVals):
            if (Verbose):
                print(MissingValMsg.format(Vname))
            MissingValErrors += 1

        if (HasInvalidVals):
            if (Verbose):
                print(InvalidNumMsg.format(Vname))
            InvalidNumErrors += 1

    TotalErrors = MissingGroupErrors + DataTypeErrors + MissingValErrors + InvalidNumErrors
    ErrorReport = "  Error counts: Missing group: {0:d}, " + \
                  "data type: {1:d}, " + \
                  "missing value: {2:d}, " + \
                  "invalid numeric values: {3:d}"
    print(ErrorReport.format(MissingGroupErrors, DataTypeErrors, MissingValErrors, InvalidNumErrors), end=' ')
    if (TotalErrors == 0):
        print("--> PASS")
    else:
        print("--> FAIL")
    print("")

    return (TotalErrors)

###############################
# MAIN
###############################

ScriptName = os.path.basename(sys.argv[0])

# Parse command line
ap = argparse.ArgumentParser()
ap.add_argument("-v", "--verbose", action="store_true",
   help="increase verbosity")
ap.add_argument("nc_file_or_dir", nargs='+',
   help="list of files or directories containing netcdf")

MyArgs = ap.parse_args()

NetcdfList = MyArgs.nc_file_or_dir
Verbose = MyArgs.verbose

# Generate the total list of netcdf files to check. The items from
# the command line can be either files or directories, and if
# a directory then you find all the netcdf files within that directory.
NcFileList = GenNcFileList(NetcdfList)

# Check the files in the list, and accumulate the error counts.
TotalErrorCount = 0
for NcFileName in NcFileList:
    (TotalErrors) = CheckNcFile(NcFileName, Verbose)
    TotalErrorCount += TotalErrors

# Return the error count. If the all files are okay, then error count will
# be zero and the shell will see a zero return code from this script.
sys.exit(TotalErrorCount)
