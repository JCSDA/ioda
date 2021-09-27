/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-hastypes.cpp
 * \brief HDF5 engine implementation of Has_Types.
 */

#include "./HH/HH-hastypes.h"

#include <hdf5_hl.h>

#include <algorithm>
#include <numeric>
#include <set>

#include "./HH/HH-Filters.h"
#include "./HH/HH-types.h"
#include "./HH/HH-util.h"
#include "./HH/Handles.h"
#include "ioda/Exception.h"
#include "ioda/Misc/StringFuncs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {

HH_HasTypes::~HH_HasTypes() = default;
HH_HasTypes::HH_HasTypes() : base_(Handles::HH_hid_t::dummy()) {}

HH_HasTypes::HH_HasTypes(HH_hid_t grp) : base_(grp) {}

detail::Type_Provider* HH_HasTypes::getTypeProvider() const { return HH_Type_Provider::instance(); }

bool HH_HasTypes::exists(const std::string& name) const {
  auto paths = splitPaths(name);
  for (size_t i = 0; i < paths.size(); ++i) {
    auto p            = condensePaths(paths, 0, i + 1);
    htri_t linkExists = H5Lexists(base_(), p.c_str(), H5P_DEFAULT);
    if (linkExists < 0)
      throw Exception("H5Lexists failed.", ioda_Here())
        .add("here", getNameFromIdentifier(base_()))
        .add("name", name);
    if (linkExists == 0) return false;
  }
#if H5_VERSION_GE(1, 12, 0)
  H5O_info1_t oinfo;
  herr_t err = H5Oget_info_by_name1(base_(), name.c_str(), &oinfo,
                                    H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#else
  H5O_info_t oinfo;
  herr_t err = H5Oget_info_by_name(base_(), name.c_str(), &oinfo,
                                   H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#endif
  if (err < 0) throw Exception("H5Oget_info_by_name failed.", ioda_Here());
  return (oinfo.type == H5O_type_t::H5O_TYPE_NAMED_DATATYPE);
}

void HH_HasTypes::remove(const std::string& name) {
  auto ret = H5Ldelete(base_(), name.c_str(), H5P_DEFAULT);
  if (ret < 0)
    throw Exception("Failed to remove link to named type.", ioda_Here()).add("name", name);
}

Type HH_HasTypes::open(const std::string& name) const {
  hid_t id = H5Topen2(base_(), name.c_str(), H5P_DEFAULT);
  if (id < 0) throw Exception("Cannot open named type", ioda_Here()).add("name", name);
  auto hnd = HH_hid_t(id, Handles::Closers::CloseHDF5Datatype::CloseP);

  auto b = std::make_shared<HH_Type>(hnd);

  return Type{b, typeid(void)};
}

std::vector<std::string> HH_HasTypes::list() const {
  std::vector<std::string> res;
  H5G_info_t info;
  herr_t e = H5Gget_info(base_(), &info);
  if (e < 0) throw Exception("H5Gget_info failed.", ioda_Here());
  res.reserve(gsl::narrow<size_t>(info.nlinks));
  for (hsize_t i = 0; i < info.nlinks; ++i) {
    // Get the name
    ssize_t szName
      = H5Lget_name_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, NULL, 0, H5P_DEFAULT);
    if (szName < 0) throw Exception("H5Lget_name_by_idx failed.", ioda_Here());
    std::vector<char> vName(szName + 1, '\0');
    if (H5Lget_name_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, vName.data(), szName + 1,
                           H5P_DEFAULT)
        < 0)
      throw Exception("H5Lget_name_by_idx failed.", ioda_Here());

      // Get the object and check the type
#if H5_VERSION_GE(1, 12, 0)
    H5O_info1_t oinfo;
    herr_t err
      = H5Oget_info_by_idx1(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT);
#else
    H5O_info_t oinfo;
    herr_t err
      = H5Oget_info_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT);
#endif

    if (err < 0) continue;
    if (oinfo.type == H5O_type_t::H5O_TYPE_NAMED_DATATYPE)
      res.emplace_back(std::string(vName.data()));
  }
  return res;
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
