import os
import sys

print("ioda python binding test\n\n")

if os.environ.get('LIBDIR') is not None:
    sys.path.append(os.environ['LIBDIR'])
    print("\tAdding sys path: " + os.environ['LIBDIR'])
else:
    print("\tLIBDIR unset. No change to sys path.")

print("\tLoading ioda library...")

import ioda


