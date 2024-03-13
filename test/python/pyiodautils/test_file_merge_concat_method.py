#!/usr/bin/env python3
import argparse
from pyiodautils import file_merge as fm

############
# MAIN
############

# Get the list of input files and the output file specs
ap = argparse.ArgumentParser(description="Test netcdf file merge using the contatenate method")
ap.add_argument("--outfile", help="Output netcdf file")
ap.add_argument("--infiles", nargs='+', help="List of input netcdf files to be concatenated")

args = ap.parse_args()
outFile = args.outfile
inFileList = args.infiles

# Run the fileMerge.concat_files numpy method for the test
print("INFO: Testing: file merge utility using the concatenate method:")
print("INFO:     input file list: ", inFileList)
print("INFO:     output file: ", outFile)

fileMerge = fm.fileMerge('concat test')
fileMerge.concat_files(inFileList, outFile)
