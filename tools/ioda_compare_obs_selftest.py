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

The diagnostic cases then check that a failure says which observation differs
and how: by its identity and channel rather than by an index into the sort, a
changed identity as one row on each side only, and --dump as a pair of tables
that differ in exactly the changed line.

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

# The prefix of the directory 'ioda_compare_obs.py --dump' creates; it must
# match dump_tables there.  Used to check before removing one.
DUMP_PREFIX = "ioda_compare_obs_dump."

EPILOG = """
examples:
  Run the self-test:
    ioda_compare_obs_selftest.py

  Keep the generated files and read them in full, which is the point of a pair
  this small.  The directory they are in is printed at the end:
    ioda_compare_obs_selftest.py --keep
    ncdump <printed directory>/selftest_obsgroup.nc4
    ncdump <printed directory>/selftest_osdf.nc4

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
NC_FILL_INT64 = -9223372036854775806
NC_FILL_UINT64 = 18446744073709551614
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
        # Latitude is an identity variable, so this is a changed observation.
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
    elif kind == "change_obsvalue":
        # B's first row is ddd4; airTemperature is not an identity variable.
        var = dataset["ObsValue"]["airTemperature"]
        values = np.asarray(var[...])
        values[0] += 1.0
        var[...] = values
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


def is_own_temp_directory(directory, prefix=TEMP_PREFIX):
    """True if `directory` is a scratch directory made by mkdtemp with `prefix`.

    A pure predicate, so the rmtree in clean_up() can be checked against paths
    like "/" without any risk of deleting them.  Keep the realpath: it makes
    a symlink judged by where it points, not by its name.
    """
    resolved = os.path.realpath(directory)
    return (os.path.dirname(resolved) == os.path.realpath(tempfile.gettempdir())
            and os.path.basename(resolved).startswith(prefix))


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


def build_dimname_pair(directory):
    """Two files holding the same values over different, equal-sized dimensions.

    A is (Location, Channel) and B is (Location, Level), both 3 x 2.  Compared
    by shape alone they match, though a channel is not a level.
    """
    paths = []
    for name, second in (("dimname_a.nc4", "Channel"), ("dimname_b.nc4", "Level")):
        path = os.path.join(directory, name)
        dataset = nc.Dataset(path, "w", format="NETCDF4")
        dataset.createDimension("Location", 3)
        dataset.createDimension(second, 2)
        dataset.createVariable("Location", "i4", ("Location",))[:] = np.arange(3)
        meta = dataset.createGroup("MetaData")
        meta.createVariable("latitude", "f4", ("Location",))[:] = [10.0, 20.0, 30.0]
        obs = dataset.createGroup("ObsValue")
        obs.createVariable("airTemperature", "f4", ("Location", second))[:] = \
            [[250.0, 251.0], [252.0, 253.0], [254.0, 255.0]]
        dataset.close()
        paths.append(path)
    return paths


def build_unsigned_pair(directory, name, unsigned, signed, fill=None):
    """A variable stored as uint64 in file A and as int64 in file B.

    Cast to int64, 2**64 - 1 wraps to -1, so it would match a signed -1.
    fill, if given, is the _FillValue of each side, as (unsigned, signed).
    """
    paths = []
    for side, dtype, values, fill_value in (("a", "u8", unsigned, fill and fill[0]),
                                            ("b", "i8", signed, fill and fill[1])):
        path = os.path.join(directory, "%s_%s.nc4" % (name, side))
        dataset = nc.Dataset(path, "w", format="NETCDF4")
        dataset.createDimension("Location", len(values))
        dataset.createVariable("Location", "i4", ("Location",))[:] = np.arange(len(values))
        meta = dataset.createGroup("MetaData")
        meta.createVariable("latitude", "f4", ("Location",))[:] = 10.0 * np.arange(len(values))
        var = meta.createVariable("counter", dtype, ("Location",), fill_value=fill_value)
        var[:] = np.array(values, dtype=dtype)
        dataset.close()
        paths.append(path)
    return paths


def unsigned_cases(directory):
    """A uint64 value int64 cannot hold must not be wrapped into a match."""
    small_a, small_b = build_unsigned_pair(directory, "unsigned", [1, 2, 3], [1, 2, 3])
    code, _ = run(small_a, small_b)
    cases = [("uint64 vs int64 holding the same small values", 0, code)]

    big_a, big_b = build_unsigned_pair(directory, "unsigned_big", [2 ** 64 - 1], [-1])
    code, output = run(big_a, big_b)
    cases.append(("uint64 2**64-1 vs int64 -1  <- must not wrap to equal", 1, code))
    cases.append(("  ...and is reported as beyond the int64 range",
                  True, "MetaData/counter: 1 uint64 value beyond the int64 range" in output))

    # NC_FILL_UINT64 is itself beyond the int64 range, but marks a missing value.
    fill_a, fill_b = build_unsigned_pair(
        directory, "unsigned_fill", [5, NC_FILL_UINT64], [5, NC_FILL_INT64],
        fill=(NC_FILL_UINT64, NC_FILL_INT64))
    code, _ = run(fill_a, fill_b)
    cases.append(("  ...but a missing uint64 at NC_FILL_UINT64 is not", 0, code))
    return cases


def build_mixed_pair(directory, name, ints, floats):
    """A variable stored as int64 in file A and as float32 in file B.

    The two writers disagree on nrecs this way. Tested to ensure that
    2^53+1 does not falsely match 2^53 due to rounding.
    """
    paths = []
    for side, dtype, values in (("a", "i8", ints), ("b", "f4", floats)):
        path = os.path.join(directory, "%s_%s.nc4" % (name, side))
        dataset = nc.Dataset(path, "w", format="NETCDF4")
        dataset.createDimension("Location", len(values))
        dataset.createVariable("Location", "i4", ("Location",))[:] = np.arange(len(values))
        meta = dataset.createGroup("MetaData")
        meta.createVariable("latitude", "f4", ("Location",))[:] = 10.0 * np.arange(len(values))
        meta.createVariable("recordCount", dtype, ("Location",))[:] = values
        dataset.close()
        paths.append(path)
    return paths


def mixed_kind_cases(directory):
    """An int on one side and a float on the other is compared exactly."""
    mixed_a, mixed_b = build_mixed_pair(directory, "mixed", [1, 2, 3], [1.0, 2.0, 3.0])
    code, output = run(mixed_a, mixed_b, ["-v"])
    cases = [("int vs float holding the same whole numbers", 0, code),
             ("  ...and the kind difference is reported",
              True, "MetaData/recordCount: int in A, float in B" in output)]
    code, _ = run(mixed_a, mixed_b, ["--strict"])
    cases.append(("  ...which is fatal under --strict", 1, code))

    # 2**53 is exact in float32, being a power of two.
    big_a, big_b = build_mixed_pair(directory, "mixed_big", [2 ** 53 + 1], [2.0 ** 53])
    code, _ = run(big_a, big_b)
    cases.append(("int 2**53+1 vs float 2**53  <- must not round to equal", 1, code))

    frac_a, frac_b = build_mixed_pair(directory, "mixed_frac", [2], [2.5])
    code, _ = run(frac_a, frac_b, ["-T", "1.0"])
    cases.append(("int 2 not equal float 2.5, even with -T 1.0", 1, code))
    return cases


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


def file_set_cases(path_a, directory):
    """A multi-file set is checked member by member, not read from its first file.

    A is written again as a two-file set, as a 2-rank IO pool would, and then
    with the second member's Channel coordinate changed.  Channel is not
    Location-indexed, so only the first member's copy is compared with the
    other side; a disagreement between members must still be reported.
    """
    first = os.path.join(directory, "selftest_set_0000.nc4")
    second = os.path.join(directory, "selftest_set_0001.nc4")
    _write(first, RECORDS[:3], legacy=True)
    _write(second, RECORDS[3:], legacy=True)
    code, _ = run("%s,%s" % (first, second), path_a)
    cases = [("A as a two-file set, matching", 0, code)]

    bad = os.path.join(directory, "selftest_set_0001_bad.nc4")
    shutil.copy(second, bad)
    dataset = nc.Dataset(bad, "r+")
    dataset["Channel"][:] = [1, 4]
    dataset.close()
    code, output = run("%s,%s" % (first, bad), path_a)
    cases.append(("a set whose members disagree on Channel", 1, code))
    cases.append(("  ...is a structural difference naming both files",
                  True, "Channel: differs between %s and %s" % (first, bad) in output))
    return cases


def write_mutated(source, directory, kind):
    """Copy B with the named corruption from mutate(); return its path."""
    target = os.path.join(directory, "selftest_osdf_mutated_%s.nc4" % kind)
    mutate(source, target, kind)
    return target


def diagnostic_cases(path_a, path_b, directory):
    """A failure must say which observation differs, and how."""
    _, output = run(path_a, write_mutated(path_b, directory, "change_obsvalue"))
    cases = [("non-identity change: row named by identity, not index",
              True, "[stationIdentification=ddd4" in output and "index " not in output)]

    _, output = run(path_a, write_mutated(path_b, directory, "drop_missing"))
    cases.append(("missing vs present: the row is listed, shown as missing",
                  True, "A = missing" in output))

    # Without pairing by identity, the moved row would misalign others.
    _, output = run(path_a, write_mutated(path_b, directory, "change_one_value"))
    cases.append(("changed identity: one row each side only, no cascade",
                  True, "1 row in A only" in output and "1 row in B only" in output
                  and "Data variables that differ" not in output))

    # B's first row is ddd4, and its second channel is Channel 3, not index 1.
    _, output = run(path_a, write_mutated(path_b, directory, "change_channel"))
    cases.append(("2-D change: channel named by its coordinate value",
                  True, "[stationIdentification=ddd4" in output and "] Channel=3:" in output))

    # B without its last row, bbb2: the rows that remain still pair and match.
    short = os.path.join(directory, "selftest_osdf_short.nc4")
    _write(short, [RECORDS[i] for i in PERMUTATION][:-1], legacy=False)
    code, output = run(path_a, short)
    cases.append(("a Location missing from B", 1, code))
    cases.append(("  ...is listed by identity, and the rest still compared",
                  True, "1 row in A only" in output and "[stationIdentification=bbb2" in output
                  and "Data variables that differ" not in output))

    code, _ = run(path_a, path_b, ["--key", "MetaData/latitude"])
    cases.append(("--key naming one identity variable, matching pair", 0, code))
    code, output = run(path_a, path_b, ["--key", "MetaData/latitude,MetaData/noSuchVar"])
    cases.append(("--key naming an unknown variable is a usage error", 2, code))
    cases.append(("  ...which names the variable", True, "--key: MetaData/noSuchVar" in output))
    return cases


def dumped_line_differences(output):
    """How many lines differ between the two tables a --dump run reports.

    The dump directory is removed afterwards, once it is confirmed to be one
    the comparator made.  None if the report names no dump.
    """
    commands = [line.split() for line in output.splitlines() if line.startswith("  diff ")]
    if not commands or len(commands[0]) != 3:
        return None
    path_a, path_b = commands[0][1:]
    with open(path_a) as file_a, open(path_b) as file_b:
        lines_a, lines_b = file_a.readlines(), file_b.readlines()
    directory = os.path.dirname(path_a)
    if is_own_temp_directory(directory, DUMP_PREFIX):
        shutil.rmtree(directory, ignore_errors=True)
    return sum(x != y for x, y in zip(lines_a, lines_b)) + abs(len(lines_a) - len(lines_b))


def dump_cases(path_a, path_b, directory):
    _, output = run(path_a, write_mutated(path_b, directory, "change_obsvalue"), ["--dump"])
    return [("--dump: the sorted tables differ in exactly one line",
             1, dumped_line_differences(output))]


def remove_directory(directory):
    """Remove a scratch directory; if that fails, say so rather than leave it silently."""
    try:
        shutil.rmtree(directory)
    except OSError as err:
        print("WARNING: could not remove scratch directory %s: %s" % (directory, err),
              file=sys.stderr)


def clean_up(keep, directory):
    """Delete the scratch directory, or under --keep say where it is."""
    if keep:
        print("Test files kept in %s -- inspect with:" % directory)
        for name in ("selftest_obsgroup.nc4", "selftest_osdf.nc4"):
            print("  ncdump %s" % os.path.join(directory, name))
    elif is_own_temp_directory(directory):
        remove_directory(directory)
    else:
        print("REFUSING to delete %s: not a %s* scratch directory"
              % (directory, TEMP_PREFIX), file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--keep", action="store_true",
                        help="leave the test files in their temporary directory, "
                             "and print where it is, so they can be inspected "
                             "with ncdump")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the comparator's full report for the good pair")
    args = parser.parse_args()

    try:
        # Always a fresh directory this script creates, even under --keep, so
        # the test files can never land on, or overwrite, anything else.
        directory = tempfile.mkdtemp(prefix=TEMP_PREFIX)
    except OSError as err:
        print("ERROR: %s" % err, file=sys.stderr)
        return 2
    # Every exit from here on -- a pass, a failure, an early error, or an
    # exception nothing expected -- cleans up the scratch directory.
    try:
        return run_self_test(args, directory)
    finally:
        clean_up(args.keep, directory)


def run_self_test(args, directory):
    """Write the fixtures into `directory`, run every case, and return the exit code."""
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
    cases += mixed_kind_cases(directory)
    cases += unsigned_cases(directory)

    dim_a, dim_b = build_dimname_pair(directory)
    code, output = run(dim_a, dim_b)
    cases.append(("Channel vs Level of equal size  <- shape alone matches", 1, code))
    cases.append(("  ...and the dimension names are reported",
                  True, "dimensions ('Location', 'Channel') != ('Location', 'Level')"
                  in output))

    # A file list of only commas names no file.  It must be a usage error, not
    # a traceback, whose exit code 1 would read as "files differ".
    for spec in (",", ", ,"):
        code, output = run(spec, path_b)
        cases.append(("file list %-5r is a usage error" % spec, 2, code))
        cases.append(("  ...reported, not a traceback",
                      True, "ERROR: no file named in" in output and "Traceback" not in output))

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

    cases += file_set_cases(path_a, directory)
    cases += diagnostic_cases(path_a, path_b, directory)
    cases += dump_cases(path_a, path_b, directory)

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

    if failures:
        print("SELF-TEST FAILED: %d of %d cases behaved unexpectedly"
              % (failures, len(cases)))
        return 1
    print("SELF-TEST PASSED: all %d cases behaved as expected" % len(cases))
    return 0


if __name__ == "__main__":
    sys.exit(main())
