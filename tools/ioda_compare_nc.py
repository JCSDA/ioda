#!/usr/bin/env python3

# (C) Copyright 2026 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

"""Compare two NetCDF4 files for structural and value equivalence.

Comparison rules:
  - Group structure must match (same subgroups at each level)
  - Variables must match (same names within each group)
  - Variable shapes must match
  - Variable data values must match (within --tolerance for floats)
  - Variable and group attributes must match by value
  - Attribute data type differences (e.g., int8 vs uint8) are ignored
    as long as the values are numerically equal after widening to int64
  - The _NCProperties HDF5 superblock attribute is not visible through
    the NetCDF API and is therefore automatically excluded

Exit codes:
  0  files match
  1  files differ (differences are printed to stdout)
  2  usage / I/O error
"""

import sys
import argparse
import numpy as np
import netCDF4 as nc


def _to_comparable(val):
    """Return val as a numpy array in a form suitable for equality testing.

    Signed and unsigned integer arrays are widened to int64 so that, e.g.,
    int8(65) and uint8(65) compare equal.  Byte-string arrays are decoded
    to Unicode so that b'hello' and 'hello' compare equal.
    """
    arr = np.asarray(val)
    if arr.dtype.kind in ('i', 'u'):
        return arr.astype(np.int64)
    if arr.dtype.kind == 'S':
        return arr.astype('U')
    return arr


def compare_attrs(obj1, obj2, path, errors):
    keys1 = set(obj1.ncattrs())
    keys2 = set(obj2.ncattrs())
    for k in sorted(keys1 - keys2):
        errors.append(f"{path}/@{k}: present in first file only")
    for k in sorted(keys2 - keys1):
        errors.append(f"{path}/@{k}: present in second file only")
    for k in sorted(keys1 & keys2):
        v1 = _to_comparable(obj1.getncattr(k))
        v2 = _to_comparable(obj2.getncattr(k))
        if v1.shape != v2.shape:
            errors.append(f"{path}/@{k}: shape {v1.shape} != {v2.shape}")
            continue
        if not np.array_equal(v1, v2):
            errors.append(f"{path}/@{k}: {v1!r} != {v2!r}")


def compare_variable(v1, v2, path, errors, tol):
    compare_attrs(v1, v2, path, errors)

    if v1.shape != v2.shape:
        errors.append(f"{path}: shape {v1.shape} != {v2.shape}")
        return

    # np.asarray on a masked array returns the underlying data including
    # fill values, giving a straightforward bit-exact comparison.
    a1 = np.asarray(v1[:])
    a2 = np.asarray(v2[:])

    if a1.dtype.kind in ('i', 'u') and a2.dtype.kind in ('i', 'u'):
        a1 = a1.astype(np.int64)
        a2 = a2.astype(np.int64)
    elif a1.dtype.kind == 'S':
        a1 = a1.astype('U')
        a2 = a2.astype('U')

    if tol > 0.0 and np.issubdtype(a1.dtype, np.floating):
        if not np.allclose(a1, a2, atol=tol, equal_nan=True):
            errors.append(f"{path}: values differ beyond tolerance {tol}")
    elif not np.array_equal(a1, a2):
        errors.append(f"{path}: values differ")


def compare_group(g1, g2, path, errors, tol):
    compare_attrs(g1, g2, path, errors)

    sub1 = set(g1.groups)
    sub2 = set(g2.groups)
    for name in sorted(sub1 - sub2):
        errors.append(f"{path}/{name}: group present in first file only")
    for name in sorted(sub2 - sub1):
        errors.append(f"{path}/{name}: group present in second file only")
    for name in sorted(sub1 & sub2):
        compare_group(g1.groups[name], g2.groups[name],
                      f"{path}/{name}", errors, tol)

    vars1 = set(g1.variables)
    vars2 = set(g2.variables)
    for name in sorted(vars1 - vars2):
        errors.append(f"{path}/{name}: variable present in first file only")
    for name in sorted(vars2 - vars1):
        errors.append(f"{path}/{name}: variable present in second file only")
    for name in sorted(vars1 & vars2):
        compare_variable(g1.variables[name], g2.variables[name],
                         f"{path}/{name}", errors, tol)


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument('file1', help="first NetCDF4 file")
    ap.add_argument('file2', help="second NetCDF4 file (reference)")
    ap.add_argument(
        '-T', '--tolerance', type=float, default=0.0, metavar='TOL',
        help="absolute tolerance for floating-point comparisons (default: 0.0)",
    )
    ap.add_argument(
        '-v', '--verbose', action='store_true',
        help="print a PASS message on success",
    )
    args = ap.parse_args()

    errors = []
    try:
        with nc.Dataset(args.file1, 'r') as f1, nc.Dataset(args.file2, 'r') as f2:
            compare_group(f1, f2, '/', errors, args.tolerance)
    except OSError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(2)

    if errors:
        print(f"FAIL: {args.file1} vs {args.file2}")
        for msg in errors:
            print(f"  {msg}")
        sys.exit(1)

    if args.verbose:
        print(f"PASS: {args.file1} matches {args.file2}")
    sys.exit(0)


if __name__ == '__main__':
    main()
