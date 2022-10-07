#!/usr/bin/env python3

import argparse
import re

# Function to return the number of spaces in the indentation
def check_indentation(string):
    count = 0
    commented = False
    for i in range(0, len(string)):
        if string[i] == " ":
            count += 1
        elif string[i] == "#":
            count += 1
            commented = True
        else:
            break
    return count, commented


# Function to do the reformatting of obsdatain/generate/obsdataout for the HDF5 file case
# Arguments:
#   'line' current input file line
def reformatObsDataIn(odi_lines):
    if debug:
        print('DEBUG: reformatObsDataIn: ', odi_lines)
    engineType = 'H5File'
    indx = 0
    comment = False
    for odi_line in odi_lines:
        if 'obsdatain:' in odi_line:
            indx, comment = check_indentation(odi_line)
        if 'obsfile:' in odi_line:
            obsfile = odi_line.split('obsfile:')[1].strip()
            # strip off braces
            obsfile = re.sub('[{}]', '', obsfile)
            if '.odb' in odi_line:
                engineType = 'ODB'

    indentation = ' '*indx
    if comment: indentation = '#' + indentation[1:]
    fileout.write(indentation + 'obsdatain:\n')
    fileout.write(indentation + '  engine:\n')
    fileout.write(indentation + f"    type: {engineType}\n")
    fileout.write(indentation + f"    obsfile: {obsfile}\n")
    for odi_line in odi_lines:
        if ('obsdatain:' not in odi_line) and ('obsfile' not in odi_line):
            # redo the indentation
            fileout.write(indentation + '    ' + odi_line.lstrip())

def reformatObsDataOut(odo_lines):
    if debug:
        print('DEBUG: reformatObsDataOut: ', odo_lines)
    engineType = 'H5File'
    indx = 0
    comment = False
    for odo_line in odo_lines:
        if 'obsdataout:' in odo_line:
            indx, comment = check_indentation(odo_line)
        if 'obsfile:' in odo_line:
            obsfile = odo_line.split('obsfile:')[1].strip()
            # strip off braces
            obsfile = re.sub('[{}]', '', obsfile)
            if '.odb' in odo_line:
                engineType = 'ODB'

    indentation = ' '*indx
    if comment: indentation = '#' + indentation[1:]
    fileout.write(indentation + 'obsdataout:\n')
    fileout.write(indentation + '  engine:\n')
    fileout.write(indentation + f"    type: {engineType}\n")
    fileout.write(indentation + f"    obsfile: {obsfile}\n")
    for odo_line in odo_lines:
        if ('obsdataout:' not in odo_line) and ('obsfile' not in odo_line):
            # redo the indentation
            fileout.write(indentation + '    ' + odo_line.lstrip())

def reformatGenerate(gen_lines):
    # assume generate list for now
    if debug:
        print('DEBUG: reformatGenerate: ', gen_lines)
    engineType = 'GenList'
    for gen_line in gen_lines:
        if 'generate:' in gen_line:
            indx, comment = check_indentation(gen_line)
        if 'random:' in gen_line:
            engineType = 'GenRandom'
    indentation = ' '*indx
    fileout.write(indentation + 'obsdatain:\n')
    fileout.write(indentation + '  engine:\n')
    fileout.write(indentation + f"    type: {engineType}\n")
    for gen_line in gen_lines:
        if ('generate:' not in gen_line) and ('list:' not in gen_line) and ('random:' not in gen_line):
            # redo the indentation
            if ('obsgrouping' in gen_line):
                fileout.write(indentation + '  ' + gen_line.lstrip())
            else:
                fileout.write(indentation + '    ' + gen_line.lstrip())

####################################### MAIN ###############################
# Grab arguments from command line. Simple usage with one input and one output file.
# Ie, this script converts one file, and if you need to convert a large set this script
# can be called from another that finds all of the files to be converted.
desc = 'Reformat obsdatain, generate and obsdataout sections of obs space YAML to new "engine" format'
parser = argparse.ArgumentParser(description=desc,
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument(
    '-i', '--input', help='path to input YAML file', type=str, required=True, default=None)
parser.add_argument(
    '-o', '--output', help='path to output YAML file', type=str, required=True, default=None)
parser.add_argument(
    '-d', '--debug', help='enable debug print statements', action='store_true', required=False)

args = parser.parse_args()

inFile = args.input
outFile = args.output
debug = args.debug

print(f"processing ... {inFile} -> {outFile}")
with open(outFile, 'w') as fileout:
    with open(inFile) as filein:
        obsdatain_section = False
        obsdatain_indent = 0
        obsdataout_section = False
        obsdataout_indent = 0
        generate_section = False
        generate_indent = 0
        for line in filein:
            indent, commented = check_indentation(line)
            if debug:
                print(line.rstrip('\n'), " -> ", indent, commented)

            ############  Process the obsdatain section ############
            # This part detects the end of the section (when the indentation returns
            # to what it was when the obsdatain keyword was found)
            if (obsdatain_section) and (indent <= obsdatain_indent):
               obsdatain_section = False
               obsdatain_indent = 0
               reformatObsDataIn(obsdatain_lines)

            # This part detects the start of the section.
            if 'obsdatain:' in line:
                obsdatain_section = True
                obsdatain_indent = indent
                obsdatain_lines = [ ]

            ############  Process the obsdataout section ############
            # This part detects the end of the section (when the indentation returns
            # to what it was when the obsdataout keyword was found)
            if (obsdataout_section) and (indent <= obsdataout_indent):
               obsdataout_section = False
               obsdataout_indent = 0
               reformatObsDataOut(obsdataout_lines)

            # This part detects the start of the section.
            if 'obsdataout:' in line:
                obsdataout_section = True
                obsdataout_indent = indent
                obsdataout_lines = [ ]

            ############  Process the generate section ############
            # This part detects the end of the section (when the indentation returns
            # to what it was when the generate keyword was found)
            if (generate_section) and (indent <= generate_indent):
               generate_section = False
               generate_indent = 0
               reformatGenerate(generate_lines)

            # This part detects the start of the section.
            if 'generate:' in line:
                generate_section = True
                generate_indent = indent
                generate_lines = [ ]

            # Collect lines if in a section we care about, otherwise write the line
            # to the output file.
            if obsdatain_section:
                obsdatain_lines.append(line)
            elif obsdataout_section:
                obsdataout_lines.append(line)
            elif generate_section:
                generate_lines.append(line)
            else:
                fileout.write(line)

print('done')

