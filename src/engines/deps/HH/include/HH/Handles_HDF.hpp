#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <hdf5.h>

#include <memory>

#include "./defs.hpp"

namespace HH {
namespace Handles {
/// Encapsulate a static hid object in a shared pointer.
inline std::shared_ptr<hid_t> createStatic(hid_t newh) { return std::shared_ptr<hid_t>(new hid_t(newh)); }

/// Detect invalid HDF5 ids
struct InvalidHDF5Handle {
  static inline bool isValid(hid_t h) {
    htri_t res = H5Iis_valid(h);
    if (res <= 0) {
      return false;
    }
    return true;
  }
  static inline bool isInvalid(hid_t h) { return !isValid(h); }
};
/// Automatically close out-of-scope HDF5 objects
namespace Closers {
struct CloseHDF5Attribute {
  static inline void Close(hid_t h) { herr_t err = H5Aclose(h); }
  static inline void CloseP(hid_t* h) {
    if (*h >= 0) H5Aclose(*h);
    delete h;
  }
};
struct CloseHDF5File {
  static inline void Close(hid_t h) { herr_t err = H5Fclose(h); }
  static inline void CloseP(hid_t* h) {
    if (*h >= 0) H5Fclose(*h);
    delete h;
  }
};
struct CloseHDF5Dataset {
  static inline void Close(hid_t h) { herr_t err = H5Dclose(h); }
  static inline void CloseP(hid_t* h) {
    if (*h >= 0) H5Dclose(*h);
    delete h;
  }
};
struct CloseHDF5Dataspace {
  static inline void Close(hid_t h) { herr_t err = H5Sclose(h); }
  static inline void CloseP(hid_t* h) {
    if (*h >= 0) H5Sclose(*h);
    delete h;
  }
};
struct CloseHDF5Datatype {
  static inline void Close(hid_t h) { herr_t err = H5Tclose(h); }
  static inline void CloseP(hid_t* h) {
    if (*h >= 0) H5Tclose(*h);
    delete h;
  }
};
struct CloseHDF5Group {
  static inline void Close(hid_t h) { herr_t err = H5Gclose(h); }
  static inline void CloseP(hid_t* h) {
    if (*h >= 0) H5Gclose(*h);
    delete h;
  }
};
struct CloseHDF5PropertyList {
  static inline void Close(hid_t h) { herr_t err = H5Pclose(h); }
  static inline void CloseP(hid_t* h) {
    if (*h >= 0) H5Pclose(*h);
    delete h;
  }
};
struct DoNotClose {
  static inline void Close(hid_t) { return; }
  static inline void CloseP(hid_t* h) { delete h; }
};
}  // namespace Closers
}  // namespace Handles
}  // namespace HH
