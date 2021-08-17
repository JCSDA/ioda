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
#include "./HH/HH-util.h"
#include "./HH/HH-variablecreation.h"
#include "./HH/HH-variables.h"
#include "./HH/Handles.h"
#include "ioda/Exception.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Misc/StringFuncs.h"

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
    if (linkExists < 0) throw Exception("H5Lexists failed.", ioda_Here())
      .add("here", getNameFromIdentifier(base_()))
      .add("dsetname", dsetname);
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
  if (err < 0) throw Exception("H5Oget_info_by_name failed.", ioda_Here());
  return (oinfo.type == H5O_type_t::H5O_TYPE_DATASET);
}

void HH_HasVariables::remove(const std::string& name) {
  auto ret = H5Ldelete(base_(), name.c_str(), H5P_DEFAULT);
  if (ret < 0) throw Exception("Failed to remove link to dataset.", ioda_Here()).add("name", name);
}

Variable HH_HasVariables::open(const std::string& name) const {
  hid_t dsetid = H5Dopen(base_(), name.c_str(), H5P_DEFAULT);
  if (dsetid < 0)
    throw Exception("Cannot open dataset", ioda_Here()).add("name", name);

  auto b = std::make_shared<HH_Variable>(
    HH_hid_t(dsetid, Handles::Closers::CloseHDF5Dataset::CloseP), shared_from_this());
  Variable var{b};
  return var;
}

std::vector<std::string> HH_HasVariables::list() const {
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
    if (H5Lget_name_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE,
      i, vName.data(), szName + 1,H5P_DEFAULT) < 0)
      throw Exception("H5Lget_name_by_idx failed.", ioda_Here());

    // Get the object and check the type
#if H5_VERSION_GE(1, 12, 0)
    H5O_info1_t oinfo;
    herr_t err = H5Oget_info_by_idx1(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE,
      i, &oinfo, H5P_DEFAULT);
#else
    H5O_info_t oinfo;
    herr_t err = H5Oget_info_by_idx(base_(), ".", H5_INDEX_NAME, H5_ITER_NATIVE,
      i, &oinfo, H5P_DEFAULT);
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
    if (dsetid < 0) throw Exception("Variable creation failed.", ioda_Here())
      .add("name", name);
    HH_hid_t res(dsetid, Handles::Closers::CloseHDF5Dataset::CloseP);
    // Note: this new variable gets handed back to Has_Variables_Base::create,
    // which then calls params.applyImmediatelyAfterVariableCreation to link up
    // dimension scales and set initial attributes.

    auto b = std::make_shared<HH_Variable>(res, shared_from_this());
    Variable var{ b };

    // One last thing: if the fill value is set, then we need to add an attribute
    // called "_FillValue" for NetCDF readers.
    if (params.fillValue_.set_) {
      Attribute fv = var.atts.create("_FillValue", in_memory_dataType);
      const FillValueData_t::FillValueUnion_t fvdata = params.fillValue_.finalize();
      fv.write(gsl::make_span<char>(
        const_cast<char*>(reinterpret_cast<const char*>(&fvdata)),
        sizeof(FillValueData_t::FillValueUnion_t)),in_memory_dataType);
    }

    return var;
  }
  catch (std::bad_cast&) {
    std::throw_with_nested(Exception("in_memory_dataType was constructed using the wrong backend.", ioda_Here()));
  }
}



void HH_HasVariables::attachDimensionScales(
  const std::vector<std::pair<Variable, std::vector<Variable>>>& mapping) {
  using std::map;
  using std::make_pair;
  using std::pair;
  using std::shared_ptr;
  using std::vector;

  // Forward mapping.
  // Unravel mapping into something HDF5-specific. We also do not care about the "named"
  // part of the Named_Variables.
  vector<pair<shared_ptr<HH_Variable>, vector<shared_ptr<HH_Variable>>>> hmapping;
  hmapping.reserve(mapping.size());
  for (const auto& m : mapping) {
    auto scaleVarBackend = std::dynamic_pointer_cast<HH_Variable>(m.first.get());
    vector<shared_ptr<HH_Variable>> varsBackend;
    varsBackend.reserve(m.second.size());
    for (const auto& v : m.second) {
      varsBackend.push_back(std::dynamic_pointer_cast<HH_Variable>(v.get()));
    }
    hmapping.emplace_back(make_pair(scaleVarBackend, varsBackend));
  }

  // Construct a mapping of hid_t to variable references.
  // We use this to accelerate lookups of HDF5 references that we are encoding into the
  // scale attributes.


  // Mapping of hid_t to variable references.
  map<hid_t, ref_t> HIDtoVarRef;
  auto EmplaceVarRef = [&HIDtoVarRef](const shared_ptr<HH_Variable> &v) -> void {
    hobj_ref_t ref;
    herr_t err = H5Rcreate(&ref, v->get()(), ".", H5R_OBJECT, -1);
    if (err < 0) throw Exception("H5Rcreate failed.", ioda_Here());
    HIDtoVarRef[v->get()()] = ref;
  };
  for (const auto& m : hmapping) {
    EmplaceVarRef(m.first);
    for (const auto& v : m.second) EmplaceVarRef(v);
  }

  // Construct forward (var.atts referencing scales) and reverse (scale att referencing vars)
  // mappings. We need to write both to the file.

  // Forward mapping of variables and object references.
  // vector<vector<ref_t>> is dimension number, vector of scales.
  vector<pair<shared_ptr<HH_Variable>, vector<vector<ref_t>>>> VarToScaleMap;
  // Reverse mapping of <scale var address, pair<scale, vector<var variable ref>>>.
  struct VarMapData {
    shared_ptr<HH_Variable> scale;
    vector<ds_list_t> vars;
  };
  map<haddr_t, VarMapData> ScaleToVarMap;

  for (auto& m : hmapping) {
    shared_ptr<HH_Variable> v                     = m.first;
    const vector<shared_ptr<HH_Variable>>& scales = m.second;
    vector<vector<ref_t>> refScalesForVar(gsl::narrow<size_t>(v->getDimensions().dimensionality));
    for (unsigned i = 0; i < gsl::narrow<unsigned>(scales.size()); ++i) {
      if (i >= refScalesForVar.size()) {
        // Indicates that there are more scales than variable dimensions, so user error.
        throw Exception("There are more scales than variable dimensions.", ioda_Here());
      }
      hid_t scale_ht = scales[i]->get()();
#if H5_VERSION_GE(1, 12, 0)
      H5O_info1_t info;
#else
      H5O_info_t info;
#endif
      if (H5Oget_info1(scale_ht, &info) < 0) throw Exception("H5Oget_info failed.", ioda_Here());

      // Forward mapping
      refScalesForVar[i].emplace_back(HIDtoVarRef.at(scale_ht));
      // Reverse mapping
      if (!ScaleToVarMap.count(info.addr))  // Make new entry if it does not already exist.
      {
        VarMapData newdata;
        newdata.scale = scales[i];
        ScaleToVarMap.emplace(make_pair(info.addr, newdata));
      }
        
      ScaleToVarMap[info.addr].vars.push_back(ds_list_t{HIDtoVarRef.at(v->get()()), i});
    }
    VarToScaleMap.emplace_back(make_pair(v, refScalesForVar));
  }

  // Append VarToScaleMap and ScaleToVarMap to the DIMENSION_LIST and REFERENCE_LIST
  // attributes of all datasets. If these attributes do not exist, create them.

  // Variables get DIMENSION_LISTs.
  for (auto& var_scale : VarToScaleMap) {
    auto& var    = var_scale.first;
    auto& scales = var_scale.second;

    attr_update_dimension_list(var.get(), scales);
  }
  // Scales get REFERENCE_LISTs.
  for (auto& scale_var : ScaleToVarMap) {
    auto& scale_addr = scale_var.first;
    auto& scale      = scale_var.second.scale;
    auto& vars       = scale_var.second.vars;

    attr_update_reference_list(scale.get(), vars);
  }
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
