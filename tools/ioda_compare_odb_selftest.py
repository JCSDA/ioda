#!/usr/bin/env python3

# (C) Copyright 2026 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

"""Self-test for ioda_compare_odb.py using files small enough to check by hand.

The real ObsGroup-vs-OSDF ODB outputs hold tens of thousands of rows, which is
far too many for a person to verify.  This script builds a deliberately tiny
pair -- 6 rows across 8 columns -- with 'odc import', so that a reviewer can
dump it in full with 'odc sql' and check it by eye, then runs the comparator on
it and on a series of mutations.

The pair is constructed to reproduce every difference the comparator has to
absorb, so that a PASS on it exercises each normalization rule:

  * rows in a different order (file B is a permutation of file A)
  * columns in a different order, as the two ioda writers produce them
  * float missing values written with a different sentinel
    (A as NULL, i.e. odc's -2147483647, B as ioda's odb_missing_float -2147483648)
  * integer missing values written as NULL in A and as 2147483647 in B
  * string missing values written as "MISSING*" in A and as NULL in B
  * a string longer than one 8-byte ODB word
  * a DOUBLE column holding values near 1e-20

The mutation cases then confirm the tool actually has teeth.  Two matter most:
'one column permuted' is invisible to per-column norms and hashes, and 'a 1e-20
value changed' is invisible to a comparison of 'odc sql --full_precision' text,
which prints both values as 0.00000000000000000.

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

HERE = os.path.dirname(os.path.abspath(__file__))
COMPARE = os.path.join(HERE, "ioda_compare_odb.py")

# Shared by mkdtemp and is_own_temp_directory so the two cannot drift apart.
TEMP_PREFIX = "ioda_compare_odb_selftest."

# The prefix of the directory 'ioda_compare_odb.py --dump' creates; it must
# match dump_tables there.  Used to check before removing one.
DUMP_PREFIX = "ioda_compare_odb_dump."

EPILOG = """
examples:
  Run the self-test:
    ioda_compare_odb_selftest.py

  Keep the generated files and read them in full, which is the point of a pair
  this small.  The directory they are in is printed at the end:
    ioda_compare_odb_selftest.py --keep
    odc sql 'select *' -i <printed directory>/selftest_obsgroup.odb
    odc sql 'select *' -i <printed directory>/selftest_osdf.odb

  Show the comparator's whole report for the matching pair:
    ioda_compare_odb_selftest.py -v

environment:
  Requires the odc executable, to write the test files, and whatever
  ioda_compare_odb.py requires.  --odc and --odc-lib default to odc on PATH
  and $odc_ROOT.

see also:
  ioda_compare_odb.py        the tool under test
"""

ODB_MISSING_FLOAT = -2147483648.0
ODB_MISSING_INT = 2147483647
MISSING_STRING = "MISSING*"

# The columns in the order file A stores them.  File B stores them in the order
# given by COLUMN_PERMUTATION.
COLUMNS = [
    ("statid", "STRING"),
    ("lat", "REAL"),
    ("lon", "REAL"),
    ("varno", "INTEGER"),
    ("obsvalue", "REAL"),
    ("qc", "INTEGER"),
    ("tiny", "DOUBLE"),
    ("longid", "STRING"),
]
COLUMN_PERMUTATION = [3, 7, 1, 0, 6, 2, 5, 4]

# Six rows, listed in the order file A stores them.  File B stores the same six
# in the order given by PERMUTATION.  Everything a reviewer needs to check the
# test by hand is in this table.  None is missing.
#
#   statid  lat   lon    varno obsvalue qc    tiny   longid
RECORDS = [
    ("aaa1", 10.0, 100.0, 2, 280.5, 0, 1e-20, "station-alpha"),
    ("bbb2", 20.0, 110.0, 2, None, 1, 2e-20, "station-bravo"),
    ("ccc3", 30.0, 120.0, 3, 282.5, None, 3e-20, "station-charlie"),
    ("ddd4", 40.0, 130.0, 3, 283.5, 2, 4e-20, None),
    ("eee5", 50.0, 140.0, 2, None, 0, 5e-20, "station-echo"),
    (None, 60.0, 150.0, 2, 285.5, 1, 6e-20, "station-foxtrot"),
]
PERMUTATION = [3, 0, 5, 2, 4, 1]


def _field(value, type_name, legacy):
    """Render one value as 'odc import' text, in the style of one of the writers."""
    if value is None:
        if legacy:
            return MISSING_STRING if type_name == "STRING" else "NULL"
        return {"STRING": "NULL", "INTEGER": str(ODB_MISSING_INT)}.get(
            type_name, repr(ODB_MISSING_FLOAT))
    if type_name in ("REAL", "DOUBLE"):
        return repr(float(value))
    return str(value)


def odb_text(columns, records, legacy):
    """The 'odc import' text for some rows: a name:TYPE header, then one line each.

    columns is a list of (name, type); each record holds its values in the
    canonical COLUMNS order and is reordered to match.
    """
    index = [column_index(name) for name, _ in columns]
    lines = [",".join("%s:%s" % column for column in columns)]
    lines += [",".join(_field(record[i], type_name, legacy)
                       for i, (_, type_name) in zip(index, columns))
              for record in records]
    return "\n".join(lines) + "\n"


def write_odb(odc, path, columns, records, legacy):
    """Write rows to an ODB file with 'odc import', keeping the text beside it."""
    text_path = path + ".txt"
    with open(text_path, "w") as text:
        text.write(odb_text(columns, records, legacy))
    result = subprocess.run([odc, "import", "-d", ",", text_path, path],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            universal_newlines=True)
    if result.returncode != 0:
        raise IOError("odc import failed for %s:\n%s" % (text_path, result.stdout))
    return path


def b_columns():
    return [COLUMNS[i] for i in COLUMN_PERMUTATION]


def b_records():
    return [list(RECORDS[i]) for i in PERMUTATION]


def column_index(name):
    return [n for n, _ in COLUMNS].index(name)


def _set_where(records, name, old, new):
    """Replace `old` by `new` in column `name`, wherever it occurs."""
    col = column_index(name)
    for record in records:
        if record[col] == old:
            record[col] = new


def _permute_one_column(columns, records):
    # Longitude alone is permuted, so every row keeps a valid-looking
    # longitude but is bound to the wrong latitude.  Per-column norms and
    # per-column hashes cannot see this.
    col = column_index("lon")
    for record, value in zip(records, [r[col] for r in records][::-1]):
        record[col] = value
    return columns, records


def _change_one_value(columns, records):
    # Latitude is an identity column, so this is a changed observation.
    records[2][column_index("lat")] += 0.25
    return columns, records


def _change_obsvalue(columns, records):
    # B's first row is ddd4; obsvalue is not an identity column.
    records[0][column_index("obsvalue")] += 1.0
    return columns, records


def _change_tiny(columns, records):
    records[0][column_index("tiny")] *= 1.1
    return columns, records


def _drop_missing(columns, records):
    _set_where(records, "obsvalue", None, 999.0)
    return columns, records


def _change_string(columns, records):
    records[1][column_index("statid")] = "zzz9"
    return columns, records


def _change_long_string(columns, records):
    # Only the second 8-byte word differs.
    _set_where(records, "longid", "station-alpha", "station-alphX")
    return columns, records


def _drop_column(columns, records):
    return [c for c in columns if c[0] != "qc"], records


def _change_type(columns, records):
    return [(n, "REAL" if n == "qc" else t) for n, t in columns], records


def _drop_row(columns, records):
    return columns, records[:-1]


# Each corruption of file B, with the label its case prints.  Every one must
# make the comparator report FAIL.
MUTATIONS = [
    ("permute_one_column", _permute_one_column,
     "one column permuted  <- norms are blind to this"),
    ("change_one_value", _change_one_value, "one value changed by 0.25"),
    ("change_obsvalue", _change_obsvalue, "a non-identity value changed by 1"),
    ("change_tiny", _change_tiny, "a 1e-20 value changed  <- odc sql text is blind"),
    ("drop_missing", _drop_missing, "a missing value replaced by real data"),
    ("change_string", _change_string, "one string changed"),
    ("change_long_string", _change_long_string, "second 8-byte word of a string changed"),
    ("drop_column", _drop_column, "a column missing from B"),
    ("change_type", _change_type, "a column declared REAL instead of INTEGER"),
    ("drop_row", _drop_row, "a row missing from B"),
]


def is_own_temp_directory(directory, prefix=TEMP_PREFIX):
    """True if `directory` is a scratch directory made by mkdtemp with `prefix`.

    A pure predicate, so the rmtree in main() can be checked against paths
    like "/" without any risk of deleting them.  Keep the realpath: it makes
    a symlink judged by where it points, not by its name.
    """
    resolved = os.path.realpath(directory)
    return (os.path.dirname(resolved) == os.path.realpath(tempfile.gettempdir())
            and os.path.basename(resolved).startswith(prefix))


def find_odc(explicit):
    """The odc executable: --odc, then PATH, then $odc_ROOT/bin."""
    if explicit:
        return explicit
    found = shutil.which("odc")
    if found:
        return found
    root = os.environ.get("odc_ROOT") or os.environ.get("ODC_ROOT")
    if root and os.path.exists(os.path.join(root, "bin", "odc")):
        return os.path.join(root, "bin", "odc")
    raise IOError("cannot find the odc executable; pass --odc")


def build_parser():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--keep", action="store_true",
                        help="leave the test files in their temporary directory, "
                             "and print where it is, so they can be inspected "
                             "with odc sql")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the comparator's full report for the good pair")
    parser.add_argument("--odc", metavar="PATH",
                        help="odc executable used to write the test files")
    parser.add_argument("--odc-lib", metavar="PATH",
                        help="libodccore passed through to ioda_compare_odb.py")
    return parser


class Fixture:
    """Where the test files go, and how to write them and run the comparator."""

    def __init__(self, odc, directory, odc_lib):
        self.odc = odc
        self.directory = directory
        self.lib_args = ["--odc-lib", odc_lib] if odc_lib else []

    def path(self, name):
        return os.path.join(self.directory, name)

    def write(self, name, columns, records, legacy):
        return write_odb(self.odc, self.path(name), columns, records, legacy)

    def run(self, path_a, path_b, extra=(), with_lib=True):
        """Run the comparator; return (exit_code, output)."""
        command = [sys.executable, COMPARE, path_a, path_b] + list(extra)
        if with_lib:
            command += self.lib_args
        result = subprocess.run(command, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, universal_newlines=True)
        return result.returncode, result.stdout


def matching_pair_cases(fixture, path_a, path_b, verbose):
    """The good pair must pass quietly, and report fully under -v."""
    # Each run of the comparator contributes a case for the exit code, and
    # some also one for the shape of the report it printed.
    code, output = fixture.run(path_a, path_b)
    cases = [("matching pair (permuted rows and columns, sentinels)", 0, code),
             ("  ...and a pass prints only the verdict, in 1 line",
              1, len(output.strip().splitlines()))]

    code, output = fixture.run(path_a, path_b, ["-v"])
    cases += [("matching pair again, with -v", 0, code),
              ("  ...and -v prints the full report, over 20 lines",
               True, len(output.splitlines()) > 20),
              ("  ...and reports the column order difference",
               True, "Column order (non-fatal)" in output)]
    if verbose:
        print(output)

    code, _ = fixture.run(path_a, path_a)
    cases.append(("identity control (A vs A)", 0, code))
    code, _ = fixture.run(path_a, path_b, ["--strict"])
    cases.append(("--strict on the matching pair  <- column order differs", 1, code))
    return cases


def write_mutated(fixture, kind):
    """Write file B with the named corruption from MUTATIONS; return its path."""
    mutate = {k: function for k, function, _ in MUTATIONS}[kind]
    columns, records = mutate(b_columns(), b_records())
    return fixture.write("selftest_osdf_mutated_%s.odb" % kind, columns, records, False)


def mutation_cases(fixture, path_a):
    """Every corruption in MUTATIONS must fail."""
    cases = []
    for kind, _, label in MUTATIONS:
        code, _ = fixture.run(path_a, write_mutated(fixture, kind))
        cases.append((label, 1, code))
    return cases


def diagnostic_cases(fixture, path_a):
    """A failure must say which observation differs, and how."""
    _, output = fixture.run(path_a, write_mutated(fixture, "change_obsvalue"))
    cases = [("non-identity change: row named by identity, not index",
              True, "[statid=ddd4" in output and "index " not in output)]

    _, output = fixture.run(path_a, write_mutated(fixture, "drop_missing"))
    cases.append(("missing vs present: the row is listed, shown as missing",
                  True, "A = missing" in output))

    # Without pairing by identity, the moved row would misalign others.
    _, output = fixture.run(path_a, write_mutated(fixture, "change_one_value"))
    cases.append(("changed identity: one row each side only, no cascade",
                  True, "1 row in A only" in output and "1 row in B only" in output
                  and "Columns that differ" not in output))

    # Two rows sharing an identity, told apart only by obsvalue, reordered.
    twin = list(RECORDS[0])
    twin[column_index("obsvalue")] = 290.5
    rows = [RECORDS[0], tuple(twin), RECORDS[1]]
    twin_a = fixture.write("twin_a.odb", COLUMNS, rows, True)
    twin_b = fixture.write("twin_b.odb", b_columns(), rows[::-1], False)
    code, _ = fixture.run(twin_a, twin_b)
    cases.append(("rows sharing an identity, reordered", 0, code))

    code, output = fixture.run(path_a, path_a, ["--key", "statid,no_such_column"])
    cases.append(("--key naming an unknown column is a usage error", 2, code))
    cases.append(("  ...which names the column", True, "--key: no_such_column" in output))
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
    directory = os.path.dirname(path_a)
    try:
        with open(path_a) as file_a, open(path_b) as file_b:
            lines_a, lines_b = file_a.readlines(), file_b.readlines()
    finally:
        if is_own_temp_directory(directory, DUMP_PREFIX):
            remove_directory(directory)
    return sum(x != y for x, y in zip(lines_a, lines_b)) + abs(len(lines_a) - len(lines_b))


def dump_cases(fixture, path_a):
    _, output = fixture.run(path_a, write_mutated(fixture, "change_obsvalue"), ["--dump"])
    return [("--dump: the sorted tables differ in exactly one line",
             1, dumped_line_differences(output))]


def file_set_cases(fixture, path_a):
    """Multi-file sets, duplicated rows and empty files."""
    # A multi-file set: B split across two IO-pool members, found by
    # expanding the base name, which does not exist itself.
    records = b_records()
    fixture.write("split_0000.odb", b_columns(), records[:4], False)
    fixture.write("split_0001.odb", b_columns(), records[4:], False)
    code, _ = fixture.run(path_a, fixture.path("split.odb"))
    cases = [("B split over split_0000/_0001, named by base", 0, code)]

    # Members of one set in different column orders.  The first member matches
    # A's order, so --strict can only fail on the difference within the set.
    fixture.write("mixed_0000.odb", COLUMNS, RECORDS[:4], True)
    fixture.write("mixed_0001.odb", b_columns(), RECORDS[4:], True)
    code, output = fixture.run(path_a, fixture.path("mixed.odb"), ["-v"])
    cases += [("set members in different column orders", 0, code),
              ("  ...and the difference within the set is reported",
               True, "mixed_0001.odb frame 1: column order differs" in output)]
    code, _ = fixture.run(path_a, fixture.path("mixed.odb"), ["--strict"])
    cases.append(("  ...and is fatal under --strict", 1, code))

    # Duplicated rows: the comparison is of multisets, so the count of each
    # distinct row matters, not only which rows occur.
    rows = [RECORDS[0], RECORDS[0], RECORDS[1]]
    dup_a = fixture.write("dup_a.odb", COLUMNS, rows, True)
    dup_b = fixture.write("dup_b.odb", COLUMNS, rows[::-1], True)
    dup_c = fixture.write("dup_c.odb", COLUMNS, [RECORDS[0], RECORDS[1], RECORDS[1]], True)
    code, _ = fixture.run(dup_a, dup_b)
    cases.append(("duplicated rows, reordered", 0, code))
    code, _ = fixture.run(dup_a, dup_c)
    cases.append(("same rows, different duplicate counts", 1, code))

    # odc writes a zero-byte file for a table with no rows.
    empty_a = fixture.write("empty_a.odb", COLUMNS, [], True)
    empty_b = fixture.write("empty_b.odb", b_columns(), [], False)
    code, _ = fixture.run(empty_a, empty_b)
    cases.append(("two empty files", 0, code))
    code, _ = fixture.run(empty_a, path_a)
    cases.append(("an empty file against a full one", 1, code))
    return cases


def guard_cases(fixture, path_a, path_b):
    """Usage errors, and the cleanup guard."""
    code, _ = fixture.run(path_a, path_b,
                          ["--odc-lib", fixture.path("no_such_libodccore.so")],
                          with_lib=False)
    cases = [("an unloadable odc library is a usage error", 2, code)]

    # A file list of only commas names no file.  It must be a usage error, not
    # a traceback, whose exit code 1 would read as "files differ".
    for spec in (",", ", ,"):
        code, output = fixture.run(spec, path_b)
        cases.append(("file list %-5r is a usage error" % spec, 2, code))
        cases.append(("  ...reported, not a traceback",
                      True, "ERROR: no file named in" in output and "Traceback" not in output))

    # These paths are only spelled, never touched -- the predicate deletes
    # nothing, which is what makes asking about them safe.
    unsafe = ["/", "/home", os.path.expanduser("~"), tempfile.gettempdir(), "", "."]
    scratch = os.path.join(tempfile.gettempdir(), TEMP_PREFIX + "abc123")
    cases += [("cleanup guard rejects /, /home, $HOME, tmp, '' and .",
               0, sum(is_own_temp_directory(p) for p in unsafe)),
              ("  ...and accepts a mkdtemp scratch directory",
               True, is_own_temp_directory(scratch))]
    return cases + setup_failure_cases(fixture)


def setup_failure_cases(fixture):
    """A self-test whose setup fails must still remove its scratch directory.

    A child copy of this script is pointed at an odc that does not exist, so
    writing its first fixture fails.  TMPDIR gives the child an empty temporary
    directory of its own, so whatever it leaves behind is found exactly, even
    with other tests running at the same time.
    """
    probe = fixture.path("tmpdir_probe")
    os.mkdir(probe)
    result = subprocess.run([sys.executable, os.path.abspath(__file__),
                             "--odc", fixture.path("no_such_odc")],
                            env=dict(os.environ, TMPDIR=probe),
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            universal_newlines=True)
    return [("a self-test whose setup fails is a usage error", 2, result.returncode),
            ("  ...and leaves no scratch directory behind", [], os.listdir(probe))]


def print_cases(cases):
    """Print the results table and return the number of cases that went wrong.

    A case is either an expected exit code or an expected boolean assertion
    about the report text, so both are rendered as strings.
    """
    print("%-62s %-8s %-8s %s" % ("case", "expect", "actual", "result"))
    print("-" * 92)
    failures = 0
    for label, expected, actual in cases:
        ok = expected == actual
        failures += 0 if ok else 1
        print("%-62s %-8s %-8s %s" % (label, expected, actual, "ok" if ok else "WRONG"))
    print()
    return failures


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
        for name in ("selftest_obsgroup.odb", "selftest_osdf.odb"):
            print("  odc sql 'select *' -i %s" % os.path.join(directory, name))
    elif is_own_temp_directory(directory):
        remove_directory(directory)
    else:
        print("REFUSING to delete %s: not a %s* scratch directory"
              % (directory, TEMP_PREFIX), file=sys.stderr)


def main():
    args = build_parser().parse_args()
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
        fixture = Fixture(find_odc(args.odc), directory, args.odc_lib)
        path_a = fixture.write("selftest_obsgroup.odb", COLUMNS, RECORDS, True)
        path_b = fixture.write("selftest_osdf.odb", b_columns(), b_records(), False)
    except (IOError, OSError) as err:
        print("ERROR: %s" % err, file=sys.stderr)
        return 2

    print("ioda_compare_odb self-test")
    print("  %d rows x %d columns = %d values -- small enough to read with odc sql"
          % (len(RECORDS), len(COLUMNS), len(RECORDS) * len(COLUMNS)))
    print("  A: %s   (NULL float/int missing, MISSING* strings)"
          % os.path.basename(path_a))
    print("  B: %s   (ioda sentinels, NULL strings, rows permuted %s,"
          " columns permuted %s)"
          % (os.path.basename(path_b), PERMUTATION, COLUMN_PERMUTATION))
    print()

    try:
        cases = matching_pair_cases(fixture, path_a, path_b, args.verbose)
        cases += mutation_cases(fixture, path_a)
        cases += diagnostic_cases(fixture, path_a)
        cases += dump_cases(fixture, path_a)
        cases += file_set_cases(fixture, path_a)
        cases += guard_cases(fixture, path_a, path_b)
    except (IOError, OSError) as err:
        print("ERROR: %s" % err, file=sys.stderr)
        return 2

    failures = print_cases(cases)

    if failures:
        print("SELF-TEST FAILED: %d of %d cases behaved unexpectedly"
              % (failures, len(cases)))
        return 1
    print("SELF-TEST PASSED: all %d cases behaved as expected" % len(cases))
    return 0


if __name__ == "__main__":
    sys.exit(main())
