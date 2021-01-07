# ioda-engines

ioda-engines provides unified in-memory and in-file data storage. The rest of jedi
depends on ioda to provide the data that different applications need, and ioda uses
ioda-engines to manipulate the data.

This repository represents part of the OOPS/IODA/UFO changes. Documentation is still being
written. Basically, we wanted to refactor ioda and provide a common observation store that
was aware of metadata and could store multidimensional data.

## Quick start

Most of the library is implemented in a few key classes under namespace ```ioda```:

- ioda::Group is a container. ioda::Group objects may contain Variables (ioda::Variable), Attributes (ioda::Attribute), and other Groups.
- ioda::Variable represents a single variable. It may also contain Attributes (ioda::Attribute).
- The ioda::Has_Attributes and ioda::Has_Variables classes are helper classes.

See [the Walkthrough page](@ref Walkthrough) for tutorials and usage guides.


Errata:

Macros are documented in the [Modules](./modules.html) tab of the manual.

The [Related Pages](./pages.html) tab lists an introduction to how we use [the GSL library](@ref GSL).


## Dependencies

Required:

- A C++ 2014-conformant compiler. Provided by gcc 6+, clang 6+, Apple clang 6+, Intel C++ 18+, or MSVC 2019+. Other compiler families may work, but are untested. [This](https://en.cppreference.com/w/cpp/compiler_support) may help.
- [HDF5](https://www.hdfgroup.org/solutions/hdf5/). Only the C and HL libraries are needed (HH provides a vastly better C++ binding.) Versions 1.8, 1.10 and 1.12 all should work. 1.12 is preferred.
- HDFforHumans (HH; included automatically in deps/HH)
- [The Guideline Support Library](@ref GSL) (included in deps/gsl-single)
- [Eigen3](https://eigen.tuxfamily.org/) (we need both the core and unsupported headers)

Optional:
- [Doxygen](http://www.doxygen.nl/) and [Graphviz](https://www.graphviz.org/)

## Build instructions

This is very easy. Make a build directory somewhere. Run ```cmake (path_to_sources)``` to configure the build system. You might need to set HDF5_DIR to point to your HDF5 installation if not auto-detected (using the -D option described in the [cmake man page](https://cmake.org/cmake/help/latest/manual/cmake.1.html)).

Documentation builds require both [Doxygen](http://www.doxygen.nl/) and [Graphviz](https://www.graphviz.org/). The ```BUILD_DOCUMENTATION``` option may be toggled between ```No``` (default), ```BuildOnly``` and ```BuildAndInstall```.

To build the code, run ```make```.

To test, run ```ctest``` or ```make test```.

(Optional) To install, make sure that ```CMAKE_INSTALL_PREFIX``` is set to something sensible and run ```make install```.

(Optional) Packaging is not yet implemented. Once it is, you will be able to run ```make package``` or ```cpack``` to produce packages.

## Directory layout

- cmake - CMake-specific macros
- deps
  - deps/gsl-single - The C++ Core Guidelines Support Library
  - deps/HH - HDFforHumans
- docs - Ancillary markdown files, Doxyfile.in
- Examples - User guides and tutorials.
- jedi - Top-level library that should either be added into the bundles or merged into oops.
- ioda - The main library. Split into include and source files.
- share - Example data files.
- test - Unit testing applications.

## Porting instructions

The current idea is that this repository will be added into the regular bundles. A few things should
be done to make it link up properly.

1. The CMake instructions should be replaced with ecbuild instructions.
2. Folders should be renamed. ioda -> ioda-engines?
3. More tests need to be produced.

## Contact info

The majority of ioda-engines is authored by Ryan Honeyager (honeyage@ucar.edu; ryan.honeyager@noaa.gov). The ObsSpace backend is written by Steve Herbener (stephenh@ucar.edu). Feel free to raise any [questions or issues on GitHub](https://github.com/JCSDA/ioda-engines/issues), or to contact the maintainers directly.

## License

(C) Copyright 2020 UCAR

This software is licensed under the terms of the Apache Licence Version 2.0
which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.

See [the LICENSE file](@ref LICENSE) for the full license text.
