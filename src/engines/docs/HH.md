# The HDF5 / HH Backend

\addtogroup ioda_engines_grp_HH

## Ancient history

HDFforHumans (HH) was developed by Ryan Honeyager from 2017 - early 2020 as an alternate
C++ interface to HDF5. HDF5's standard interface was new and unstable, and
he had an awful time trying to keep [ABI compatability](https://en.wikipedia.org/wiki/Application_binary_interface)
across even small revisions to the HDF5 library. Other developers had
similar experiences - [H5CPP](http://h5cpp.org/) also started around the same time.
So, a standalone wrapper was produced which was quickly integrated into
research projects (e.g. [libicedb](https://github.com/rhoneyager/libicedb)),
though it never quite had a standalone release.

HH informed many of the design decisions for ioda-engines, which decouples
a user-friendly frontend interface from a storage provider implementation.
The code for HH was ported to ioda, and it acts as the reference on-disk
storage engine for ioda-managed data.

### Why not NetCDF?

There were several reasons for this, though none prevent the development
of a pure NetCDF backend. During development, it was easier to find and link
HDF5, which had a standard CMake find_package implementation. NetCDF did not.
HDF5 had fewer dependencies, and it was easier to build on Windows. Also,
NetCDF nowadays is partly based on the HDF5 library, but its development
lags HDF5 changes by a year or more. All NetCDF-4 files are HDF5 files
behind the scenes, though the reverse is not true. So, it was simply easier
to try HDF5 first, and it worked.

Of course, NetCDF offers several features not available in HDF5. It has
built-in support for accessing files over the internet using
[DAP](https://en.wikipedia.org/wiki/OPeNDAP) or
[THREDDS](https://www.unidata.ucar.edu/software/tds/current/)
servers. Also, some JEDI developers have noted that the NetCDF Python interfaces
(and via [xarray](http://xarray.pydata.org/en/stable/)) have fewer performance
hiccups than [h5py](https://www.h5py.org/) et al., which likely result from
NetCDF-tuning of the underlying HDF5 calls. Creating a NetCDF-specific
engine should not be too difficult, as there is a one-to-one correspondence
for many function calls.

## How does it work?

The HH engine *wraps* the HDF5 library calls and resource
handles. HDF5 is a C library, so we want to provide
[abstraction](https://www.geeksforgeeks.org/abstraction-in-c/),
[encapsulation](https://www.geeksforgeeks.org/encapsulation-in-c/),
[inheritance](https://www.geeksforgeeks.org/inheritance-in-c/),
and [RAII](https://en.cppreference.com/w/cpp/language/raii).

The old HDF5 [reference manual](https://support.hdfgroup.org/HDF5/doc/RM/RM_H5Front.html)
groups these function calls for attributes, datasets (variables), files,
filters, groups, identifiers, objects, property lists, spaces, and types.
This list might seem familiar to the elements used inside of ioda. Early on
in ioda-engines development we decided that we really liked their data model,
and chose to model ioda similarly. Besides wrapping their function calls, we
also wrap HDF5's resource handles (type ```hid_t```). Because this is a C
library, there are no objects. To avoid exposing internal details to end users,
the HDF5 developers pass back opaque handles to callers that "open" or
"create" objects. This implementation is similar to the [Windows API's use of
handles and objects](https://docs.microsoft.com/en-us/windows/win32/sysinfo/handles-and-objects).
Functions are passed these resource handles, but all of the implementation
details are hidden away by the library. Handles are reference-counted. There
can be multiple active handles that refer to the same object. We follow a similar
encapsulation scheme inside of ioda. Each ```Attribute```, ```Group```, and ```Variable```
is a class-based resource handle that uses the
[PImpl technique](https://en.cppreference.com/w/cpp/language/pimpl) to hide the
engine implementation details from the end user. Again, the difference here
is that we have all the power of C++ available, so our handles look like classes
and auto-destruct when they go out of scope. In our design, users also can
transparently duplicate handles (```Group g2 = g1;```) without worrying about
issues with deep pointer copies or derived-class slicing. For details on how this
is accomplished, see the **TODO(Ryan): Add link to class structure design document!**

### More implementation specifics

A new HH engine is instantiated when the user calls an entry function, usually
located in [ioda/Engines/Factory.h](https://github.com/JCSDA-internal/ioda/blob/develop/src/engines/ioda/include/ioda/Engines/Factory.h)
or [ioda/Engines/HH.h](https://github.com/JCSDA-internal/ioda/blob/develop/src/engines/ioda/include/ioda/Engines/HH.h).
The create and open calls are implemented in [src/ioda/engines/HH.cpp](https://github.com/JCSDA-internal/ioda/blob/develop/src/engines/ioda/src/ioda/Engines/HH.cpp).
The entry functions create a new HH backend and encapsulate it within a Group object. From
there, all calls to groups, variables, and attributes are silently intercepted and are
passed to the HH backend implementation, which is located in
[src/ioda/Engines/HH](https://github.com/JCSDA-internal/ioda/tree/develop/src/engines/ioda/src/ioda/Engines/HH).
All header files at this level are private, and all classes are marked with ```IODA_HIDDEN```,
which means that they are only directly linkable inside of the ioda library.

For a more illustrative example, take a look at the HH_Attribute class, which
implements the Attribute API. The header file is rather bare. The class inherits
from ```ioda::detail::Attribute_Backend```. About half of the functions are ```final``` and
are those called by the frontend Attribute object. The function definitions usually
are concerned with proper wrapping of the C calls. For example, ```HH_Attribute::getName()```
sets up dual calls to ```H5Aget_name``` and then encapsulates the return
data into a string.

### The type system

HDF5 has a very rich type system, and extensive emphasis is placed
on ensuring that data are portable among systems with wildly different
architectures. This is more complicated than big and little endian... different
processor architectures implement slightly different floating-point number
specifications, and mantissa and exponent differences do occur. Furthermore,
C's fundamental types have lengths / bounds that vary across compilers.
HDF5's fundamental data types include all integers from eight to 64 bits.
This includes both signed and unsigned types. Floating
point types of 32 and 64 bits are supported, as are long doubles (of 80, 96 or 128 bits).

There are several possible string types, representing different combinations of
fixed-length and variable length strings, as well as ascii vs UTF-8. By default,
ioda can read in any of these string types, but everything that it writes is
assumed to be a variable-length, UTF-8 string. We subscribe to the
[UTF-8 everywhere](http://utf8everywhere.org/) guidelines.

Not all languages support all of these types. Fortran, for instance, lacks unsigned
types. Type conversions are transparently applied in such cases. Also for Fortran,
fixed-length strings are the norm, and these are silently trimmed. Other engines
should eventually converge on this standard.

More advanced types such as arrays and compound types are also supported, but these
have limited use at present. We will investigate whether a custom type is suitable
for ``oops::DateTime`` serialization, as constant string <-> time type conversions
are undesirable. Given how our type system is structured, it is likely that
different engines can implement this storage differently.

### The data layout

The [HDF5 file layout](https://support.hdfgroup.org/HDF5/doc/H5.intro.html#Intro-FileTech)
emulates a filesystem partition. The super block contains top-level
filesystem details, and it references the filesystem root (location '/') group.
Each group acts as a directory. Each group implements a B-tree that lists all child
objects, which includes other groups, datasets (variables), symbolic and external
links (a future expansion target in ioda), and custom type definitions (unused by HH). Each object has a header that keeps track
of where the object's data are located, as well as any attributes and other metadata.

The format is really flexible regarding how the data are laid out on disk, and
this allows for substantial performance tuning. One of the big takeaways, though,
is that this format is **not** a packet table format. In a packet, the location
is the fundamental dimension, and all data variables are grouped together at each
location. While HDF5 can emulate this behavior, we instead prefer its usual data layout,
which groups each dataset's data together in a series of one or more
contiguous regions within the file.

The trade-off between locality and continuity is complicated. Both methods have advantages.
Ioda's interfaces are used *both* for files and as in-memory containers, which informed how
we want to match data to end-user operations.

By grouping by variables:
- [Vectorized CPU operations](https://en.wikipedia.org/wiki/Vectorization) become possible.
  Modern CPUs can process 2-8 32-bit floats per instruction, such as with the [SSE](https://en.wikipedia.org/wiki/Streaming_SIMD_Extensions) instructions.
- Compression becomes more efficient. The data appear less "random" to a compression algorithm.
- Resizing already-existing variables is less costly. (This is less important for
  a stable file format, but users can also resize objects in memory, which is a ioda use case.)

By grouping by packets:
- MPI distributions of data are easier to implement. The performance characteristics, however,
  are hard to test and are heavily design dependent.
- Splitting files and appending data along a location axis is trivial.
