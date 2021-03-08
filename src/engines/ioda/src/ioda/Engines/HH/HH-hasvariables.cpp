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
 * \file HH-hasvariables.cpp
 * \brief HDF5 engine implementation of Has_Variables.
 */

#include "./HH/HH-hasvariables.h"

#include <hdf5_hl.h>

#include <algorithm>
#include <numeric>
#include <set>

#include "./HH/HH-Filters.h"
#include "./HH/HH-attributes.h"
#include "./HH/HH-hasattributes.h"
#include "./HH/HH-types.h"
#include "./HH/HH-variablecreation.h"
#include "./HH/HH-variables.h"
#include "./HH/Handles.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Misc/SFuncs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
HH_HasVariables::~HH_HasVariables() = default;
HH_HasVariables::HH_HasVariables() : base_(Handles::HH_hid_t::dummy()) {}

HH_HasVariables::HH_HasVariables(HH_hid_t grp, HH_hid_t fileroot)
    : base_(grp), fileroot_(fileroot) {}

detail::Type_Provider* HH_HasVariables::getTypeProvider() const {
  return HH_Type_Provider::instance();
}

FillValuePolicy HH_HasVariables::getFillValuePolicy() const {
  // Cheaply-reconstituted object.
  Has_Attributes fratts(std::make_shared<HH_HasAttributes>(fileroot_));

  if (fratts.exists("_NCProperties"))
    return ioda::FillValuePolicy::NETCDF4;
  else if (fratts.exists("_ioda_layout"))
    return ioda::FillValuePolicy::NETCDF4;
  return FillValuePolicy::HDF5;
}

bool HH_HasVariables::exists(const std::string& dsetname) const {
  auto paths = splitPaths(dsetname);
  for (size_t i = 0; i < paths.size(); ++i) {
    auto p            = condensePaths(paths, 0, i + 1);
    htri_t linkExists = H5Lexists(base_(), p.c_str(), H5P_DEFAULT);
    if (linkExists < 0) throw;  // HH_throw;
    if (linkExists == 0) return false;
  }
#if H5_VERSION_GE(1, 12, 0)
  H5O_info1_t oinfo;
  herr_t err = H5Oget_info_by_name1(base_(), dsetname.c_str(), &oinfo,
                                    H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#else
  H5O_info_t oinfo;
  herr_t err = H5Oget_info_by_name(base_(), dsetname.c_str(), &oinfo,
                                   H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#endif
  if (err < 0) throw;  // HH_throw;
  return (oinfo.type == H5O_type_t::H5O_TYPE_DATASET);
}

void HH_HasVariables::remove(const std::string& name) {
  auto ret = H5Ldelete(base_(), name.c_str(), H5P_DEFAULT);
  if (ret < 0)
    throw;  // HH_throw.add("Reason", "Failed to remove link to dataset.")
            //.add("name", name)
            //.add("Return code", ret);
}

Variable HH_HasVariables::open(const std::string& name) const {
  hid_t dsetid = H5Dopen(base_(), name.c_str(), H5P_DEFAULT);
  if (dsetid < 0)
    throw std::logic_error("Cannot open dataset");  // HH_throw.add("Reason", "Cannot open
                                                    // dataset").add("Name", dsetname);

  auto b = std::make_shared<HH_Variable>(
    HH_hid_t(dsetid, Handles::Closers::CloseHDF5Dataset::CloseP), shared_from_this());
  Variable var{b};
  return var;
}

std::vector<std::string> HH_HasVariables::list() const {
  std::vector<std::string> res;
  H5G_info_t info;
  herr_t e = H5Gget_info(base_(), &info);
  if (e < 0) throw;  // HH_throw;
  res.reserve(gsl::narrow<size_t>(info.nlinks));
  for (hsize_t i = 0; i < info.nlinks; ++i) {
    // Get the name
    ssize_t szName
      = H5Lget_name_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, NULL, 0, H5P_DEFAULT);
    if (szName < 0) throw;  // HH_throw;
    std::vector<char> vName(szName + 1, '\0');
    H5Lget_name_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, vName.data(), szName + 1,
                       H5P_DEFAULT);

    // Get the object and check the type
#if H5_VERSION_GE(1, 12, 0)
    H5O_info1_t oinfo;
    herr_t err
      = H5Oget_info_by_idx1(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT);
    // herr_t err = H5Oget_info_by_name1(base_(), vName.data(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT
    // only, per docs.
#else
    H5O_info_t oinfo;
    herr_t err
      = H5Oget_info_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT);
    // herr_t err = H5Oget_info_by_name(base_(), vName.data(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT
    // only, per docs.
#endif
    if (err < 0) continue;
    if (oinfo.type == H5O_type_t::H5O_TYPE_DATASET) res.emplace_back(std::string(vName.data()));
  }
  return res;
}

Variable HH_HasVariables::create(const std::string& name, const Type& in_memory_dataType,
                                 const std::vector<Dimensions_t>& dimensions,
                                 const std::vector<Dimensions_t>& max_dimensions,
                                 const VariableCreationParameters& params) {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());

    // Dataset creation parameters and chunk sizes
    VariableCreation hParams(params, dimensions, max_dimensions, typeBackend);

    hid_t dsetid
      = H5Dcreate(base_(),                // The group that holds the variable
                  name.c_str(),           // The name of the variable. If forward slashes are found,
                                          // intermediate groups get created.
                  typeBackend->handle(),  // The data type of the variable (int, char, ...)
                  hParams.dataspace()(),  // The data space of the variable (dimensions, max dims)
                  hParams.linkCreationPlist()(),     // The link creation property list (create
                                                     // intermediate groups if necessary)
                  hParams.datasetCreationPlist()(),  // The dataset creation property list
                                                     // (compression, chunking, fill value)
                  hParams.datasetAccessPlist()()  // The dataset access property list (H5P_DEFAULT)
      );
    if (dsetid < 0) throw;  // HH_throw;
    HH_hid_t res(dsetid, Handles::Closers::CloseHDF5Dataset::CloseP);
    // Note: this new variable gets handed back to Has_Variables_Base::create,
    // which then calls params.applyImmediatelyAfterVariableCreation to link up
    // dimension scales and set initial attributes.

    auto b = std::make_shared<HH_Variable>(res, shared_from_this());
    Variable var{b};
    return var;
  } catch (std::bad_cast&) {
    throw;
  }
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
