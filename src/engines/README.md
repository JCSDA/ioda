# IODA-engines

IODA-engines provides unified in-memory and in-file data storage. The rest of JEDI
depends on IODA to provide the data that different applications need, and IODA uses
IODA-engines to manipulate the data.

## Quick start

Most of the library is implemented in a few key classes under namespace ```ioda```:

- ioda::Group is a container. ioda::Group objects may contain Variables (ioda::Variable), Attributes (ioda::Attribute), and other Groups.
- ioda::Variable represents a single variable. It may also contain Attributes (ioda::Attribute).
- The ioda::Has_Attributes and ioda::Has_Variables classes are helper classes.


Errata:

Macros are documented in the [Modules](./modules.html) tab of the manual.

The [Related Pages](./pages.html) tab lists an introduction to how we use [the GSL library](@ref GSL).


## Dependencies

Required:

- A C++ 2014-conformant compiler. Provided by gcc 6+, clang 6+, Apple clang 6+, Intel C++ 18+, or MSVC 2019+. Other compiler families may work, but are untested. [This](https://en.cppreference.com/w/cpp/compiler_support) may help.
- [HDF5](https://www.hdfgroup.org/solutions/hdf5/). Only the C and HL libraries are needed. Version 1.12 or greater is preferred. Not all functions are available in versions 1.8 and 1.10.
- [The Guideline Support Library](@ref GSL)
- [Eigen3](https://eigen.tuxfamily.org/) (we need both the core and unsupported headers)

Optional:
- [Doxygen](http://www.doxygen.nl/) and [Graphviz](https://www.graphviz.org/) are needed to generate the html documentation.
- [ODC](https://github.com/ecmwf/odc) + [eckit](https://github.com/ecmwf/eckit) + oops are needed to enable ODC support.
- [eckit](https://github.com/ecmwf/eckit) is needed for some of the unit tests.

## Build instructions

This is very easy. Make a build directory somewhere. Run ```cmake (path_to_sources)``` to configure the build system. You might need to set HDF5_DIR to point to your HDF5 installation if not auto-detected (using the -D option described in the [cmake man page](https://cmake.org/cmake/help/latest/manual/cmake.1.html)).

Documentation builds require both [Doxygen](http://www.doxygen.nl/) and [Graphviz](https://www.graphviz.org/). The ```BUILD_DOCUMENTATION``` option may be toggled between ```No``` (default), ```BuildOnly``` and ```BuildAndInstall```.

To build the code, run ```make```.

To test, run ```ctest``` or ```make test```.

(Optional) To install, make sure that ```CMAKE_INSTALL_PREFIX``` is set to something sensible and run ```make install```.

(Optional) Packaging is not yet implemented. Once it is, you will be able to run ```make package``` or ```cpack``` to produce packages.

## Directory layout

- cmake - CMake-specific macros
- docs - Ancillary markdown files, Doxyfile.in
- Examples - User guides and tutorials.
- ioda - The main library. Split into include and source files.
- share - Example data files.
- test - Unit testing applications.

## Contact info

The majority of ioda-engines is authored by Ryan Honeyager (honeyage@ucar.edu; ryan.honeyager@noaa.gov). The ObsSpace backend is written by Steve Herbener (stephenh@ucar.edu). Feel free to raise any [questions or issues on GitHub](https://github.com/JCSDA/ioda-engines/issues), or to contact the maintainers directly.

## License

(C) Copyright 2020-2021 UCAR

This software is licensed under the terms of the Apache Licence Version 2.0
which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

See [the LICENSE file](@ref LICENSE) for the full license text.
