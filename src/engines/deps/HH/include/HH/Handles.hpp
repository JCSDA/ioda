#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <hdf5.h>

#include <functional>
#include <memory>

#include "./defs.hpp"
#include "Handles_HDF.hpp"

namespace HH {

namespace Handles {
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

class HH_DL HH_hid_t {
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
}  // namespace Handles
using Handles::HH_hid_t;
}  // namespace HH
