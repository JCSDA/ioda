#!/usr/bin/env python3

# (C) Copyright 2026 UCAR
#
# This software is licensed under the terms of the Apache Licence Version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

r"""Compare two ODB files for equivalence, independent of row and column order.

The ODB counterpart of ioda_compare_obs.py: it checks files produced by the OSDF
container writer against those produced by the legacy ObsGroup container.  Both
containers write through ODC::createFile, but the two facades list their
variables in a different order, so the columns come out in a different order,
and rows follow the container's location order, which can also differ.

Method: identity-first sort, then pairing by identity
  An ODB file is a single flat table, in a long layout: one row per location,
  varno, and level or channel.  Columns are matched by name, then each file is
  sorted by a key made of the ENTIRE row, with the columns that identify an
  observation first (IDENTITY_COLUMNS, or --key) and every other column after
  them.  Walking the two sorted files together then pairs the rows whose
  identity matches; a row with no partner is reported as present on one side
  only, and the remaining columns are compared over the paired rows.

  The identity columns lead so that a difference shows up where it is: a
  changed value in a non-identity column can only reorder rows that share an
  identity, and a changed identity is one observation on each side only, not
  a shift that misaligns every row after it.  No small set of columns is unique
  -- the real sonde outputs have rows that share an identity, and even exact
  duplicate rows -- which is why the rest of the row still breaks ties.  Two
  file sets therefore match only if they hold the same multiset of rows,
  exactly as with a plain full-row sort.

  --dump writes both sorted tables as text, identity columns first, so that an
  ordinary diff of the two shows just the rows that differ.

Reading: the odc C library, not odc sql
  The files are decoded with libodccore through ctypes, which yields the stored
  binary values.  (The obvious alternative, 'odc sql --full_precision', prints a
  fixed 17 decimal places rather than 17 significant digits.) pyodc, the official
  Python binding, is not part of spack-stack, so the library is called directly.

Normalization rules.  Every column becomes one of three kinds plus a missing mask:

  INTEGER, BITFIELD -> int     exact int64; missing when == odb_missing_int
                               (2147483647)
  REAL, DOUBLE      -> float   float64; missing when not finite or equal to
                               odb_missing_float (-2147483648, what ioda
                               writes) or odc's default missing double
                               (-2147483647, what odc itself writes)
  STRING            -> string  bytes with trailing NUL and blank padding
                               stripped, decoded as latin-1 so that no two
                               distinct byte strings collide; missing when
                               empty, "MISSING*", or the byte image of a
                               missing double (odc writes NULL strings so)

  The odc decoder returns each column's stored missing value rather than the
  API's, which is why the sentinels are matched here rather than read back from
  the library.

Fatal vs reported:
  Always fatal: a column present on one side only, a column whose declared type
  differs between the two sides, a row count mismatch, a row with no partner
  of the same identity on the other side, and any data difference.
  Column order -- between the two sides, or between the frames or member files
  of one side -- is reported but not fatal, because the two writers differ
  there by design.  --strict makes it fatal, which is how to ask whether that
  gap has closed.  Codecs are never compared: odc picks one per frame from the
  data it holds, so they depend on row order.

Multi-file sets:
  With "write multiple files: true" the IO pool writes one file per pool rank,
  suffixed _0000, _0001, ... before the last dot.  The rows of every frame of
  every member are concatenated before sorting.

--tolerance applies to floats only, integers always being exact, and relaxes the
comparison but not the sort, which needs bit-exact key values; a ULP-level
divergence would cascade into many failures rather than be absorbed.

Output:
  A pass prints just the verdict; a failure prints only the reasons for it.
  -v prints the full report of every column that was checked, pass or fail.
  A row is named by its identity, e.g. [statid=07510 varno=2 ...], rather
  than by its position in the sort, which means nothing to a reader.

Exit codes:
  0  files match
  1  files differ (the reasons are printed to stdout)
  2  usage / I/O error, including a libodccore that cannot be found or loaded

This function was created using Generative AI (Claude), and is Quality Tier 2 under the JCSDA
generative AI policy. As Tier 2, it should only be used as a validation tool, and never as part of
production code.
"""

import sys
import os
import struct
import ctypes
import ctypes.util
import argparse
import shutil
import tempfile
from collections import namedtuple

import numpy as np

# The sort, the column comparison and the file-set expansion are shared with
# the netCDF comparator, so that the two tools cannot drift apart on them.
from ioda_compare_obs import (Column, resolve_file_set, summarize, compare_column,
                              plural, align_by_identity, identity_label,
                              unmatched_lines, take, cell_texts, write_tables)


# OdbConstants.h, and odc's own API defaults.  See the module docstring.
ODB_MISSING_INT = 2147483647
ODB_MISSING_FLOATS = (
    -2147483648.0,   # odb_missing_float, written by ioda
    -2147483647.0,   # odc's default missing double, written by odc tools
)
MISSING_STRING = "MISSING*"
# A NULL string written by odc is the byte image of the missing double, in
# the host byte order the decoder returns.
MISSING_STRING_BYTES = tuple(struct.pack("=d", v) for v in ODB_MISSING_FLOATS)

# enum OdcColumnType in odc/api/odc.h
ODC_TYPES = {1: "INTEGER", 2: "REAL", 3: "STRING", 4: "BITFIELD", 5: "DOUBLE"}
TYPE_KIND = {"INTEGER": "int", "BITFIELD": "int",
             "REAL": "float", "DOUBLE": "float", "STRING": "string"}

# enum OdcErrorValues and the integer behaviour flags in odc/api/odc.h
ODC_SUCCESS = 0
ODC_ITERATION_COMPLETE = 1
ODC_INTEGERS_AS_LONGS = 2

# The columns that, between them, identify one observation in the long layout:
# the station or platform, the time and place, the variable (varno), and the
# level or channel.  Those a file lacks are skipped; --key replaces the list.
IDENTITY_COLUMNS = ("statid", "date", "time", "lat", "lon", "seqno", "varno",
                    "initial_vertco_reference", "vertco_reference_1", "chans", "level")

# Rows are the only dimension of an ODB table.  The Column tuples carry it so
# that the shared helpers see the shape they expect.
ROW_DIMS = ("row",)


class OdcLibrary:
    """The few libodccore entry points needed to decode a file, via ctypes."""

    def __init__(self, path):
        self.path = path
        lib = ctypes.CDLL(path)
        vp, ip, lp = ctypes.c_void_p, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_long)
        pp = ctypes.POINTER(ctypes.c_void_p)
        cpp = ctypes.POINTER(ctypes.c_char_p)
        # Declaring every signature keeps 64-bit pointers from being truncated
        # to a C int, which ctypes does by default.
        for name, argtypes in (
                ("odc_initialise_api", []),
                ("odc_integer_behaviour", [ctypes.c_int]),
                ("odc_open_path", [pp, ctypes.c_char_p]),
                ("odc_close", [vp]),
                ("odc_new_frame", [pp, vp]),
                ("odc_free_frame", [vp]),
                ("odc_next_frame", [vp]),
                ("odc_frame_row_count", [vp, lp]),
                ("odc_frame_column_count", [vp, ip]),
                ("odc_frame_column_attributes", [vp, ctypes.c_int, cpp, ip, ip, ip]),
                ("odc_frame_bitfield_attributes", [vp, ctypes.c_int, ctypes.c_int,
                                                   cpp, ip, ip]),
                ("odc_new_decoder", [pp]),
                ("odc_free_decoder", [vp]),
                ("odc_decoder_defaults_from_frame", [vp, vp]),
                ("odc_decode", [vp, vp, lp]),
                ("odc_decoder_column_data_array", [vp, ctypes.c_int, ip, ip, pp])):
            function = getattr(lib, name)
            function.argtypes = argtypes
            function.restype = ctypes.c_int
        lib.odc_error_string.argtypes = [ctypes.c_int]
        lib.odc_error_string.restype = ctypes.c_char_p
        self.lib = lib
        self.check(lib.odc_initialise_api(), "initialise the odc API")
        # Integers as doubles would round values above 2**53, the same trap
        # ioda_compare_obs.py avoids by never widening integers to float64.
        self.check(lib.odc_integer_behaviour(ODC_INTEGERS_AS_LONGS),
                   "select 64-bit integers")

    def check(self, code, what):
        if code != ODC_SUCCESS:
            message = self.lib.odc_error_string(code)
            raise IOError("odc: cannot %s: %s"
                          % (what, message.decode("utf-8", "replace") if message else code))

    def read_frames(self, path):
        """Yield (schema, arrays) for each frame of a file.

        schema is a list of (column name, declared type) in file order, and
        arrays maps each name to a copy of its decoded values.
        """
        lib = self.lib
        reader = ctypes.c_void_p()
        self.check(lib.odc_open_path(ctypes.byref(reader), path.encode()),
                   "open %s" % path)
        frame = ctypes.c_void_p()
        try:
            self.check(lib.odc_new_frame(ctypes.byref(frame), reader),
                       "create a frame for %s" % path)
            while True:
                code = lib.odc_next_frame(frame)
                if code == ODC_ITERATION_COMPLETE:
                    return
                self.check(code, "advance to the next frame of %s" % path)
                yield self._decode_frame(frame, path)
        finally:
            if frame:
                lib.odc_free_frame(frame)
            lib.odc_close(reader)

    def _decode_frame(self, frame, path):
        lib = self.lib
        ncols = ctypes.c_int()
        self.check(lib.odc_frame_column_count(frame, ctypes.byref(ncols)),
                   "count the columns of %s" % path)

        decoder = ctypes.c_void_p()
        self.check(lib.odc_new_decoder(ctypes.byref(decoder)), "create a decoder")
        try:
            self.check(lib.odc_decoder_defaults_from_frame(decoder, frame),
                       "configure a decoder for %s" % path)
            nrows = ctypes.c_long()
            self.check(lib.odc_decode(decoder, frame, ctypes.byref(nrows)),
                       "decode %s" % path)

            schema, arrays = [], {}
            for col in range(ncols.value):
                name = ctypes.c_char_p()
                ctype, size, nbits = ctypes.c_int(), ctypes.c_int(), ctypes.c_int()
                self.check(lib.odc_frame_column_attributes(
                    frame, col, ctypes.byref(name), ctypes.byref(ctype),
                    ctypes.byref(size), ctypes.byref(nbits)),
                    "read column %d of %s" % (col, path))
                name = name.value.decode("utf-8", "replace")
                type_name = ODC_TYPES.get(ctype.value, "type%d" % ctype.value)
                if type_name == "BITFIELD":
                    type_name += self._bitfield_layout(frame, col, nbits.value, path)

                element, stride = ctypes.c_int(), ctypes.c_int()
                data = ctypes.c_void_p()
                self.check(lib.odc_decoder_column_data_array(
                    decoder, col, ctypes.byref(element), ctypes.byref(stride),
                    ctypes.byref(data)), "locate column %s of %s" % (name, path))

                kind = TYPE_KIND.get(type_name.split("[")[0])
                if kind is None:
                    raise IOError("column %s of %s has unsupported type %s"
                                  % (name, path, type_name))
                dtype = {"int": np.int64, "float": np.float64}.get(
                    kind, "S%d" % element.value)
                count = nrows.value
                if count == 0:
                    values = np.zeros(0, dtype=dtype)
                else:
                    # The decoder owns this buffer, and it is freed below, so
                    # the strided view is copied out before then.
                    extent = stride.value * (count - 1) + element.value
                    raw = ctypes.string_at(data.value, extent)
                    values = np.ndarray((count,), dtype=dtype, buffer=raw,
                                        strides=(stride.value,)).copy()
                if name in arrays:
                    raise IOError("column %s appears twice in %s" % (name, path))
                schema.append((name, type_name))
                arrays[name] = values
            return schema, arrays
        finally:
            lib.odc_free_decoder(decoder)

    def _bitfield_layout(self, frame, col, count, path):
        """'[name:size;...]', so that two bitfields compare equal only if laid out alike."""
        fields = []
        for entry in range(count):
            name = ctypes.c_char_p()
            offset, size = ctypes.c_int(), ctypes.c_int()
            self.check(self.lib.odc_frame_bitfield_attributes(
                frame, col, entry, ctypes.byref(name), ctypes.byref(offset),
                ctypes.byref(size)), "read a bitfield of %s" % path)
            fields.append("%s:%d" % (name.value.decode("utf-8", "replace"), size.value))
        return "[%s]" % ";".join(fields)


def odc_install_roots():
    """Install prefixes that may hold libodccore: $odc_ROOT, then odc on PATH."""
    roots = [os.environ.get(variable) for variable in ("odc_ROOT", "ODC_ROOT")]
    tool = shutil.which("odc")
    if tool:
        roots.append(os.path.dirname(os.path.dirname(os.path.realpath(tool))))
    return [root for root in roots if root]


def find_odc_library(explicit):
    """Locate libodccore: --odc-lib, then the install roots, then ldconfig."""
    if explicit:
        return explicit
    candidates = [os.path.join(root, libdir, name)
                  for root in odc_install_roots()
                  for libdir in ("lib", "lib64")
                  for name in ("libodccore.so", "libodccore.dylib")]
    found = next((path for path in candidates if os.path.exists(path)), None)
    found = found or ctypes.util.find_library("odccore")
    if not found:
        raise IOError("cannot find libodccore; pass --odc-lib, or set odc_ROOT")
    return found


def canonical_column(type_name, raw):
    """Apply the normalization rules table to one column's decoded values."""
    kind = TYPE_KIND[type_name.split("[")[0]]
    if kind == "int":
        missing = raw == ODB_MISSING_INT
        values = np.where(missing, 0, raw).astype(np.int64)
    elif kind == "float":
        missing = ~np.isfinite(raw)
        for sentinel in ODB_MISSING_FLOATS:
            missing = missing | (raw == sentinel)
        values = np.where(missing, 0.0, raw)
    else:
        # A NULL written by odc is recognized on the raw bytes, before padding
        # is stripped: its trailing byte image could otherwise lose characters.
        words = [bytes(v) for v in raw.astype(object)]
        nulls = np.array([w.ljust(8, b"\x00")[:8] in MISSING_STRING_BYTES
                          for w in words], dtype=bool)
        text = np.array([w.rstrip(b"\x00").rstrip(b" ").decode("latin-1")
                         for w in words], dtype=object).astype("U")
        missing = nulls | (text == "") | (text == MISSING_STRING)
        values = np.where(missing, "", text)
    return Column(kind, values, missing, ROW_DIMS, type_name)


def load_file_set(library, paths):
    """Read a file set and return (columns, nrows, column_order, order_notes, types, structural).

    Every frame of every member is concatenated, by column name, so the column
    order of a frame does not affect the data.  A column that some frame lacks,
    or declares with a different type, cannot be concatenated, so it is
    reported as structural and left out.  column_order is that of the first
    frame, and order_notes names any later frame that orders the columns
    differently.
    """
    frames = read_all_frames(library, paths)
    column_order = [name for name, _ in frames[0][1]] if frames else []
    order_notes = frame_order_notes(frames)
    every_name = {name for _, schema, _ in frames for name, _ in schema}

    types, structural = {}, []
    for name in sorted(every_name):
        type_name, problem = column_type(name, frames)
        if problem:
            structural.append(problem)
        else:
            types[name] = type_name

    nrows = sum(len(next(iter(arrays.values()))) for _, _, arrays in frames if arrays)
    columns = {name: canonical_column(type_name,
                                      np.concatenate([arrays[name] for _, _, arrays in frames]))
               for name, type_name in types.items()}
    return columns, nrows, column_order, order_notes, types, structural


def frame_order_notes(frames):
    """A line for each frame whose column order differs from the set's first frame.

    Only the columns the two frames share are compared: a column missing from
    a frame is already reported, as structural.
    """
    if not frames:
        return []
    first_label, first_schema, _ = frames[0]
    first = [name for name, _ in first_schema]
    notes = []
    for label, schema, _ in frames[1:]:
        names = [name for name, _ in schema]
        shared = set(first) & set(names)
        if [n for n in names if n in shared] != [n for n in first if n in shared]:
            notes.append("%s: column order differs from %s" % (label, first_label))
    return notes


def read_all_frames(library, paths):
    """Every frame of every file in the set, in order, as (label, schema, arrays).

    A file with no rows can be zero bytes long; odc reads it as having no
    frames, so it contributes nothing, which is the right meaning.
    """
    return [("%s frame %d" % (path, index + 1), schema, arrays)
            for path in paths
            for index, (schema, arrays) in enumerate(library.read_frames(path))]


def column_type(name, frames):
    """(declared type, None) if every frame declares `name` alike, else (None, problem)."""
    absent = [label for label, schema, _ in frames if name not in dict(schema)]
    if absent:
        return None, ("%s: missing from %d of %d frames (first: %s)"
                      % (name, len(absent), len(frames), absent[0]))
    declared = {dict(schema)[name] for _, schema, _ in frames}
    if len(declared) > 1:
        return None, ("%s: declared as %s in different frames of the set"
                      % (name, " and ".join(sorted(declared))))
    return declared.pop(), None


def all_order_lines(a, b, common):
    """Every column order difference: within each file set, then between them."""
    return (["A: %s" % note for note in a.order_notes]
            + ["B: %s" % note for note in b.order_notes]
            + column_order_lines(a.order, b.order, common))


def column_order_lines(order_a, order_b, common):
    """Report how the column order differs, over the columns both sides have."""
    listed_a = [n for n in order_a if n in common]
    listed_b = [n for n in order_b if n in common]
    if listed_a == listed_b:
        return []
    moved = [n for n, m in zip(listed_a, listed_b) if n != m]
    lines = ["%d of %d common columns are in a different position"
             % (len(moved), len(listed_a))]
    lines.append("A: %s" % ", ".join(listed_a))
    lines.append("B: %s" % ", ".join(listed_b))
    return lines


def failure_report(structural, unmatched, data_lines, order_lines, strict):
    """The reasons a comparison failed, and nothing else.

    This is what a default (non-verbose) run prints when the files differ.  The
    column order section appears only under --strict, the only time it
    contributes to the verdict.
    """
    lines = []
    if structural:
        lines.append("Structural differences:")
        lines.extend("  %s" % line for line in structural)
    if unmatched:
        lines.append("Rows on one side only:")
        lines.extend("  %s" % line for line in unmatched)
    if data_lines:
        lines.append("Columns that differ:")
        for _, rendered in data_lines:
            lines.extend(rendered)
    if strict and order_lines:
        lines.append("Column order differs (fatal under --strict):")
        lines.extend("  %s" % line for line in order_lines)
    return lines


EPILOG = """
examples:
  Compare a legacy ObsGroup output against an OSDF output:
    ioda_compare_odb.py testoutput/diagout-sonde.odc testoutput/diagout-sonde-osdf.odc

  Compare multi-file sets written by a 2-rank IO pool.  A base name is expanded
  to the _0000.._000N members only when the base file does not exist, so name
  the members explicitly if both forms are present:
    ioda_compare_odb.py diagout-aod-multifiles.odc diagout-aod-multifiles-osdf.odc
    ioda_compare_odb.py a_0000.odc,a_0001.odc b_0000.odc,b_0001.odc

  See everything that was checked, not just the verdict:
    ioda_compare_odb.py a.odc b.odc -v

  Allow a small numeric difference, and show more of each mismatch:
    ioda_compare_odb.py a.odc b.odc -T 1.0e-6 -n 25

  Name the columns that identify an observation, for an obs type the default
  list does not suit:
    ioda_compare_odb.py a.odc b.odc --key statid,date,time,varno,level

  Write both sorted tables as text, to see the differing rows with diff:
    ioda_compare_odb.py a.odc b.odc --dump

  Require the columns to be in the same order as well.  The two writers order
  them differently, so this will normally fail:
    ioda_compare_odb.py a.odc b.odc --strict

environment:
  Requires numpy, the odc library (libodccore), and ioda_compare_obs.py next to
  this script.  The library is found through --odc-lib, then $odc_ROOT, then an
  odc executable on PATH.

see also:
  ioda_compare_odb_selftest.py  self-check on tiny hand-verifiable pairs
  ioda_compare_obs.py           the netCDF equivalent
  compare-odbs                  in-order comparison against a reference file
"""


def build_parser():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog=EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("files_a", metavar="FILES_A",
                        help="first file set: a path, 'a.odc,b.odc', or a base "
                             "path expanded to base_0000.odc, base_0001.odc, ...")
    parser.add_argument("files_b", metavar="FILES_B",
                        help="second file set (same forms)")
    parser.add_argument("-T", "--tolerance", type=float, default=0.0, metavar="TOL",
                        help="absolute tolerance for numeric values (default: 0.0). "
                             "Relaxes the comparison, not the sort.")
    parser.add_argument("-n", "--max-diffs", type=int, default=5, metavar="N",
                        help="show at most N differing rows per column (default: 5)")
    parser.add_argument("--strict", action="store_true",
                        help="also fail when the columns are in a different order, "
                             "which is reported but not fatal by default because "
                             "the two writers differ there by design")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the full report of everything that was checked. "
                             "By default a pass prints only the verdict and a failure "
                             "prints only the reasons for it.")
    parser.add_argument("--key", metavar="COLS",
                        help="comma-separated columns that identify an observation, "
                             "sorted on first and used to pair rows (default: those "
                             "of %s present in both file sets)" % ",".join(IDENTITY_COLUMNS))
    parser.add_argument("--dump", action="store_true",
                        help="write both sorted tables as text to a new temporary "
                             "directory, and print a diff command for them")
    parser.add_argument("--odc-lib", metavar="PATH",
                        help="path to libodccore (default: found through $odc_ROOT "
                             "or an odc executable on PATH)")
    return parser


FileSet = namedtuple("FileSet", ["paths", "columns", "nrows", "order", "order_notes",
                                 "types", "structural"])


def section(heading, lines):
    """A report section: the heading, its indented lines, a blank; or nothing."""
    if not lines:
        return []
    return [heading] + ["  %s" % line for line in lines] + [""]


def structural_differences(a, b, common):
    """Everything that is fatal regardless of the flags."""
    structural = ["A: %s" % line for line in a.structural]
    structural += ["B: %s" % line for line in b.structural]
    structural += ["%s: column present in A only" % name
                   for name in sorted(set(a.columns) - set(b.columns))]
    structural += ["%s: column present in B only" % name
                   for name in sorted(set(b.columns) - set(a.columns))]
    structural += ["%s: declared %s in A but %s in B" % (name, a.types[name], b.types[name])
                   for name in common if a.types[name] != b.types[name]]
    if a.nrows != b.nrows:
        structural.append("row count %d (A) != %d (B)" % (a.nrows, b.nrows))
    return structural


def identity_columns(requested, a, b, common):
    """The identity key: the --key columns, or those of IDENTITY_COLUMNS present.

    Only a column both sides declare alike can key the sort; a type mismatch is
    already structural.
    """
    usable = [n for n in common if a.columns[n].kind == b.columns[n].kind]
    if requested is None:
        return [n for n in IDENTITY_COLUMNS if n in usable]
    names = [n.strip() for n in requested.split(",") if n.strip()]
    unknown = [n for n in names if n not in usable]
    if unknown:
        raise ValueError("--key: %s not a column both file sets declare alike"
                         % ", ".join(unknown))
    return names


def align_rows(a, b, common, identity):
    """Sort both sides identity first, then pair the rows whose identity matches."""
    rest = [n for n in common if n not in identity and a.columns[n].kind == b.columns[n].kind]
    return align_by_identity(a.columns, a.nrows, b.columns, b.nrows, identity, rest)


def sort_report(a, b, identity, common, alignment):
    """The report lines describing the sort and the pairing."""
    key = ", ".join(identity) if identity else "(none -- rows paired in full-row order)"
    return ["Canonical row sort",
            "  identity key = %s" % key,
            "  then the other %s" % plural(len(common) - len(identity), "common column"),
            "  A: %d of %d rows have distinct identities" % (alignment.distinct_a, a.nrows),
            "  B: %d of %d rows have distinct identities" % (alignment.distinct_b, b.nrows),
            "  %s paired by identity, %d in A only, %d in B only"
            % (plural(len(alignment.rows_a), "row"), len(alignment.only_a), len(alignment.only_b)),
            ""]


def column_result(name, a, b, alignment, identity, args):
    """(ok, report lines) for one column, compared over the paired rows."""
    ca, cb = a.columns[name], b.columns[name]
    rows_a, rows_b = alignment.rows_a, alignment.rows_b

    def row_label(position):
        return identity_label(a.columns, identity, rows_a[position])

    ok, details = compare_column(take(ca, rows_a), take(cb, rows_b), None, None,
                                 args.tolerance, args.max_diffs, row_label)
    if ok:
        return True, [("  %-4s %-40s %-9s %s"
                       % ("PASS", name, a.types[name], summarize(ca))).rstrip()]
    return False, ([("  %-4s %-40s %-9s" % ("DIFF", name, a.types[name])).rstrip(),
                    "       A: %s" % summarize(ca),
                    "       B: %s" % summarize(cb)]
                   + ["       %s" % line for line in details])


def compare_columns(a, b, common, alignment, identity, args):
    """Compare every common column over the paired rows.

    Returns (number passed, [(name, lines)] for each failure, report lines).
    The failures keep their rendered lines, which are what the default
    (non-verbose) run prints: on a failure the user should see why, without
    the whole report around it.
    """
    passed, failures, report = 0, [], []
    for name in common:
        ok, lines = column_result(name, a, b, alignment, identity, args)
        report.extend(lines)
        if ok:
            passed += 1
        else:
            failures.append((name, lines))
    if common:
        report = (["Columns (compared over the %s paired by identity)"
                   % plural(len(alignment.rows_a), "row")] + report + [""])
    return passed, failures, report


def dump_tables(a, b, common, identity, alignment):
    """Write both sorted tables to a new temporary directory; return the paths."""
    names = identity + [n for n in common if n not in identity]
    return write_tables("ioda_compare_odb_dump.", [
        (label, names, [cell_texts(side.columns[n], order) for n in names])
        for label, side, order in (("A", a, alignment.sort_a), ("B", b, alignment.sort_b))])


def header_lines(a, b, library, common):
    return ["ODB comparison",
            "  A: %s (%s)" % (a.paths[0], plural(len(a.paths), "file")),
            "  B: %s (%s)" % (b.paths[0], plural(len(b.paths), "file")),
            "  odc library: %s" % library.path,
            "",
            "Rows:    A = %d   B = %d" % (a.nrows, b.nrows),
            "Columns: %d in A, %d in B, %d in common"
            % (len(a.columns), len(b.columns), len(common)),
            ""]


def summary_lines(passed, failures, structural, alignment, order_lines, fatality):
    return ["Summary",
            "  columns                %3d pass  %3d fail" % (passed, len(failures)),
            "  rows on one side only  %3d in A  %3d in B  (fatal)"
            % (len(alignment.only_a), len(alignment.only_b)),
            "  structural differences %3d            (fatal)" % len(structural),
            "  column order           %-12s   (%sfatal)"
            % ("differs" if order_lines else "same", fatality),
            ""]


def load_side(library, paths):
    return FileSet(paths, *load_file_set(library, paths))


def main():
    # ---- 1. Parse arguments and load both sides ------------------------------
    args = build_parser().parse_args()
    try:
        paths_a = resolve_file_set(args.files_a)
        paths_b = resolve_file_set(args.files_b)
        library = OdcLibrary(find_odc_library(args.odc_lib))
        a = load_side(library, paths_a)
        b = load_side(library, paths_b)
        common = sorted(set(a.columns) & set(b.columns))
        identity = identity_columns(args.key, a, b, common)
    except (IOError, OSError, ValueError) as err:
        print("ERROR: %s" % err, file=sys.stderr)
        return 2

    # ---- 2. Compare ----------------------------------------------------------
    structural = structural_differences(a, b, common)
    alignment = align_rows(a, b, common, identity)
    unmatched = unmatched_lines(a.columns, b.columns, identity, alignment, args.max_diffs)
    passed, failures, column_report = compare_columns(a, b, common, alignment, identity, args)
    order_lines = all_order_lines(a, b, set(common))

    # ---- 3. Assemble the full report -----------------------------------------
    # It is accumulated rather than printed as it is produced, because by
    # default only the verdict, or the reasons for a failure, is printed.
    fatality = "" if args.strict else "non-"
    report = header_lines(a, b, library, common) + sort_report(a, b, identity, common, alignment)
    report += section("Rows on one side only (fatal)", unmatched) + column_report
    report += section("Structural differences (fatal)", structural)
    report += section("Column order (%sfatal)" % fatality, order_lines)
    report += summary_lines(passed, failures, structural, alignment, order_lines, fatality)

    # ---- 4. Decide the verdict and print -------------------------------------
    failed = bool(structural or unmatched or failures or (args.strict and order_lines))
    if args.verbose:
        print("\n".join(line.rstrip() for line in report))
    elif failed:
        reasons = failure_report(structural, unmatched, failures, order_lines, args.strict)
        print("\n".join(reasons))
    if args.dump:
        path_a, path_b = dump_tables(a, b, common, identity, alignment)
        print("Sorted rows written for inspection; compare them with:")
        print("  diff %s %s" % (path_a, path_b))

    if failed:
        print("FAIL: %s vs %s" % (args.files_a, args.files_b))
        return 1
    print("PASS: both file sets contain the same %d ODB rows." % a.nrows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
