#!/usr/bin/env python3

# (C) Copyright 2026 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

r"""Compare two IODA obs files for equivalence, independent of row order.

Useful for comparing files produced by the OSDF container writer against those
produced by the legacy ObsGroup container.

Method: full-row canonical sort
  Each file is sorted by a key made of the ENTIRE observation record at each
  location -- every Location-indexed variable, a (Location, Channel) variable
  contributing one key column per channel -- then compared variable by variable.
  Because the key is the whole row, two rows can tie only if they are identical,
  so tie-break order cannot affect the result. The report prints the tie count
  so this can be checked.

  Location alone is held out of the key, being a row label rather than data, so a
  disagreement about it is reported on one line instead of scrambling the sort.

Normalization rules, each absorbing a deliberate difference between the writers.
Every variable becomes one of four kinds plus a missing mask:

  NC_STRING (str)      -> string  missing when == _FillValue or "MISSING*"
  NC_CHAR |S1 over a   -> string  trailing dim counts characters: join it into
    character dimension           one string per observation and drop it
  NC_CHAR |S1 over     -> bool    b'\x01' True, b'' and b'\x00' False;
    index dims only               never missing -- an OSDF boolean
  NC_CHAR |SN, N>1     -> string  fixed-length, decoded
  NC_UBYTE / NC_BYTE   -> bool    x != 0; never missing -- a legacy boolean
  NC_INT / NC_INT64    -> int     exact int64; missing when == _FillValue or a
                                  known sentinel, tested in the NATIVE dtype
  NC_FLOAT / NC_DOUBLE -> float   float64; missing when not finite,
                                  == _FillValue, or |v| >= 1e30

  An int on one side and a float on the other (legacy writes nrecs as float32,
  OSDF as int32) are compared as floats.

Three of those rules are load-bearing: done naively, each makes the tool report
PASS on files that differ.

  * Booleans ignore _FillValue.  The legacy writer sets it to 0, which IS False,
    so masking on it would mark every legacy False as missing.
  * A one-byte character variable is a boolean only when all its dimensions index
    observations; stationId(Location, nchars) is a string.  Read as a boolean,
    every printable character becomes True and unrelated ids compare equal.
  * Integers are never widened to float64.  Values above 2**53 collide when
    rounded, as do the two int64 missing sentinels.

Fatal vs reported:
  Data differences fail.  Differences in coordinate variables (name equals its
  only dimension: Location, Channel, nvars, ...) and in attributes are reported
  but not fatal, because the writers differ there by design -- OSDF emits no
  global attributes at all.  --strict makes them fatal, which is how to ask
  whether that gap has closed.

Multi-file sets:
  With "write multiple files: true" the IO pool writes one file per pool rank,
  suffixed _0000, _0001, ... before the last dot.  Location-indexed variables are
  concatenated across the set; others are read from the first file and checked
  against the rest.

--tolerance applies to floats only, integers always being exact, and relaxes the
comparison but not the sort, which needs bit-exact key values; a ULP-level
divergence would cascade into many failures rather than be absorbed.

Output:
  A pass prints just the verdict; a failure prints only the reasons for it.
  -v prints the full report of every variable that was checked, pass or fail.
  A tie warning, when there is one, is printed at every verbosity, because it
  qualifies the verdict rather than explaining it.

Exit codes:
  0  files match
  1  files differ (the reasons are printed to stdout)
  2  usage / I/O error
"""

import sys
import os
import glob
import math
import argparse
from collections import namedtuple

import numpy as np
import netCDF4 as nc


# util::missingValue<T>() and the NC_FILL_* constants, which the two writers
# choose between.  Tested in the native integer dtype -- see the module
# docstring.
KNOWN_INT_MISSING = (
    -2147483643,           # util::missingValue<int32_t>()
    -2147483647,           # NC_FILL_INT
    -9223372036854775801,  # util::missingValue<int64_t>()
    -9223372036854775806,  # NC_FILL_INT64
)
MISSING_STRING = "MISSING*"

# Floats at or beyond this magnitude are missing markers, not data.  Covers both
# NC_FILL_FLOAT (9.96921e+36) and util::missingValue<float>() (-3.368795e+38).
FLOAT_MISSING_MAGNITUDE = 1.0e30

LOCATION = "Location"

# Kept out of the sort key.  Location is a row label rather than observed data,
# and excluding it means a disagreement about it is reported on one line instead
# of scrambling the sort and failing every variable.
KEY_EXCLUDE = (LOCATION,)

# Dimensions that index observations rather than counting characters.  A one-byte
# character variable whose trailing dimension is NOT one of these is a
# fixed-length string stored as a char array, e.g. stationId(Location, nchars)
# Note this list will need to be maintained if the writers ever add a new index dimension
INDEX_DIMS = ("Location", "Channel", "Level", "nfactors", "nvars", "nrecs")

# kind    : 'int' | 'float' | 'bool' | 'string'
# values  : int64 | float64 | bool | unicode ndarray, missing entries zeroed
# missing : bool ndarray of the same shape
# dims    : tuple of netCDF dimension names.  For a char array this excludes the
#           character dimension, which canonical_column collapses away, so it
#           always matches the shape of `values`.
# note    : short human-readable remark for the report, may be ''
Column = namedtuple("Column", ["kind", "values", "missing", "dims", "note"])


def resolve_file_set(spec):
    """Turn a command-line file specification into an ordered list of paths.

    A spec is a comma-separated list of paths, or a single path.  A single path
    that does not exist is expanded to the multi-file set the IO pool writes:
    'foo.nc4' -> foo_0000.nc4, foo_0001.nc4, ... (the '_%04d' suffix inserted
    before the last dot, matching uniquifyFileName in EngineUtils.cpp).

    Exact paths win over expansion.  Both 'foo.nc4' and 'foo_0000.nc4' routinely
    coexist in a test output directory, so guessing would eventually select the
    wrong files and report a confidently wrong PASS.
    """
    if "," in spec:
        paths = [p.strip() for p in spec.split(",") if p.strip()]
        missing = [p for p in paths if not os.path.exists(p)]
        if missing:
            raise IOError("no such file: %s" % ", ".join(missing))
        return paths

    if os.path.exists(spec):
        return [spec]

    root, ext = os.path.splitext(spec)
    paths = sorted(glob.glob("%s_[0-9][0-9][0-9][0-9]%s" % (root, ext)))
    if not paths:
        raise IOError("no such file, and no %s_NNNN%s set found" % (root, ext))
    return paths


def collect_variables(group, prefix, out):
    """Recursively map 'Group/Subgroup/varname' -> netCDF4.Variable."""
    for name, var in group.variables.items():
        out[prefix + name] = var
    for name, sub in group.groups.items():
        collect_variables(sub, prefix + name + "/", out)
    return out


def logical_type(var):
    """Return 'string', 'bool', 'int' or 'float' for a netCDF4 Variable.

    Both writers normally emit NC_STRING for strings, but the legacy writer
    builds fixed-length strings first and only converts them in a post-pass, and
    older IODA files store strings as char arrays, so both forms are handled.

    A one-byte character variable is ambiguous: it is a boolean when all of its
    dimensions index observations, and a char-array string when its trailing
    dimension counts characters.  See INDEX_DIMS.
    """
    dtype = var.dtype
    if dtype is str or dtype == str:
        return "string"

    npdtype = np.dtype(dtype)
    if npdtype.kind in ("S", "U"):
        if npdtype.itemsize > 1:
            return "string"
        dims = tuple(var.dimensions)
        if len(dims) >= 2 and dims[-1] not in INDEX_DIMS:
            return "string"
        return "bool"

    if npdtype.itemsize == 1 and npdtype.kind in ("i", "u", "b"):
        return "bool"
    if npdtype.kind in ("i", "u"):
        return "int"
    return "float"


def _fill_value(var):
    """The variable's declared _FillValue, or None."""
    if "_FillValue" in var.ncattrs():
        return var.getncattr("_FillValue")
    return None


def _decode(value):
    """Render one netCDF string element as a Python str."""
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", "replace")
    return str(value)


def canonical_column(var, raw):
    """Apply the normalization rules table to one variable's raw data."""
    kind = logical_type(var)
    dims = tuple(var.dimensions)
    fill = _fill_value(var)

    if kind == "string":
        arr = np.asarray(raw)
        note = ""
        if arr.dtype.kind in ("S", "U") and arr.dtype.itemsize == 1 and len(dims) >= 2:
            # A char array: collapse the trailing character dimension into one
            # string per observation, and drop that dimension from the column.
            values = np.asarray(nc.chartostring(arr)).astype("U")
            values = np.char.rstrip(np.char.rstrip(values, "\x00"))
            dims = dims[:-1]
            note = "char array -> string"
            # The declared fill is a single character here, so it says nothing
            # about whether a joined string is missing.
            missing = (values == MISSING_STRING) | (values == "")
        else:
            flat = [_decode(v) for v in np.asarray(raw, dtype=object).ravel()]
            values = np.array(flat, dtype=object).reshape(arr.shape).astype("U")
            if arr.dtype.kind == "S" and arr.dtype.itemsize > 1:
                note = "%s -> string" % arr.dtype
            missing = values == MISSING_STRING
            if fill is not None:
                missing = missing | (values == _decode(fill))
        values = np.where(missing, "", values)
        return Column(kind, values, missing, dims, note)

    if kind == "bool":
        arr = np.asarray(raw)
        if arr.dtype.kind in ("S", "U"):
            flat = [bool(v) and v not in (b"", b"\x00", "", "\x00")
                    for v in arr.ravel()]
            values = np.array(flat, dtype=bool).reshape(arr.shape)
            note = "char <-> bool"
        else:
            values = arr != 0
            note = "%s <-> bool" % arr.dtype
        # Booleans have no representable missing state -- see module docstring.
        return Column(kind, values, np.zeros(values.shape, dtype=bool), dims, note)

    arr = np.asarray(raw)
    if kind == "int":
        # Test sentinels in the native integer dtype, then keep the values
        # integral.  Widening to float64 here would make int64 values above
        # 2**53 indistinguishable, which in a comparison tool means silently
        # reporting different data as equal.
        missing = np.zeros(arr.shape, dtype=bool)
        if fill is not None:
            missing = missing | (arr == np.asarray(fill).astype(arr.dtype))
        for sentinel in KNOWN_INT_MISSING:
            if np.can_cast(np.min_scalar_type(sentinel), arr.dtype, casting="safe"):
                missing = missing | (arr == arr.dtype.type(sentinel))
        values = arr.astype(np.int64)
        values = np.where(missing, 0, values)
        note = "" if arr.dtype == np.int64 else "%s -> int64" % arr.dtype
        return Column("int", values, missing, dims, note)

    values = arr.astype(np.float64)
    missing = ~np.isfinite(values)
    missing = missing | (np.abs(values) >= FLOAT_MISSING_MAGNITUDE)
    if fill is not None:
        missing = missing | (values == float(np.asarray(fill).ravel()[0]))
    values = np.where(missing, 0.0, values)
    note = "" if arr.dtype == np.float64 else "%s -> float64" % arr.dtype
    return Column("float", values, missing, dims, note)


def load_file_set(paths):
    """Read a file set and return (columns, nlocs, datasets, structural).

    Variables whose first dimension is Location are concatenated along axis 0
    across the set.  All other variables are taken from the first file; the
    remaining files hold per-pool-rank copies of the same dimension data.
    """
    datasets = []
    for path in paths:
        dataset = nc.Dataset(path, "r")
        # netCDF4 masks on _FillValue by default, which would silently hide the
        # zeros of a ubyte boolean whose fill value is 0.  Manage missingness
        # explicitly instead.
        dataset.set_auto_mask(False)
        datasets.append(dataset)

    per_file = [collect_variables(d, "", {}) for d in datasets]

    # Every member of a file set should carry the same schema.  Report each
    # inconsistent variable once, naming the files that lack it, rather than
    # once per file pair.
    every_name = set().union(*(set(v) for v in per_file))
    names = set(per_file[0]).intersection(*(set(v) for v in per_file[1:]))

    structural = []
    for name in sorted(every_name - names):
        absent = [p for p, v in zip(paths, per_file) if name not in v]
        structural.append("%s: missing from %d of %d files in the set (%s)"
                          % (name, len(absent), len(paths), ", ".join(absent)))

    nlocs = sum(d.dimensions[LOCATION].size for d in datasets
                if LOCATION in d.dimensions)

    columns = {}
    for name in sorted(names):
        var = per_file[0][name]
        if var.dimensions and var.dimensions[0] == LOCATION and len(datasets) > 1:
            raw = np.concatenate([np.asarray(pf[name][...]) for pf in per_file], axis=0)
            columns[name] = canonical_column(var, raw)
        else:
            columns[name] = canonical_column(var, np.asarray(var[...]))

    return columns, nlocs, datasets, structural


def is_coordinate(name, dims):
    """True for a netCDF coordinate variable (name equals its only dimension)."""
    return len(dims) == 1 and name == dims[0]


def key_columns(columns, nloc, names):
    """Build the full-row sort key: one 1-D sortable array per scalar slot.

    Every Location-indexed variable contributes one key column per trailing
    index, so a (Location, Channel) variable with 15 channels contributes 15.
    """
    keys = []
    for name in names:
        column = columns[name]
        values = column.values
        trailing = 1
        for size in values.shape[1:]:
            trailing *= size
        flat = values.reshape(nloc, trailing)
        if column.kind == "float":
            flat = np.where(column.missing.reshape(nloc, trailing), np.inf, flat)
        elif column.kind == "int":
            # Sort missing last, as np.inf does for floats, without leaving int64.
            flat = np.where(column.missing.reshape(nloc, trailing),
                            np.iinfo(np.int64).max, flat)
        elif column.kind == "bool":
            flat = flat.astype(np.int8)
        for index in range(trailing):
            keys.append(flat[:, index])
    return keys


def canonical_order(keys, nloc):
    """Lexicographic order over the key columns, first column primary."""
    if nloc == 0 or not keys:
        return np.arange(nloc)
    return np.lexsort(tuple(reversed(keys)))


def count_ties(keys, order):
    """Adjacent row pairs identical across every key column.

    Zero ties means the row correspondence between the two file sets is unique,
    not merely consistent.
    """
    if len(order) < 2 or not keys:
        return 0
    tied = np.ones(len(order) - 1, dtype=bool)
    for key in keys:
        ordered = key[order]
        tied &= ordered[1:] == ordered[:-1]
        if not tied.any():
            return 0
    return int(tied.sum())


def summarize(column):
    """A short human-checkable summary of a column's valid values."""
    nmiss = int(column.missing.sum())
    if column.kind not in ("int", "float"):
        return "n=%d missing=%d" % (column.values.size - nmiss, nmiss)
    valid = column.values[~column.missing]
    if valid.size == 0:
        return "n=0 missing=%d" % nmiss
    if column.kind == "int":
        # Exact: Python ints do not overflow or round.
        return "n=%d missing=%d min=%d max=%d sum=%d" % (
            valid.size, nmiss, valid.min(), valid.max(), sum(int(v) for v in valid))
    # math.fsum is correctly rounded, so the sum is a function of the multiset
    # and is bit-identical under permutation.  A naive float32 sum over IODA
    # data overflows to -inf and would make this line worse than useless.
    return "n=%d missing=%d min=%.6g max=%.6g sum=%.17g" % (
        valid.size, nmiss, valid.min(), valid.max(), math.fsum(valid))


def compare_column(ca, cb, order_a, order_b, tol, max_diffs):
    """Compare two canonicalized columns under the given row permutations.

    Returns (ok, detail_lines).
    """
    if ca.values.shape != cb.values.shape:
        return False, ["shape %s != %s" % (ca.values.shape, cb.values.shape)]

    numeric = ("int", "float")
    if ca.kind != cb.kind and not (ca.kind in numeric and cb.kind in numeric):
        return False, ["kind %s != %s" % (ca.kind, cb.kind)]

    values_a, values_b = ca.values, cb.values
    missing_a, missing_b = ca.missing, cb.missing

    # One side integral and the other floating -- nrecs is float32 from the
    # legacy writer and int32 from the OSDF writer, for instance.  Compare as
    # floats; both sides stay exact below 2**53.
    mixed_numeric = ca.kind != cb.kind
    if mixed_numeric:
        values_a = values_a.astype(np.float64)
        values_b = values_b.astype(np.float64)
    if order_a is not None:
        values_a, missing_a = values_a[order_a], missing_a[order_a]
        values_b, missing_b = values_b[order_b], missing_b[order_b]

    details = []
    bad = missing_a != missing_b
    if bad.any():
        details.append("missing-value mask differs at %d of %d elements"
                       % (int(bad.sum()), bad.size))

    present = ~(missing_a | missing_b)
    # A tolerance only makes sense for floating-point values; integers are
    # compared exactly so that no int64 difference can be absorbed.
    if tol > 0.0 and (ca.kind == "float" or mixed_numeric):
        unequal = ~np.isclose(values_a, values_b, atol=tol, rtol=0.0)
    else:
        unequal = values_a != values_b
    unequal &= present

    if unequal.any():
        details.append("%d of %d present values differ"
                       % (int(unequal.sum()), int(present.sum())))
        for index in list(zip(*np.nonzero(unequal)))[:max_diffs]:
            position = index[0] if len(index) == 1 else index
            details.append("    index %s:  A = %s   B = %s"
                           % (position, values_a[index], values_b[index]))

    return not details, details


def _diff_attrs(obj_a, obj_b, path, lines):
    """Diff the attribute sets of two netCDF objects (group or variable)."""
    keys_a, keys_b = set(obj_a.ncattrs()), set(obj_b.ncattrs())
    for key in sorted(keys_a - keys_b):
        lines.append("%s/@%s: present in A only" % (path, key))
    for key in sorted(keys_b - keys_a):
        lines.append("%s/@%s: present in B only" % (path, key))
    for key in sorted(keys_a & keys_b):
        value_a = np.asarray(obj_a.getncattr(key))
        value_b = np.asarray(obj_b.getncattr(key))
        if value_a.shape != value_b.shape or not np.array_equal(value_a, value_b):
            lines.append("%s/@%s: %r != %r" % (path, key, value_a, value_b))
    return lines


def compare_attributes(group_a, group_b, path, lines):
    """Recursively diff group and variable attributes.  Reported, not fatal."""
    _diff_attrs(group_a, group_b, path, lines)
    for name in sorted(set(group_a.variables) & set(group_b.variables)):
        _diff_attrs(group_a.variables[name], group_b.variables[name],
                    "%s/%s" % (path, name), lines)
    for name in sorted(set(group_a.groups) & set(group_b.groups)):
        compare_attributes(group_a.groups[name], group_b.groups[name],
                           "%s/%s" % (path, name), lines)
    return lines


def failure_report(structural, data_lines, coord_lines, attr_lines, strict):
    """The reasons a comparison failed, and nothing else.

    This is what a default (non-verbose) run prints when the files differ.  It
    lists only the categories that actually caused the failure, so that under
    --strict the coordinate and attribute sections appear, and otherwise they do
    not -- they did not contribute to the verdict.
    """
    lines = []
    if structural:
        lines.append("Structural differences:")
        lines.extend("  %s" % line for line in structural)
    if data_lines:
        lines.append("Data variables that differ:")
        for _, rendered in data_lines:
            lines.extend(rendered)
    if strict and coord_lines:
        lines.append("Coordinate variables that differ (fatal under --strict):")
        for _, rendered in coord_lines:
            lines.extend(rendered)
    if strict and attr_lines:
        lines.append("Attribute differences (fatal under --strict):")
        lines.extend("  %s" % line for line in attr_lines[:20])
        if len(attr_lines) > 20:
            lines.append("  ... %d more" % (len(attr_lines) - 20))
    return lines


EPILOG = """
examples:
  Compare a legacy ObsGroup output against an OSDF output:
    ioda_compare_obs.py testoutput/sondes_obs_2018041500_m.nc4 \\
                        testoutput/sondes_obs_2018041500_m_osdf_frame_rows.nc4

  Compare against a multi-file set written by a 4-rank IO pool.  A base name is
  expanded to the _0000.._000N members only when the base file does not exist,
  so name the members explicitly if both forms are present:
    ioda_compare_obs.py legacy.nc4 osdf.nc4
    ioda_compare_obs.py legacy.nc4 osdf_0000.nc4,osdf_0001.nc4

  See everything that was checked, not just the verdict:
    ioda_compare_obs.py a.nc4 b.nc4 -v

  Allow a small numeric difference, and show more of each mismatch:
    ioda_compare_obs.py a.nc4 b.nc4 -T 1.0e-6 -n 25

  Require the coordinate variables and attributes to agree as well.  The OSDF
  writer emits no global attributes, so this will normally fail:
    ioda_compare_obs.py a.nc4 b.nc4 --strict

  Use in a script: the exit code is the verdict, and a pass says one line:
    ioda_compare_obs.py a.nc4 b.nc4 && echo same

environment:
  Requires netCDF4 and numpy

see also:
  ioda_compare_obs_selftest.py  self-check on a tiny hand-verifiable pair
  ioda_compare_nc.py            in-order comparison against a reference file
"""


def build_parser():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("files_a", metavar="FILES_A",
                        help="first file set: a path, 'a.nc4,b.nc4', or a base "
                             "path expanded to base_0000.nc4, base_0001.nc4, ...")
    parser.add_argument("files_b", metavar="FILES_B",
                        help="second file set (same forms)")
    parser.add_argument("-T", "--tolerance", type=float, default=0.0, metavar="TOL",
                        help="absolute tolerance for numeric values (default: 0.0). "
                             "Relaxes the comparison, not the sort.")
    parser.add_argument("-n", "--max-diffs", type=int, default=5, metavar="N",
                        help="show at most N differing indices per variable (default: 5)")
    parser.add_argument("--strict", action="store_true",
                        help="also fail on coordinate-variable and attribute "
                             "differences, which are reported but not fatal by "
                             "default because the two writers differ there by design")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the full report of everything that was checked. "
                             "By default a pass prints only the verdict and a failure "
                             "prints only the reasons for it.")
    return parser


def main():
    # ---- 1. Parse arguments and load both sides ------------------------------
    args = build_parser().parse_args()
    exclude = set(KEY_EXCLUDE)

    try:
        paths_a = resolve_file_set(args.files_a)
        paths_b = resolve_file_set(args.files_b)
        columns_a, nloc_a, datasets_a, structural_a = load_file_set(paths_a)
        columns_b, nloc_b, datasets_b, structural_b = load_file_set(paths_b)
    except (IOError, OSError) as err:
        print("ERROR: %s" % err, file=sys.stderr)
        return 2

    # ---- 2. Set up the report buffer -----------------------------------------
    # The body is accumulated rather than printed as it is produced, so that it
    # can be flushed in one piece after the files are closed.  Redirect stdout if
    # you want only the exit code.
    out = []

    def emit(line=""):
        out.append(line.rstrip())

    emit("IODA obs comparison")
    emit("  A: %s (%d file%s)" % (paths_a[0], len(paths_a), "" if len(paths_a) == 1 else "s"))
    emit("  B: %s (%d file%s)" % (paths_b[0], len(paths_b), "" if len(paths_b) == 1 else "s"))
    emit()

    # ---- 3. Structural checks, then split the variables into two groups ------
    # Anything collected in `structural` is fatal regardless of the flags: the
    # two files cannot represent the same ObsGroup if their shapes disagree.
    structural = list(structural_a) + list(structural_b)
    names_a, names_b = set(columns_a), set(columns_b)
    for name in sorted(names_a - names_b):
        structural.append("%s: variable present in A only" % name)
    for name in sorted(names_b - names_a):
        structural.append("%s: variable present in B only" % name)
    if nloc_a != nloc_b:
        structural.append("Location count %d (A) != %d (B)" % (nloc_a, nloc_b))

    # Location-indexed variables get the canonical sort; everything else is
    # compared positionally.  A Location variable whose shapes disagree cannot
    # be sorted meaningfully, so it falls through to the positional group where
    # compare_column reports the shape mismatch.
    common = sorted(names_a & names_b)
    loc_names = [n for n in common
                 if columns_a[n].dims and columns_a[n].dims[0] == LOCATION
                 and columns_a[n].values.shape == columns_b[n].values.shape]
    other_names = [n for n in common if n not in set(loc_names)]

    emit("Locations: A = %d   B = %d" % (nloc_a, nloc_b))
    emit("Variables: %d in A, %d in B, %d in common (%d Location-indexed, %d other)"
         % (len(names_a), len(names_b), len(common), len(loc_names), len(other_names)))
    emit()

    # ---- 4. Establish the canonical row order --------------------------------
    # With differing location counts there is no correspondence to establish, so
    # the orders stay None and everything is compared positionally.
    if nloc_a != nloc_b:
        emit("Location counts differ -- cannot establish a row correspondence.")
        order_a = order_b = None
        ties = 0
        tie_warning = None
    else:
        # A variable can only key the sort if both sides canonicalize it the
        # same way; a kind mismatch is reported when it is compared instead.
        key_names = [n for n in loc_names
                     if n not in exclude and columns_a[n].kind == columns_b[n].kind]
        keys_a = key_columns(columns_a, nloc_a, key_names)
        keys_b = key_columns(columns_b, nloc_b, key_names)
        order_a = canonical_order(keys_a, nloc_a)
        order_b = canonical_order(keys_b, nloc_b)
        ties = count_ties(keys_a, order_a)

        emit("Canonical row sort")
        emit("  key = %d Location-indexed variable%s -> %d key column%s"
             % (len(key_names), "" if len(key_names) == 1 else "s",
                len(keys_a), "" if len(keys_a) == 1 else "s"))
        if exclude:
            emit("  excluded from key: %s" % ", ".join(sorted(exclude)))
        emit("  %d of %d rows distinct (%d tie%s)%s"
             % (nloc_a - ties, nloc_a, ties, "" if ties == 1 else "s",
                " -> row correspondence is unique" if ties == 0 else ""))
        if ties:
            # Tied rows are identical across the key, so any variable NOT in the
            # key may be paired arbitrarily between the two files.  Without this
            # warning a spurious DIFF on such a variable looks like real data
            # loss.  Variables in the key are unaffected: a tie there means the
            # rows are genuinely interchangeable.
            tie_warning = (
                "%d tied row pair%s: rows identical across the sort key, so "
                "differences reported for variables excluded from it (%s) may "
                "be an artifact of arbitrary pairing rather than real."
                % (ties, "" if ties == 1 else "s",
                   ", ".join(sorted(exclude)) if exclude else "none"))
            emit("  WARNING: %s" % tie_warning)
        else:
            tie_warning = None
        emit()

    # ---- 5. Compare every variable -------------------------------------------
    # record() sorts each result into one of two buckets.  That split is the
    # whole fatality policy: data differences fail, coordinate differences only
    # fail under --strict.  nonlocal is required because these counters are
    # rebound with +=, unlike coord_lines, which is only mutated.
    data_pass, data_fail, coord_pass, coord_diff = 0, 0, 0, 0
    coord_lines, data_lines = [], []

    def record(ok, lines, coord, name):
        """Tally one result, keeping the rendered lines of the ones that differ.

        Those lines are what the default (non-verbose) run prints: on a failure
        the user should see why, without the whole report around it.
        """
        nonlocal data_pass, data_fail, coord_pass, coord_diff
        if coord:
            coord_pass, coord_diff = coord_pass + ok, coord_diff + (not ok)
            if not ok:
                coord_lines.append((name, lines))
        else:
            data_pass, data_fail = data_pass + ok, data_fail + (not ok)
            if not ok:
                data_lines.append((name, lines))

    def compare_section(heading, section_names, permute):
        """Compare a group of variables and report each one.

        `permute` selects whether the canonical row order is applied, which is
        the only difference between the Location-indexed and the other section.
        """
        if not section_names:
            return
        emit(heading)
        for name in section_names:
            ca, cb = columns_a[name], columns_b[name]
            coord = is_coordinate(name, ca.dims)
            ok, details = compare_column(
                ca, cb,
                order_a if permute else None, order_b if permute else None,
                args.tolerance, args.max_diffs)
            shape = "x".join(str(s) for s in ca.values.shape)
            # On a match the two summaries are equal, so one on the headline is
            # enough.  On a mismatch a single summary would be misleading about
            # which side it describes, so both are listed underneath instead.
            # rstrip here rather than in emit(), because these lines are also
            # printed directly by the failure report.
            lines = [("  %-4s %-46s %-7s %-9s %s%s"
                      % ("PASS" if ok else "DIFF", name, ca.kind, shape,
                         summarize(ca) if ok else "",
                         "  [coordinate]" if coord else "")).rstrip()]
            if not ok:
                lines.append("       A: %s" % summarize(ca))
                lines.append("       B: %s" % summarize(cb))
                lines.extend("       %s" % line for line in details)
            record(ok, lines, coord, name)
            for line in lines:
                emit(line)
        emit()

    compare_section("Location-indexed variables (compared after canonical sort)",
                    loc_names, True)
    compare_section("Other variables (compared in file order)", other_names, False)

    # ---- 6. Report the remaining sections ------------------------------------
    # Each section prints only when it has content.
    attr_lines = compare_attributes(datasets_a[0], datasets_b[0], "", [])

    if structural:
        emit("Structural differences (fatal)")
        for line in structural:
            emit("  %s" % line)
        emit()

    if coord_lines:
        emit("Coordinate variables that differ (%sfatal)"
             % ("" if args.strict else "non-"))
        for _, lines in coord_lines:
            for line in lines:
                emit(line)
        emit()

    if attr_lines:
        # Members of a file set carry identical attributes, so only the leading
        # file of each side is diffed; doing all of them would just repeat every
        # line once per rank.
        emit("Attributes (%sfatal; %s vs %s)"
             % ("" if args.strict else "non-",
                os.path.basename(paths_a[0]), os.path.basename(paths_b[0])))
        for line in attr_lines[:20]:
            emit("  %s" % line)
        if len(attr_lines) > 20:
            emit("  ... %d more" % (len(attr_lines) - 20))
        emit()

    strictness = "" if args.strict else "non-"
    emit("Summary")
    emit("  data variables         %3d pass  %3d fail" % (data_pass, data_fail))
    emit("  coordinate variables   %3d pass  %3d diff  (%sfatal)"
         % (coord_pass, coord_diff, strictness))
    emit("  attribute differences  %3d            (%sfatal)"
         % (len(attr_lines), strictness))
    emit()

    # ---- 7. Decide the verdict and flush -------------------------------------
    # Data and structure always fail; coordinates and attributes only under
    # --strict, because the OSDF writer differs from the legacy one there by
    # design and would otherwise fail every real comparison.
    failed = (len(structural) > 0
              or data_fail > 0
              or (args.strict and (coord_diff > 0 or len(attr_lines) > 0)))

    for dataset in datasets_a + datasets_b:
        dataset.close()

    # --verbose prints everything that was checked.  By default a pass is just
    # the verdict, and a failure is only the reasons for it -- the passing
    # variables are not what the reader needs at that moment.
    if args.verbose:
        print("\n".join(out))
    elif failed:
        for line in failure_report(structural, data_lines, coord_lines,
                                   attr_lines, args.strict):
            print(line)

    # The tie warning qualifies the verdict itself rather than the detail, so it
    # is printed at every verbosity, on a pass as well as a failure.
    if tie_warning:
        print("WARNING: %s" % tie_warning)

    if failed:
        print("FAIL: %s vs %s" % (args.files_a, args.files_b))
        return 1

    print("PASS: both file sets contain the same %d observation records." % nloc_a)
    return 0


if __name__ == "__main__":
    sys.exit(main())
