import os
import sys

print("ioda python binding test\n\n")

if os.environ.get('LIBDIR') is not None:
    sys.path.append(os.environ['LIBDIR'])
    print("\tAdding sys path: " + os.environ['LIBDIR'])
else:
    print("\tLIBDIR unset. No change to sys path.")

print("\tLoading jedi_base library...")

import jedi_base

print("\tjedi_base library is at " + jedi_base.OS.getLibPath())

print("\tLoading ioda library...")

import ioda


