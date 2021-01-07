# HDFforHumans (HH)

This is a C++14 interface for HDF5.

This is a clone of the HDFforHumans repository, located at https://github.com/rhoneyager/HDFforHumans.
This code will be removed once ioda-engines is fully integrated into ioda.

## Requirements

- HDF5 (obviously)
- The Guidelines Support Library (packaged with HH).
  See: https://www.modernescpp.com/index.php/c-core-guideline-the-guidelines-support-library)
- Eigen3 (only to build the example app)
- Boost unit testing library (to build the unit tests)
- Doxygen (to build the documentation)

## Building

```
git clone git@github.com:rhoneyager/HH.git
cd HH
mkdir build
cd build
cmake ..
make
```

## Running the example app

```
cd build
make
./bin/HH-example
h5dump test.hdf5
```

## Generating the API documentation

```
cd build
cmake -D HH_BUILD_DOCUMENTATION=Separate
make docs
firefox ./doc/html/index.html
```

## License

See [the LICENSE file](./LICENSE).
