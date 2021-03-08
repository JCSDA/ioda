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
 * \file Handles.cpp
 * \brief HDF5 resource handles in C++.
 */
#include "./HH/Handles.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
namespace Handles {
HH_hid_t::~HH_hid_t() = default;
hid_t HH_hid_t::get() const { return *(_h.get()); }
::std::shared_ptr<hid_t> HH_hid_t::getShared() const { return _h; }
HH_hid_t::HH_hid_t() : HH_hid_t(-1, HH::Handles::Closers::DoNotClose::CloseP) {}
HH_hid_t::HH_hid_t(::std::shared_ptr<hid_t> h) : _h(h) {}
HH_hid_t::HH_hid_t(hid_t val, const std::function<void(hid_t*)>& closer) {
  if (closer) {
    _h = ::std::shared_ptr<hid_t>(new hid_t(val), closer);
  } else
    _h = ::std::shared_ptr<hid_t>(new hid_t(val));
}
hid_t HH_hid_t::operator()() const { return get(); }
HH_hid_t HH_hid_t::dummy() { return HH_hid_t(-1, Closers::DoNotClose::CloseP); }
bool HH_hid_t::isValid() const {
  H5I_type_t typ = H5Iget_type(get());
  return (typ != H5I_BADID);
}

}  // namespace Handles
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
