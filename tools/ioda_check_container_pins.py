#!/usr/bin/env python3
"""Check that no ioda test obs space relies on the 'use data frame container' default.

While ioda migrates from the ObsGroup container to the OSDF (IFrame) containers, the
parameter default in src/ObsSpaceParameters.h is going to change. Any obs space that does
not state its container would silently move to the other one, and because most tests pass
under either container, ctest would stay green while the coverage disappeared.

Two ways for a test to state its container are legitimate, and this script accepts both:

  * the YAML names it -- either per obs space ("use data frame container"), or once for
    the whole file ("obs data container"). The section-level key is read from the
    "observations" mapping by oops::ObsSpaces, but from the top level of the file by
    ioda::test::applyContainerDefault and by the deprecated-sequence path in
    oops ObsTestsFixture. Which one applies depends on the driver, and a key in the wrong
    place is a silent no-op, so the placement is checked against the file's shape.

  * the ctest registration names it, by setting IODA_TEST_CONTAINER. That is how one
    container-agnostic YAML gets registered twice, once per container. Such a file must
    NOT also name a container, because both YAML forms outrank the environment variable
    and would quietly collapse the pair onto one container.

Run it with no arguments, from anywhere:

    ioda/tools/ioda_check_container_pins.py

TODO(someone): delete this check, along with applyContainerDefault and the
IODA_TEST_CONTAINER environment variable, once the ObsGroup container is retired.
"""

import glob
import os
import pathlib
import re
import sys

try:
    import yaml
except ImportError:
    print('SKIP: PyYAML is not available, cannot check container pins')
    sys.exit(0)

PIN = 'use data frame container'
SECTION = 'obs data container'

# Both inputs are fixed by the layout of the repository this script lives in, so they are
# derived from its own path rather than passed in.
IODA_ROOT = pathlib.Path(__file__).resolve().parent.parent
TESTINPUT = IODA_ROOT / 'test' / 'testinput'
CMAKELISTS = IODA_ROOT / 'test' / 'CMakeLists.txt'


def registrations_by_yaml(cmakelists):
    """Map each testinput yaml to whether every ctest using it sets IODA_TEST_CONTAINER.

    Returns {yaml_path: (n_registrations, n_with_env)}.
    """
    text = open(cmakelists).read()
    out = {}
    for m in re.finditer(r'ecbuild_add_test\s*\(', text):
        start = m.end()
        depth, i = 1, start
        while depth and i < len(text):
            depth += (text[i] == '(') - (text[i] == ')')
            i += 1
        body = text[start:i]
        has_env = 'IODA_TEST_CONTAINER' in body
        for y in set(re.findall(r'testinput/[A-Za-z0-9_/.\-]+\.yaml', body)):
            n, e = out.get(y, (0, 0))
            out[y] = (n + 1, e + has_env)
    return out


def obs_spaces(node):
    """Every 'obs space' mapping. PyYAML has already resolved '<<:' and '*aliases'."""
    out = []
    if isinstance(node, dict):
        for k, v in node.items():
            if k == 'obs space' and isinstance(v, dict):
                out.append(v)
            else:
                out += obs_spaces(v)
    elif isinstance(node, list):
        for v in node:
            out += obs_spaces(v)
    return out


def section_pin(doc):
    """Where a section-level 'obs data container' would take effect for this file, if set.

    oops::ObsSpaces reads it from the 'observations' mapping; applyContainerDefault and
    the deprecated-sequence path read it from the top level. Returns (found, where).
    """
    if not isinstance(doc, dict):
        return False, None
    observations = doc.get('observations')
    if isinstance(observations, dict):
        return SECTION in observations, "in the 'observations' section"
    return SECTION in doc, 'at the top level'


def _registration_owned_error(rel, n_reg, names_a_container):
    """A yaml whose ctests all set IODA_TEST_CONTAINER must not name a container itself."""
    if not names_a_container:
        return None
    return ('%s: names a container, but all %d ctest(s) using it set '
            'IODA_TEST_CONTAINER. Both YAML forms outrank the environment variable, so '
            'the per-container pair would collapse onto one container. Remove the '
            'container setting from this file.' % (rel, n_reg))


def _yaml_owned_error(rel, spaces, has_section, where):
    """A yaml nothing pins from the outside must state a container for every obs space."""
    if has_section:
        return None
    missing = [s.get('name', '<unnamed>') for s in spaces if PIN not in s]
    if not missing:
        return None
    return ("%s: %d obs space(s) state no container and would follow the '%s' parameter "
            "default: %s. Add '%s: true|false' to each, or a single '%s' %s of this file."
            % (rel, len(missing), PIN, ', '.join(repr(m) for m in missing),
               PIN, SECTION, where))


def check_file(path, rel, regs):
    """Check one yaml. Returns (error message or None, number of obs spaces checked)."""
    try:
        with open(path) as stream:
            doc = yaml.safe_load(stream)
    except (OSError, yaml.YAMLError) as exc:
        # Report and keep going, so one bad file does not hide the rest.
        return '%s: cannot read: %s' % (rel, exc), 0

    spaces = obs_spaces(doc)
    if not spaces:
        return None, 0

    n_reg, n_env = regs.get('testinput/' + rel.replace(os.sep, '/'), (0, 0))
    if n_reg == 0:
        return None, 0                                 # not run by any ctest

    has_section, where = section_pin(doc)

    if n_env == n_reg:
        names_a_container = has_section or any(PIN in s for s in spaces)
        return _registration_owned_error(rel, n_reg, names_a_container), len(spaces)

    if n_env:
        return ('%s: %d of %d ctest(s) using it set IODA_TEST_CONTAINER. Either all of '
                'them should, or none.' % (rel, n_env, n_reg)), len(spaces)

    return _yaml_owned_error(rel, spaces, has_section, where), len(spaces)


def main():
    absent = [p for p in (TESTINPUT, CMAKELISTS) if not p.exists()]
    if absent:
        print('FAIL: expected %s, but it does not exist. This script reads the ioda tree '
              'it lives in; it has probably been copied somewhere else.' % absent[0])
        return 1

    regs = registrations_by_yaml(CMAKELISTS)
    paths = sorted(glob.glob(os.path.join(TESTINPUT, '**', '*.yaml'), recursive=True))
    results = [check_file(p, os.path.relpath(p, TESTINPUT), regs) for p in paths]

    errors = [e for e, _ in results if e]
    checked = sum(n for _, n in results)

    if errors:
        print('FAIL: %d file(s) leave an obs space on the container default\n'
              % len(errors))
        for e in errors:
            print('  ' + e + '\n')
        return 1

    print('PASS: %d obs space(s) all state their container' % checked)
    return 0


if __name__ == '__main__':
    sys.exit(main())
