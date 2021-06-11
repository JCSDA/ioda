#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file Handles.h
 * \brief HDF5 resource handles in C++.
 */
#include <hdf5.h>

#include <functional>
#include <memory>

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {

namespace Handles {

/// @brief Describes what a handle points to.
/// @deprecated To be removed.
/// @ingroup ioda_internals_engines_hh
enum class Handle_Types {
  ATTRIBUTE,
  DATASET,
  DATASPACE,
  DATATYPE,
  FILE,
  GROUP,
  LINK,
  PROPERTYLIST,
  REFERENCE,
  UNKNOWN
};

/// @brief A class to wrap HDF5's hid_t resource handles.
/// @ingroup ioda_internals_engines_hh
///
/// This class adds RAII resource management to keep track of the various
/// hid_t handles that HDF5 returns to ioda. A handle, in this context, is
/// an opaque integer that references an HDF5-internal data structure that
/// tracks what is accessing a particular resource like a file, a group, an
/// attribute, and so on. Whenever you open or create a resource, you get back
/// a handle that you reference when calling other functions on that resource.
///
/// The problem with hid_t is that HDF5's C interface is a C interface, and there
/// are three key points that HH_hid_t aims to address.
///
/// - You can go out of scope and accidentally lose track of an open resource handle.
/// - You can release the handle but might use the wrong function to release, since
///   HDF5 provides at least ten release functions.
/// - HDF5 has some reserved values, like H5P_DEFAULT, that are not strictly handles
///   but are passed to the same function calls. These values can never be released.
///
/// Usage:
///
/// Wrapping an HDF5 return call into a managed handle:
/// ```
/// hid_t raw_handle = H5Fopen(.....);
/// HH_hid_t managed_handle(raw_handle, Handles::Closers::CloseHDF5File::CloseP);
/// ...
/// ```
/// The functions in Handles::Closers let you specify which function releases a handle.
///
/// Using a wrapped handle:
/// ```
/// auto res = H5Gopen(managed_handle(), ...);
/// // You could also explicitly type managed_handle.get().
/// ```
///
/// Cloning a handle:
/// ```
/// HH_hid_t hnd2 = hnd1;
/// ```
///
/// Checking if a handle is "valid" - i.e. that an error did not occur:
/// ```
/// HH_hid_t managed_handle(H5Fopen(.....), Handles::Closers::CloseHDF5File::CloseP);
/// if (!managed_handle.isValid()) throw;
/// ```
///
/// @todo Autodetect hid_t handle "type" if possible and auto-assign the closer function.
///   Will not work in all cases, particularly property lists, as HDF5 does not have a
///   good detection function for these.
class HH_hid_t {
  ::std::shared_ptr<hid_t> _h;
  // Handle_Types _typ;
public:
  ~HH_hid_t();
  hid_t get() const;
  ::std::shared_ptr<hid_t> getShared() const;

  // Future handle type safety implementation:
  // Handle_Types get_type() const { return _typ; }
  // bool isA(Handle_Types t) const { return (_typ == t); }
  // template <typename T>
  // bool isA() const { return (_typ == ); }

  HH_hid_t();
  HH_hid_t(::std::shared_ptr<hid_t> h);
  HH_hid_t(hid_t val, const std::function<void(hid_t*)>& closer = nullptr);
  hid_t operator()() const;
  static HH_hid_t dummy();
  bool isValid() const;
};

/// @brief Encapsulate a static hid object in a shared pointer.
/// @ingroup ioda_internals_engines_hh
inline std::shared_ptr<hid_t> createStatic(hid_t newh) {
  return std::shared_ptr<hid_t>(new hid_t(newh));
}

/// @brief Detect invalid HDF5 ids
/// @ingroup ioda_internals_engines_hh
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

/// \brief Structs in this namespace implement close operations on HDF5 handles.
/// \ingroup ioda_internals_engines_hh
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
using Handles::HH_hid_t;
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
