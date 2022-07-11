/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-groups.cpp
 * \brief HDF5 engine implementation of Group.
 */
#include "./HH/HH-groups.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <set>
#include <string>
#include <utility>

#include "./HH/HH-hasattributes.h"
#include "./HH/HH-hastypes.h"
#include "./HH/HH-hasvariables.h"
#include "./HH/Handles.h"
#include "ioda/Exception.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Misc/StringFuncs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
HH_Group::HH_Group(HH_hid_t grp, ::ioda::Engines::Capabilities caps, HH_hid_t fileroot)
    : backend_(grp), fileroot_(fileroot), caps_(caps) {
  atts = Has_Attributes(std::make_shared<HH_HasAttributes>(grp));
  types = Has_Types(std::make_shared<HH_HasTypes>(grp));
  vars = Has_Variables(std::make_shared<HH_HasVariables>(grp, fileroot));
}

Group HH_Group::create(const std::string& name) {
  // Group creation property list
  HH_hid_t groupCreationProps(H5Pcreate(H5P_GROUP_CREATE),
                              Handles::Closers::CloseHDF5PropertyList::CloseP);
  if (!groupCreationProps.isValid()) throw Exception("H5Pcreate failed.", ioda_Here());
  if (0 > H5Pset_link_creation_order(groupCreationProps(),
                                     H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED))
    throw Exception("H5Pset_link_creation_order failed.", ioda_Here());

  // Link creation property list
  HH_hid_t linkCreationProps(H5Pcreate(H5P_LINK_CREATE),
                             Handles::Closers::CloseHDF5PropertyList::CloseP);
  if (!linkCreationProps.isValid()) throw Exception("H5Pcreate failed.", ioda_Here());
  if (0 > H5Pset_create_intermediate_group(linkCreationProps(), 1))
    throw Exception("H5Pset_create_intermediate_group failed.", ioda_Here());

  // Finally, create the group
  hid_t res = H5Gcreate(backend_(),    // hid
                        name.c_str(),  // name in ascii character encoding
                        linkCreationProps(), groupCreationProps(),
                        H5P_DEFAULT  // Group access property list
  );
  if (res < 0) throw Exception("H5Gcreate failed.", ioda_Here());
  HH_hid_t hnd(res, Handles::Closers::CloseHDF5Group::CloseP);

  auto backend = std::make_shared<HH_Group>(hnd, caps_, fileroot_);
  return ::ioda::Group{backend};
}

Group HH_Group::open(const std::string& name) const {
  hid_t g = H5Gopen(backend_(), name.c_str(), H5P_DEFAULT);
  if (g < 0) throw Exception("H5Gopen failed.", ioda_Here());
  HH_hid_t grp_handle(g, Handles::Closers::CloseHDF5Group::CloseP);

  auto res = std::make_shared<HH_Group>(grp_handle, caps_, fileroot_);
  return ::ioda::Group{res};
}

bool HH_Group::exists(const std::string& name) const {
  auto paths = splitPaths(name);
  for (size_t i = 0; i < paths.size(); ++i) {
    auto p            = condensePaths(paths, 0, i + 1);
    htri_t linkExists = H5Lexists(backend_(), p.c_str(), H5P_DEFAULT);
    if (linkExists < 0) throw Exception("H5Lexists failed.", ioda_Here());
    if (linkExists == 0) return false;
  }

  // Check that the object is a group
#if H5_VERSION_GE(1, 12, 0)
  H5O_info1_t obj_info;
  herr_t err = H5Oget_info_by_name1(backend_(), name.c_str(), &obj_info, H5P_DEFAULT);
#else
  H5O_info_t obj_info;
  herr_t err = H5Oget_info_by_name(backend_(), name.c_str(), &obj_info, H5P_DEFAULT);
#endif
  if (err < 0) throw Exception("H5Oget_info_by_name failed.", ioda_Here());
  return (obj_info.type == H5O_TYPE_GROUP);
}

FillValuePolicy HH_Group::getFillValuePolicy() const { return vars.getFillValuePolicy(); }

/// Data to pass to/from iterator classes.
struct Iterator_data_t {
  std::map<ObjectType, std::list<std::string>> lists;  // NOLINT: Visibility
  Iterator_data_t() {
    lists[ObjectType::Group]         = std::list<std::string>();
    lists[ObjectType::Variable]      = std::list<std::string>();
    lists[ObjectType::Unimplemented] = std::list<std::string>();
  }
};

/// Callback function for H5Lvisit / H5Literate.
#if H5_VERSION_GE(1, 12, 0)
herr_t iterate_find_by_link(hid_t g_id, const char* name, const H5L_info2_t* info, void* op_data)
#else
herr_t iterate_find_by_link(hid_t g_id, const char* name, const H5L_info_t* info, void* op_data)
#endif
{
  Iterator_data_t* op = (Iterator_data_t*)op_data;  // NOLINT: HDF5 mandates that op_data be void*.

  // HARD, SOFT, EXTERNAL are all valid link types in HDF5. We only implement hard
  // links for now.
  if (info->type != H5L_TYPE_HARD) {
    op->lists[ObjectType::Unimplemented].emplace_back(name);
    return 0;
  }

  // Get the object and check the type
#if H5_VERSION_GE(1, 12, 0)
  H5O_info1_t oinfo;
  herr_t err
    = H5Oget_info_by_name1(g_id, name, &oinfo, H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#else
  H5O_info_t oinfo;
  herr_t err = H5Oget_info_by_name(g_id, name, &oinfo, H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#endif
  if (err < 0) return -1;

  if (oinfo.type == H5O_type_t::H5O_TYPE_GROUP)
    op->lists[ObjectType::Group].emplace_back(name);
  else if (oinfo.type == H5O_type_t::H5O_TYPE_DATASET)
    op->lists[ObjectType::Variable].emplace_back(name);
  else if (oinfo.type == H5O_type_t::H5O_TYPE_NAMED_DATATYPE)
    op->lists[ObjectType::Type].emplace_back(name);
  else
    op->lists[ObjectType::Unimplemented].emplace_back(name);

  return 0;
}

std::map<ObjectType, std::vector<std::string>> HH_Group::listObjects(ObjectType filter,
                                                                     bool recurse) const {
  std::map<ObjectType, std::vector<std::string>> res;
  Iterator_data_t iter_data;

  // Check if the group has link creation order stored and/or indexed.
  // This is not the default, but it can speed up object lists considerably.
  // We hope that odd cases do not occur where a parent group preserves the index
  // but its child does not.
  unsigned crt_order_flags = 0;  // Set only for a group.
  // We don't know yet if backend_hid refers to a file or a group. Check.
  // This matters because link creation order is a group-specific property.
  if (H5I_GROUP == H5Iget_type(backend_())) {
    HH_hid_t createpl(H5Gget_create_plist(backend_()),
                      Handles::Closers::CloseHDF5PropertyList::CloseP);
    if (createpl() < 0) throw Exception("H5Gget_create_plist failed.", ioda_Here());
    if (0 > H5Pget_link_creation_order(createpl(), &crt_order_flags))
      throw Exception("H5Pget_link_creation_order failed.", ioda_Here());
  }
  // We only care if this property is tracked. Indexing is performed on the fly
  // if it is not available (and incurs a read penalty).
  // Oddly, the HDF5 documentation suggests that this parameter is unnecessary.
  //  The functions should fall back to indexing by name if the creation order index
  //  is unavailable. But, that's not what we observed.
  H5_index_t idxclass = (crt_order_flags & H5P_CRT_ORDER_TRACKED)  // NOLINT
                          ? H5_INDEX_CRT_ORDER
                          : H5_INDEX_NAME;

  herr_t search_res
    = (recurse)
        ? H5Lvisit(backend_(), idxclass, H5_ITER_NATIVE, iterate_find_by_link,
                   reinterpret_cast<void*>(&iter_data))
        : H5Literate(backend_(), idxclass, H5_ITER_NATIVE, 0, iterate_find_by_link,
                     reinterpret_cast<void*>(&iter_data));  // NOLINT: 0 is not a nullptr here.

  if (search_res < 0) throw Exception("H5Lvisit / H5Literate failed.", ioda_Here())
    .add("recurse", recurse);

  for (auto& cls : iter_data.lists) {
    if (filter == ObjectType::Ignored || filter == cls.first)
      res[cls.first] = std::vector<std::string>(std::make_move_iterator(cls.second.begin()),
                                                std::make_move_iterator(cls.second.end()));
  }

  return res;
}
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
