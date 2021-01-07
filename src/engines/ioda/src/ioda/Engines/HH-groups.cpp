/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Engines/HH/HH-groups.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <set>
#include <string>

#include "ioda/Misc/Dimensions.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
Group HH_Group_Backend::create(const std::string& name) {
  ::HH::GroupParameterPack params;
  // We want fast reads of data. Indexing may to be turned off
  // to enable faster writes, but this situation's impact remains to be assessed.
  params.groupCreationProperties.set_link_creation_order = true;
  params.linkCreationProperties.CreateIntermediateGroups = true;

  auto backend =
    std::make_shared<HH_Group_Backend>(backend_->create(name, params), caps_, *(fileroot_.get()));
  return ::ioda::Group{backend};
}

Group HH_Group_Backend::open(const std::string& name) const {
  ::HH::Group hhg = backend_->open(name);
  auto backend = std::make_shared<HH_Group_Backend>(hhg, caps_, *(fileroot_.get()));
  return ::ioda::Group{backend};
}

/// Data to pass to/from iterator classes.
struct Iterator_data_t {
  std::map<ObjectType, std::list<std::string>> lists;  // NOLINT: Visibility
  Iterator_data_t() {
    lists[ObjectType::Group] = std::list<std::string>();
    lists[ObjectType::Variable] = std::list<std::string>();
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
  herr_t err = H5Oget_info_by_name1(g_id, name, &oinfo, H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#else
  H5O_info_t oinfo;
  herr_t err = H5Oget_info_by_name(g_id, name, &oinfo, H5P_DEFAULT);  // H5P_DEFAULT only, per docs.
#endif
  if (err < 0) return -1;

  if (oinfo.type == H5O_type_t::H5O_TYPE_GROUP)
    op->lists[ObjectType::Group].emplace_back(name);
  else if (oinfo.type == H5O_type_t::H5O_TYPE_DATASET)
    op->lists[ObjectType::Variable].emplace_back(name);
  else
    op->lists[ObjectType::Unimplemented].emplace_back(name);

  return 0;
}

std::map<ObjectType, std::vector<std::string>> HH_Group_Backend::listObjects(ObjectType filter,
                                                                             bool recurse) const {
  std::map<ObjectType, std::vector<std::string>> res;
  Iterator_data_t iter_data;

  // Check if the group has link creation order stored and/or indexed.
  // This is not the default, but it can speed up object lists considerably.
  // We hope that odd cases do not occur where a parent group preserves the index
  // but its child does not.
  auto backend_hid = backend_->get();
  unsigned crt_order_flags = 0;  // Set only for a group.
  // We don't know yet if backend_hid refers to a file or a group. Check.
  // This matters because link creation order is a group-specific property.
  if (H5I_GROUP == H5Iget_type(backend_hid())) {
    ::HH::HH_hid_t createpl(H5Gget_create_plist(backend_hid()),
                            ::HH::Handles::Closers::CloseHDF5PropertyList::CloseP);
    if (createpl() < 0) throw;                                                // jedi_throw;
    if (0 > H5Pget_link_creation_order(createpl(), &crt_order_flags)) throw;  // jedi_throw;
  }
  // We only care if this property is tracked. Indexing is performed on the fly
  // if it is not available (and incurs a read penalty).
  // Oddly, the HDF5 documentation suggests that this parameter is unnecessary.
  //  The functions should fall back to indexing by name if the creation order index
  //  is unavailable. But, that's not what we observed.
  H5_index_t idxclass = (crt_order_flags & H5P_CRT_ORDER_TRACKED)  // NOLINT
                          ? H5_INDEX_CRT_ORDER
                          : H5_INDEX_NAME;

  herr_t search_res =
    (recurse) ? H5Lvisit(backend_->get()(), idxclass, H5_ITER_NATIVE, iterate_find_by_link,
                         reinterpret_cast<void*>(&iter_data))
              : H5Literate(backend_->get()(), idxclass, H5_ITER_NATIVE, 0, iterate_find_by_link,
                           reinterpret_cast<void*>(&iter_data));  // NOLINT: 0 is not a nullptr here.

  if (search_res < 0) throw;  // jedi_throw;

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
