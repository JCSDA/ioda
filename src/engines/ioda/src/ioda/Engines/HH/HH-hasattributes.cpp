/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-hasattributes.cpp
 * \brief HDF5 engine implementation of Has_Attributes.
 */

#include "./HH/HH-hasattributes.h"

#include "./HH/HH-attributes.h"
#include "./HH/HH-types.h"
#include "./HH/HH-util.h"
#include "ioda/Exception.h"
#include "ioda/Misc/Dimensions.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
HH_HasAttributes::HH_HasAttributes() : base_(Handles::HH_hid_t::dummy()) {}
HH_HasAttributes::HH_HasAttributes(HH_hid_t b) : base_(b) {}
HH_HasAttributes::~HH_HasAttributes() = default;
detail::Type_Provider* HH_HasAttributes::getTypeProvider() const {
  return HH_Type_Provider::instance();
}

std::vector<std::string> HH_HasAttributes::list() const {
  std::vector<std::string> res;

#if H5_VERSION_GE(1, 12, 0)
  H5O_info1_t info;
  herr_t err = H5Oget_info1(base_(), &info);  // H5P_DEFAULT only, per docs.
#else
  H5O_info_t info;
  herr_t err = H5Oget_info(base_(), &info);  // H5P_DEFAULT only, per docs.
#endif

  if (err < 0) throw;  // HH_throw;
  res.resize(gsl::narrow<size_t>(info.num_attrs));
  for (size_t i = 0; i < res.size(); ++i) {
    HH_Attribute a(HH_hid_t(H5Aopen_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, (hsize_t)i,
                                           H5P_DEFAULT, H5P_DEFAULT),
                            Handles::Closers::CloseHDF5Attribute::CloseP));
    res[i] = a.getName();
  }

  return res;
}

bool HH_HasAttributes::exists(const std::string& attname) const {
#if H5_VERSION_GE(1, 12, 0)
  H5O_info1_t info;
  herr_t err = H5Oget_info1(base_(), &info);  // H5P_DEFAULT only, per docs.
#else
  H5O_info_t info;
  herr_t err = H5Oget_info(base_(), &info);  // H5P_DEFAULT only, per docs.
#endif
  if (err < 0) throw Exception("H5Oget_info failed.", ioda_Here());
  if (info.num_attrs < thresholdLinear) {
    auto ret = iterativeAttributeSearch(base_(), attname.c_str(), getAttrCreationOrder(base_(), info.type));
    bool success = ret.first;
    return success;
  } else {
    auto ret = H5Aexists(base_(), attname.c_str());
    if (ret < 0) throw Exception("H5Aexists failed.", ioda_Here());
    return (ret > 0);
  }
}

void HH_HasAttributes::remove(const std::string& attname) {
  herr_t err = H5Adelete(base_(), attname.c_str());
  if (err < 0) throw Exception("H5Adelete failed.", ioda_Here());
}

Attribute HH_HasAttributes::open(const std::string& name) const {
#if H5_VERSION_GE(1, 12, 0)
  H5O_info1_t info;
  herr_t err = H5Oget_info1(base_(), &info);  // H5P_DEFAULT only, per docs.
#else
  H5O_info_t info;
  herr_t err = H5Oget_info(base_(), &info);  // H5P_DEFAULT only, per docs.
#endif
  if (err < 0) throw Exception("H5Oget_info failed.", ioda_Here());
  if (info.num_attrs < thresholdLinear) {
    HH_Attribute a = iterativeAttributeSearchAndOpen(base_(), info.type, name.c_str());
    if (!a.get().isValid()) throw Exception("iterativeAttributeSearchAndOpen failed.",
      ioda_Here());
    auto b = std::make_shared<HH_Attribute>(a);
    Attribute att{b};
    return att;
  } else {
    auto ret = H5Aopen(base_(), name.c_str(), H5P_DEFAULT);
    if (ret < 0) throw Exception("H5Aopen failed", ioda_Here());
    auto b = std::make_shared<HH_Attribute>(ret);
    Attribute att{b};
    return att;
  }
}

Attribute HH_HasAttributes::create(const std::string& attrname, const Type& in_memory_dataType,
                                   const std::vector<Dimensions_t>& dimensions) {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
    std::vector<hsize_t> hDims;
    hDims.reserve(dimensions.size());
    for (const auto& d : dimensions) hDims.push_back(gsl::narrow<hsize_t>(d));

    hid_t space = (dimensions.empty())
                    ? H5Screate(H5S_SCALAR)
                    : H5Screate_simple(gsl::narrow<int>(hDims.size()), hDims.data(), nullptr);
    HH_hid_t dspace{space, Handles::Closers::CloseHDF5Dataspace::CloseP};
    auto attI = HH_hid_t(H5Acreate(base_(), attrname.c_str(), typeBackend->handle(), dspace(),
                                   H5P_DEFAULT, H5P_DEFAULT),
                         Handles::Closers::CloseHDF5Attribute::CloseP);
    if (H5Iis_valid(attI()) <= 0) throw Exception("H5Acreate failed.", ioda_Here());

    auto b = std::make_shared<HH_Attribute>(attI);
    Attribute att{b};
    return att;
  } catch (std::bad_cast) {
    std::throw_with_nested(Exception("typeBackend is the wrong type. Expected HH_Type.",
      ioda_Here()));
  }
}
void HH_HasAttributes::rename(const std::string& oldName, const std::string& newName) {
  auto ret = H5Arename(base_(), oldName.c_str(), newName.c_str());
  if (ret < 0) throw Exception("H5Arename failed.", ioda_Here());
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
