#!/usr/bin/env python3

# (C) Crown Copyright 2021, Met Office
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

import argparse
from collections import namedtuple
import configparser
import re
import struct
import subprocess
import sys

###############################
# TYPES
###############################

# A test case specifying the expected contents of a list of NetCDF variables.
#
# Constructor arguments:
# vars: List of names of NetCDF variables.
# sql:  ODB SQL select statement that should produce the expected values of
#       the NetCDF variables `vars`.
# vars_should_not_exist: If present and set to true, it indicates that the
#       NetCDF variables `vars` should not exist.
Case = namedtuple("Case", ["vars", "sql", "vars_should_not_exist"])
Column = namedtuple("Column", ["name", "values"])
Variable = namedtuple("Variable", ["name", "values"])

###############################
# SUBROUTINES
###############################

def parse_config_file(path):
    """Read a configuration file containing a list of test cases.

    Arguments:
        path: Path to a configuration file.

    Returns: a list of Case objects representing the test cases.
    """
    cases = []

    config = configparser.ConfigParser()
    config.read(path)
    for section in config.sections():
        section_config = config[section]
        vars = section_config["netcdf variables"]
        vars = [var.strip() for var in vars.split(",")]
        vars_should_not_exist = section_config.getboolean("should not exist", fallback=False)
        if vars_should_not_exist:
            sql = None
            if "sql query" in section_config:
                raise Exception("'sql query' is incompatible with 'should not exist'")
        else:
            sql = section_config["sql query"].strip()
        cases.append(Case(vars, sql, vars_should_not_exist))
    return cases


def read_netcdf_file(path, ncdump_exe):
    """Read variables from a NetCDF file.

    Arguments:
        path       Path to a NetCDF file.
        ncdump_exe Path to the 'ncdump' executable.

    Returns: a dictionary mapping the name of each variable from the specified
    NetCDF file to its value.
    """

    netcdf = {}
    re_begin_group = re.compile(r"^\s*group: (\S+) {\s*", re.ASCII)
    re_data = re.compile(r"^\s*data:\s*", re.ASCII)
    re_variable_equals = re.compile(r"^\s*(\S+)\s*=\s*(.*)", re.ASCII)
    var_path = []
    var_value = []
    in_data = False
    in_variable = False

    command = [ncdump_exe, "-p", "9,17", path]
    output = subprocess.check_output(command, universal_newlines=True)

    for line in output.splitlines():
        # print(repr(line))
        if in_variable:
            if not continue_assigning_var_value(line, var_value):
                netcdf["/".join(var_path)] = var_value
                var_path.pop()
                var_value = []
                in_variable = False
            continue
        if "{" in line:
            in_data = False
            match = re_begin_group.match(line)
            if match:
                var_path.append(match.group(1))
            continue
        if "}" in line:
            in_data = False
            if var_path:
                var_path.pop()
            continue
        if not in_data:
            if re_data.match(line):
                in_data = True
            continue
        match = re_variable_equals.match(line)
        if match:
            in_variable = True
            var_path.append(match.group(1))
            if not continue_assigning_var_value(match.group(2), var_value):
                netcdf["/".join(var_path)] = var_value
                var_path.pop()
                var_value = []
                in_variable = False

    return netcdf


# Returns False if this was the last line in the variable assignment, True otherwise
def continue_assigning_var_value(line, var_value):
    last_element = line.endswith(";")
    if last_element:
        line = line[:-1]
    for item in line.split(","):  # Assume there aren't any strings with embedded commas...
        item = item.strip()
        if item == "":
            continue
        if item.startswith('"'):
            assert (item.endswith('"'))
            var_value.append(item[1:-1])
        elif item == "_":  # Missing value
            var_value.append(None)
        else:
            var_value.append(convert_to_int_or_single_precision_float(item))
    return not last_element


def execute_sql(sql, odb_file, odc_exe):
    """Execute an ODB SQL query.

    Arguments:
        sql:      The query to execute.
        odb_file: Path to an ODB file.
        odc_exe:  Path to the 'odc' executable.

    Returns: a list of Column objects containing the names and values of
             columns produced by the query.
    """
    try:
        output = subprocess.check_output([odc_exe, "sql", sql, "-i", odb_file, "--full_precision"],
                                         universal_newlines=True)
        return parse_odc_sql_output(output)
    except subprocess.CalledProcessError:
        print("The SQL query '%s' failed" % sql)
        return None


def parse_odc_sql_output(output):
    lines = output.splitlines()
    assert(len(lines) > 0)
    column_names = [s.strip() for s in lines[0].split("\t")]
    columns = [[] for i in range(len(column_names))]
    for line in lines[1:]:
        row = [s.strip() for s in line.split("\t")]
        for column, cell in zip(columns, row):
            if cell.startswith("'"):
                assert(cell.endswith("'"))
                column.append(cell[1:-1])
            elif cell == "NULL":
                column.append(None)
            else:
                # Unfortunately ODC prints floating-point values in the "fixed" format, which means
                # depending on their magnitude they have a varying number of significant digits.
                # As a mitigation, we convert them to single precision
                column.append(convert_to_int_or_single_precision_float(cell))
    return [Column(n, c) for n, c in zip(column_names, columns)]


def get_vars(netcdf, vars):
    result = []
    for var in vars:
        if var in netcdf:
            result.append(Variable(var, netcdf[var]))
        else:
            result.append(Variable(var, None))
    return result


def vars_do_not_exist(netcdf, vars):
    result = True
    for var in vars:
        if var in netcdf:
            print("The NetCDF file contains the unexpected variable '%s'" % var)
            result = False
    return result


def all_vars_match_columns(netcdf_vars, odc_columns, sql):
    if len(netcdf_vars) != len(odc_columns):
        print("The number of specified NetCDF variables (%s) does not match the number of columns (%s) "
              "produced by the ODC query '%s'" % (len(netcdf_vars), len(odc_columns), sql))
        return False
    result = True
    for var, column in zip(netcdf_vars, odc_columns):
        if not var_matches_column(var, column):
            result = False
    return result


def var_matches_column(netcdf_var, odc_column):
    if netcdf_var.values is None:
        print("The NetCDF file does not contain variable '%s'" % netcdf_var.name)
        return False
    if len(netcdf_var.values) != len(odc_column.values):
        print("The NetCDF variable '%s' has a different length (%s) than the corresponding ODB column '%s' (%s)" %
              (netcdf_var.name, len(netcdf_var.values), odc_column.name, len(odc_column.values)))
        return False
    differences_found = False
    for i in range(len(netcdf_var.values)):
        if netcdf_var.values[i] != odc_column.values[i]:
            if not differences_found:
                print("Differences between NetCDF variable '%s' and ODB column '%s':" %
                      (netcdf_var.name, odc_column.name))
                differences_found = True
            print("  Element %s: %s != %s" % (i, netcdf_var.values[i], odc_column.values[i]))
    return not differences_found


def convert_to_int_or_single_precision_float(x):
    x = float(x)
    if x == int(x):
        return int(x)
    else:
        return truncate_to_single_precision(x)


def truncate_to_single_precision(x):
    return struct.unpack('f', struct.pack('f', x))[0]


###############################
# MAIN
###############################

# Parse command line
ap = argparse.ArgumentParser(description="Compare NetCDF variables to results of ODB SQL queries")
ap.add_argument("config_file", help="Path to a file listing pairs of SQL statements extracting columns from an ODB "
                                    "file and lists of paths to NetCDF variables whose contents should match these "
                                    "columns")
ap.add_argument("odb_file", help="Path to the ODB file")
ap.add_argument("nc_file", help="Path to the NetCDF file")
ap.add_argument("--odc", default="odc", help="Path to the ODC executable")
ap.add_argument("--ncdump", default="ncdump", help="Path to the NCDUMP executable")

args = ap.parse_args()

# Perform requested work
success = True

cases = parse_config_file(args.config_file)
netcdf = read_netcdf_file(args.nc_file, args.ncdump)
for case in cases:
    if case.vars_should_not_exist:
        if not vars_do_not_exist(netcdf, case.vars):
            success = False
    else:
        odc_columns = execute_sql(case.sql, args.odb_file, args.odc)
        if odc_columns:
            netcdf_vars = get_vars(netcdf, case.vars)
            if not all_vars_match_columns(netcdf_vars, odc_columns, case.sql):
                success = False
        else:
            success = False

sys.exit(0 if success else 1)
