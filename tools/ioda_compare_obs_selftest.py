#!/usr/bin/env python3

# (C) Copyright 2026 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

"""Self-test for ioda_compare_obs.py using files small enough to check by hand.

The real ObsGroup-vs-OSDF test outputs hold tens of thousands of values, which
is far too many for a person to verify.  This script builds a deliberately tiny
pair -- 6 observations across 9 variables -- that a reviewer can dump in full
with ncdump and check by eye, then runs the comparator on it and on a series of
mutations.  The exact value count is printed when the script runs.

The pair is constructed to reproduce every difference the two IODA writers are
known to introduce, so that a PASS on it exercises each normalization rule:

  * rows in a different order (file B is a permutation of file A)
  * float missing values written with a different sentinel
    (A uses NC_FILL_FLOAT 9.96921e+36, B uses util::missingValue -3.368795e+38)
  * integer missing values written with a different sentinel
    (A uses NC_FILL_INT, B uses util::missingValue<int32_t>)
  * a boolean stored as ubyte in A and as char in B
  * different integer widths (int32 in A, int64 in B)
  * a vlen string variable carrying the "MISSING*" sentinel.  Both real
    writers agree here -- they both emit NC_STRING with that sentinel -- so
    the pair matches too; the point is to exercise the string path.
  * global attributes present in A and absent in B
  * a 'units' attribute added to every variable in B, coordinates included
  * a (Location, Channel) variable, to cover the 2-D case

The mutation cases then confirm the tool actually has teeth.  The important one
is 'one column permuted': it is invisible to per-variable norms and per-variable
hashes, and is the failure mode this comparator exists to catch.

Exit codes:
  0  every self-test behaved as expected
  1  at least one self-test did not
  2  usage / I/O error
"""

import os
import sys
import shutil
import argparse
import subprocess
import tempfile

import numpy as np
import netCDF4 as nc

HERE = os.path.dirname(os.path.abspath(__file__))
COMPARE = os.path.join(HERE, "ioda_compare_obs.py")

# Shared by mkdtemp and is_own_temp_directory so the two cannot drift apart.
TEMP_PREFIX = "ioda_compare_obs_selftest."

EPILOG = """
examples:
  Run the self-test:
    ioda_compare_obs_selftest.py

  Keep the generated files and read them in full, which is the point of a pair
  this small:
    ioda_compare_obs_selftest.py --keep /tmp/selftest
    ncdump /tmp/selftest/selftest_obsgroup.nc4
    ncdump /tmp/selftest/selftest_osdf.nc4

  Show the comparator's whole report for the matching pair:
    ioda_compare_obs_selftest.py -v

environment:
  Requires netCDF4 and numpy

see also:
  ioda_compare_obs.py        the tool under test
"""

NC_FILL_FLOAT = 9.9692099683868690e+36
NC_FILL_INT = -2147483647
MISSING_FLOAT = -3.368795e+38
MISSING_INT32 = -2147483643
MISSING_STRING = "MISSING*"

# Six observations, listed in the order file A stores them.  File B stores the
# same six records in the order given by PERMUTATION.  Everything a reviewer
# needs to check the test by hand is in this table.
#
#   station  lat    lon     airTemp        qc          flag   bt(2 channels)
RECORDS = [
    ("aaa1", 10.0, 100.0, 280.5, 0, True, (250.0, 251.0)),
    ("bbb2", 20.0, 110.0, None, 1, False, (252.0, 253.0)),
    ("ccc3", 30.0, 120.0, 282.5, None, True, (254.0, 255.0)),
    ("ddd4", 40.0, 130.0, 283.5, 2, False, (256.0, 257.0)),
    ("eee5", 50.0, 140.0, None, 0, True, (258.0, 259.0)),
    (None, 60.0, 150.0, 285.5, 1, False, (260.0, 261.0)),
]
PERMUTATION = [3, 0, 5, 2, 4, 1]
CHANNELS = [1, 3]


def _write(path, records, legacy):
    """Write the record table to a file, in the style of one of the writers."""
    dataset = nc.Dataset(path, "w", format="NETCDF4")
    dataset.createDimension("Location", len(records))
    dataset.createDimension("Channel", len(CHANNELS))

    fill_float = NC_FILL_FLOAT if legacy else MISSING_FLOAT
    fill_int = NC_FILL_INT if legacy else MISSING_INT32
    int_type = "i4" if legacy else "i8"

    if legacy:
        # The legacy writer copies the input file's global attributes; the OSDF
        # writer emits none at all.
        dataset.setncattr("_ioda_layout", "ObsGroup")
        dataset.setncattr("date_time", np.int32(2018041500))

    # The OSDF writer stamps units on the coordinate variables too, so the
    # fixture does the same to stay faithful to the real output.
    location = dataset.createVariable("Location", int_type, ("Location",))
    location[:] = np.arange(len(records))
    channel = dataset.createVariable("Channel", "i4", ("Channel",))
    channel[:] = np.array(CHANNELS, dtype=np.int32)
    if not legacy:
        location.setncattr("units", "")
        channel.setncattr("units", "")

    meta = dataset.createGroup("MetaData")
    obs = dataset.createGroup("ObsValue")
    preqc = dataset.createGroup("PreQC")

    def add(group, name, dtype, dims, values, fill):
        var = group.createVariable(name, dtype, dims, fill_value=fill)
        if not legacy:
            # The OSDF writer stamps a units attribute on every variable.
            var.setncattr("units", "MISSING*")
        var[...] = values
        return var

    add(meta, "latitude", "f4", ("Location",),
        np.array([r[1] for r in records], dtype=np.float32), fill_float)
    add(meta, "longitude", "f4", ("Location",),
        np.array([r[2] for r in records], dtype=np.float32), fill_float)

    station = meta.createVariable("stationIdentification", str, ("Location",),
                                  fill_value=MISSING_STRING)
    if not legacy:
        station.setncattr("units", "MISSING*")
    for index, record in enumerate(records):
        station[index] = MISSING_STRING if record[0] is None else record[0]

    add(obs, "airTemperature", "f4", ("Location",),
        np.array([fill_float if r[3] is None else r[3] for r in records],
                 dtype=np.float32), fill_float)

    add(preqc, "airTemperature", int_type, ("Location",),
        np.array([fill_int if r[4] is None else r[4] for r in records]), fill_int)

    # A boolean: ubyte in the legacy file, char in the OSDF file.
    if legacy:
        flag = meta.createVariable("qualityFlag", "u1", ("Location",), fill_value=0)
        flag[:] = np.array([1 if r[5] else 0 for r in records], dtype=np.uint8)
    else:
        flag = meta.createVariable("qualityFlag", "S1", ("Location",))
        flag.setncattr("units", "MISSING*")
        flag[:] = np.array([b"\x01" if r[5] else b"" for r in records])

    add(obs, "brightnessTemperature", "f4", ("Location", "Channel"),
        np.array([r[6] for r in records], dtype=np.float32), fill_float)

    dataset.close()


def build_pair(directory):
    """Write the matching legacy/OSDF pair and return their paths."""
    path_a = os.path.join(directory, "selftest_obsgroup.nc4")
    path_b = os.path.join(directory, "selftest_osdf.nc4")
    _write(path_a, RECORDS, legacy=True)
    _write(path_b, [RECORDS[i] for i in PERMUTATION], legacy=False)
    return path_a, path_b


def mutate(source, target, kind):
    """Copy `source` to `target` and corrupt it in one specific way."""
    shutil.copy(source, target)
    dataset = nc.Dataset(target, "r+")
    dataset.set_auto_mask(False)
    if kind == "permute_one_column":
        # Latitude alone is permuted, so every observation keeps a valid-looking
        # latitude but is bound to the wrong longitude.  Per-variable norms and
        # per-variable hashes cannot see this.
        var = dataset["MetaData"]["longitude"]
        values = np.asarray(var[...])
        var[...] = values[::-1]
    elif kind == "change_one_value":
        var = dataset["MetaData"]["latitude"]
        values = np.asarray(var[...])
        values[2] += 0.25
        var[...] = values
    elif kind == "drop_missing":
        var = dataset["ObsValue"]["airTemperature"]
        values = np.asarray(var[...])
        values[np.abs(values) >= 1.0e30] = 999.0
        var[...] = values
    elif kind == "change_string":
        dataset["MetaData"]["stationIdentification"][0] = "zzz9"
    elif kind == "change_bool":
        var = dataset["MetaData"]["qualityFlag"]
        values = np.asarray(var[...])
        var[...] = values[::-1]
    elif kind == "change_channel":
        var = dataset["ObsValue"]["brightnessTemperature"]
        values = np.asarray(var[...])
        values[0, 1] += 1.0
        var[...] = values
    else:
        raise ValueError("unknown mutation %s" % kind)
    dataset.close()


def count_values(path):
    """Total number of stored values in a file, for the 'check it by hand' claim."""
    def walk(group):
        total = 0
        for var in group.variables.values():
            size = 1
            for length in var.shape:
                size *= length
            total += size
        for sub in group.groups.values():
            total += walk(sub)
        return total

    dataset = nc.Dataset(path, "r")
    total = walk(dataset)
    dataset.close()
    return total


def is_own_temp_directory(directory):
    """True if `directory` is a scratch directory made by mkdtemp below.

    A pure predicate, so the rmtree in main() can be checked against paths
    like "/" without any risk of deleting them.  Keep the realpath: it makes
    a symlink judged by where it points, not by its name.
    """
    resolved = os.path.realpath(directory)
    return (os.path.dirname(resolved) == os.path.realpath(tempfile.gettempdir())
            and os.path.basename(resolved).startswith(TEMP_PREFIX))


def run(path_a, path_b, extra=()):
    """Run the comparator; return (exit_code, output)."""
    command = [sys.executable, COMPARE, path_a, path_b] + list(extra)
    result = subprocess.run(command, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, universal_newlines=True)
    return result.returncode, result.stdout


def build_charstring_pair(directory):
    """Two files whose fixed-length char-array strings are entirely different.

    A char array such as stationId(Location, nchars) must be read as a string.
    Read as a boolean it becomes all-True on both sides and the files compare
    equal, which is a silent false PASS.
    """
    # The identifiers are space-padded to the full width on purpose.  With NUL
    # padding the two files would hold different numbers of non-NUL characters
    # per row, so even a boolean misreading would come out unequal and the
    # regression would be caught by accident.  Padded to equal length, the
    # misreading yields all-True on both sides -- a true false PASS, which is
    # what this case has to reproduce.
    paths = []
    for name, ids in (("charstr_a.nc4", ["alpha", "bravo", "charlie"]),
                      ("charstr_b.nc4", ["zulu", "yankee", "xray"])):
        path = os.path.join(directory, name)
        dataset = nc.Dataset(path, "w", format="NETCDF4")
        dataset.createDimension("Location", 3)
        dataset.createDimension("nchars", 8)
        dataset.createVariable("Location", "i4", ("Location",))[:] = np.arange(3)
        meta = dataset.createGroup("MetaData")
        meta.createVariable("latitude", "f4", ("Location",))[:] = [10.0, 20.0, 30.0]
        var = meta.createVariable("stationId", "S1", ("Location", "nchars"))
        padded = np.array(["%-8s" % i for i in ids], dtype="S8")
        var[:] = nc.stringtochar(padded)
        dataset.close()
        paths.append(path)
    return paths


def build_bigint_pair(directory):
    """Two files whose int64 values differ by one, on either side of 2**53.

    Widened to float64 these collide, so an implementation that casts integers
    to float cannot tell them apart.
    """
    paths = []
    for name, value in (("bigint_a.nc4", 2 ** 53), ("bigint_b.nc4", 2 ** 53 + 1)):
        path = os.path.join(directory, name)
        dataset = nc.Dataset(path, "w", format="NETCDF4")
        dataset.createDimension("Location", 1)
        dataset.createVariable("Location", "i4", ("Location",))[:] = [0]
        meta = dataset.createGroup("MetaData")
        meta.createVariable("bigCounter", "i8", ("Location",))[:] = [value]
        dataset.close()
        paths.append(path)
    return paths


def build_tie_pair(directory):
    """Two files holding two observations that are identical in every variable.

    Location is held out of the sort key, so rows this alike tie and Location
    alone could be paired either way round.  That is harmless -- Location is a
    non-fatal coordinate variable, so the pair still passes -- but the tool must
    report the tie rather than let it pass unmentioned.
    """
    paths = []
    for name, locations in (("tie_a.nc4", [0, 1]), ("tie_b.nc4", [1, 0])):
        path = os.path.join(directory, name)
        dataset = nc.Dataset(path, "w", format="NETCDF4")
        dataset.createDimension("Location", 2)
        dataset.createVariable("Location", "i4", ("Location",))[:] = locations
        meta = dataset.createGroup("MetaData")
        meta.createVariable("longitude", "f4", ("Location",))[:] = [100.0, 100.0]
        meta.createVariable("latitude", "f4", ("Location",))[:] = [10.0, 10.0]
        dataset.close()
        paths.append(path)
    return paths


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--keep", metavar="DIR",
                        help="write the test files to DIR and leave them there, "
                             "so they can be inspected with ncdump")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the comparator's full report for the good pair")
    args = parser.parse_args()

    # Kept separate from `directory` so that the cleanup below can only ever
    # be handed a directory this script created itself.
    temp_directory = None if args.keep else tempfile.mkdtemp(prefix=TEMP_PREFIX)
    directory = args.keep or temp_directory
    if args.keep and not os.path.isdir(args.keep):
        os.makedirs(args.keep)
        print("Created keep directory %s" % args.keep)
    elif args.keep:
        print("Using existing keep directory %s" % args.keep)

    try:
        path_a, path_b = build_pair(directory)
    except (IOError, OSError) as err:
        print("ERROR: %s" % err, file=sys.stderr)
        return 2

    total = count_values(path_a)
    print("ioda_compare_obs self-test")
    print("  %d observations, %d values in total -- small enough to read with ncdump"
          % (len(RECORDS), total))
    print("  A: %s   (ObsGroup style: NC_FILL sentinels, ubyte bool, int32,"
          " global attrs)" % os.path.basename(path_a))
    print("  B: %s   (OSDF style: util::missingValue sentinels, char bool,"
          " int64, units attrs, rows permuted %s)"
          % (os.path.basename(path_b), PERMUTATION))
    print()

    cases = []

    # Each run of the comparator contributes two cases: one for the exit code
    # and one for the shape of the report it printed.
    code, output = run(path_a, path_b)
    cases.append(("matching pair (permuted rows, all writer differences)", 0, code))
    cases.append(("  ...and a pass prints only the verdict, in 1 line",
                  1, len(output.strip().splitlines())))

    code, output = run(path_a, path_b, ["-v"])
    cases.append(("matching pair again, with -v", 0, code))
    cases.append(("  ...and -v prints the full report, over 20 lines",
                  True, len(output.splitlines()) > 20))
    if args.verbose:
        print(output)

    code, _ = run(path_a, path_a)
    cases.append(("identity control (A vs A)", 0, code))

    for kind, label in [
        ("permute_one_column", "one column permuted  <- norms are blind to this"),
        ("change_one_value", "one value changed by 0.25"),
        ("drop_missing", "a missing value replaced by real data"),
        ("change_string", "one string changed"),
        ("change_bool", "boolean column permuted"),
        ("change_channel", "one channel of a 2-D variable changed"),
    ]:
        target = os.path.join(directory, "selftest_osdf_mutated_%s.nc4" % kind)
        mutate(path_b, target, kind)
        code, _ = run(path_a, target)
        cases.append((label, 1, code))

    code, _ = run(path_a, path_b, ["--strict"])
    cases.append(("--strict on the matching pair", 1, code))

    # Regression cases for three ways an earlier version reported PASS on files
    # that differ, or FAIL on files that do not.
    char_a, char_b = build_charstring_pair(directory)
    code, output = run(char_a, char_b)
    cases.append(("char-array strings differ  <- must not be read as bool", 1, code))
    cases.append(("  ...and are classified as strings",
                  True, "string" in output.split("stationId")[-1][:40]))

    big_a, big_b = build_bigint_pair(directory)
    code, _ = run(big_a, big_b)
    cases.append(("int64 2**53 vs 2**53+1  <- must not collapse in float64", 1, code))

    # The cleanup guard.  These paths are only spelled, never touched -- the
    # predicate deletes nothing, which is what makes asking about them safe.
    unsafe = ["/", "/home", os.path.expanduser("~"), tempfile.gettempdir(), "", "."]
    cases.append(("cleanup guard rejects /, /home, $HOME, tmp, '' and .",
                  0, sum(is_own_temp_directory(path) for path in unsafe)))
    cases.append(("  ...and accepts a mkdtemp scratch directory", True,
                  is_own_temp_directory(os.path.join(tempfile.gettempdir(),
                                                     TEMP_PREFIX + "abc123"))))

    tie_a, tie_b = build_tie_pair(directory)
    code, output = run(tie_a, tie_b)
    cases.append(("indistinguishable observations still pass", 0, code))
    cases.append(("  ...and the tie is reported, not passed over silently",
                  True, "WARNING" in output and "tied row pair" in output))

    # A case is either an expected exit code or an expected boolean assertion
    # about the report text, so render both as strings.
    print("%-62s %-8s %-8s %s" % ("case", "expect", "actual", "result"))
    print("-" * 92)
    failures = 0
    for label, expected, actual in cases:
        ok = expected == actual
        failures += 0 if ok else 1
        print("%-62s %-8s %-8s %s"
              % (label, expected, actual, "ok" if ok else "WRONG"))
    print()

    if args.keep:
        print("Test files kept in %s -- inspect with:" % directory)
        print("  ncdump %s" % path_a)
        print("  ncdump %s" % path_b)
    elif is_own_temp_directory(temp_directory):
        shutil.rmtree(temp_directory, ignore_errors=True)
    else:
        print("REFUSING to delete %s: not a %s* scratch directory"
              % (temp_directory, TEMP_PREFIX), file=sys.stderr)

    if failures:
        print("SELF-TEST FAILED: %d of %d cases behaved unexpectedly"
              % (failures, len(cases)))
        return 1
    print("SELF-TEST PASSED: all %d cases behaved as expected" % len(cases))
    return 0


if __name__ == "__main__":
    sys.exit(main())
