/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-util.cpp
 * \brief HDF5 utility functions.
 */

#include "./HH/HH-util.h"

#include <hdf5_hl.h>
#include <algorithm>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "./HH/Handles.h"
#include "./HH/HH-attributes.h"
#include "./HH/HH-variables.h"
#include "ioda/Exception.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {

  /// Callback function for H5Aiterate / H5Aiterate2 / H5Aiterate1.
#if H5_VERSION_GE(1, 8, 0)
herr_t iterate_find_attr(hid_t loc_id, const char* name, const H5A_info_t* info, void* op_data) {
#else
herr_t iterate_find_attr(hid_t loc_id, const char* name, void* op_data) {
  H5A_info_t in_info;
  H5Aget_info_by_name(loc_id, ".", name, &in_info, H5P_DEFAULT);)
  H5A_info_t* info = &in_info;
#endif

  Iterator_find_attr_data_t* op
    = (Iterator_find_attr_data_t*)op_data;  // NOLINT: HDF5 mandates that op_data be void*.

  if (name == nullptr) return -1;
  if (std::string(name) == op->search_for) {
    op->success = true;
    return 1;
  }
  op->idx++;
  return 0;
}

H5_index_t getAttrCreationOrder(hid_t obj, H5O_type_t objType) {
  if (objType != H5O_TYPE_DATASET && objType != H5O_TYPE_GROUP)
    throw Exception("Invalid object type", ioda_Here());
  // Apparently files do not have a creation plist. They show up as groups,
  // so we catch this and return H5_INDEX_NAME.
  if (objType == H5O_TYPE_GROUP) {
    H5I_type_t typ = H5Iget_type(obj); // NOLINT
    if (typ < 0) throw Exception("Error determining object type", ioda_Here());
    if (typ == H5I_FILE) return H5_INDEX_NAME; // NOLINT
  }
  
  unsigned crt_order_flags = 0;
  hid_t hcreatepl
    = (objType == H5O_TYPE_DATASET) ? H5Dget_create_plist(obj) : H5Gget_create_plist(obj);
  HH_hid_t createpl(hcreatepl, Handles::Closers::CloseHDF5PropertyList::CloseP);

  if (createpl() < 0) throw Exception("Cannot get creation property list", ioda_Here());
  if (0 > H5Pget_attr_creation_order(createpl(), &crt_order_flags))
    throw Exception("Cannot get attribute creation order", ioda_Here());
  // We only care if this property is tracked. Indexing is performed on the fly
  // if it is not available (and incurs a read penalty), but this has performance of
  // at least ordering by name.
  return (crt_order_flags & H5P_CRT_ORDER_TRACKED)  // NOLINT
           ? H5_INDEX_CRT_ORDER
           : H5_INDEX_NAME;
}

std::pair<bool, hsize_t> iterativeAttributeSearch(hid_t baseObject, const char* attname, H5_index_t iteration_type) {
  
  // H5Aiterate2 exists in v1.8 and up.
  hsize_t pos = 0;
  Iterator_find_attr_data_t opts;
  opts.search_for = std::string(attname);
  herr_t append_ret
    = H5Aiterate2(baseObject,                     // Search on this dataset
                  iteration_type,                 // Iterate by name or creation order
                  H5_ITER_NATIVE,                 // Fastest ordering possible
                  &pos,                           // Initial (and current) index
                  iterate_find_attr,              // C-style search function
                  reinterpret_cast<void*>(&opts)  // Data passed to/from the C-style search function
    );
  if (append_ret < 0) return std::pair<bool, hsize_t>(false, -2);
  return std::pair<bool, hsize_t>(opts.success, opts.idx);
}

HH_Attribute iterativeAttributeSearchAndOpen(hid_t baseObject, H5O_type_t objType,
                                             const char* attname) {
  // Get search order
  H5_index_t iteration_type = getAttrCreationOrder(baseObject, objType);
  auto searchres            = iterativeAttributeSearch(baseObject, attname, iteration_type);
  bool searchSuccess        = searchres.first;
  hsize_t idx               = searchres.second;

  HH_Attribute aDims_HH;
  // Either create the attribute and set its initial data or open the attribute and append.
  if (searchSuccess) {
    // Open attribute and read data
    hid_t found_att = H5Aopen_by_idx(baseObject, ".", iteration_type, H5_ITER_NATIVE, idx,
                                     H5P_DEFAULT, H5P_DEFAULT);
    if (found_att < 0) throw Exception("Cannot open attribute by index", ioda_Here());
    aDims_HH
      = HH_Attribute(HH_hid_t(std::move(found_att), Handles::Closers::CloseHDF5Attribute::CloseP));
  } else {
    aDims_HH
      = HH_Attribute(HH_hid_t());
  }

  return aDims_HH;
}


void attr_update_dimension_list(HH_Variable* var, const std::vector<std::vector<ref_t>>& new_dim_list) {
  hid_t var_id = var->get()();

  auto dims = var->getDimensions();
  // dimension of the "DIMENSION_LIST" array
  hsize_t hdims[1] = {gsl::narrow<hsize_t>(dims.dimensionality)};
  // The attribute's dataspace
  HH_hid_t sid{-1, Handles::Closers::CloseHDF5Dataspace::CloseP};
  if ((sid = H5Screate_simple(1, hdims, NULL)).get() < 0)
    throw Exception("Cannot create simple dataspace.", ioda_Here());
  // The attribute's datatype
  HH_hid_t tid{-1, Handles::Closers::CloseHDF5Datatype::CloseP};
  if ((tid = H5Tvlen_create(H5T_STD_REF_OBJ)).get() < 0)
    throw Exception("Cannot create variable length array type.", ioda_Here());
  // Check if the DIMENSION_LIST attribute exists.
  HH_Attribute aDimList = iterativeAttributeSearchAndOpen(var_id, H5O_TYPE_DATASET, DIMENSION_LIST);
  // If the DIMENSION_LIST attribute does not exist, create it.
  // If it exists, read it.
  using std::vector;
  vector<hvl_t> dimlist_in_data(dims.dimensionality);
  if (!aDimList.get().isValid()) {
    // Create
    hid_t aid = H5Acreate(var_id, DIMENSION_LIST, tid(), sid(), H5P_DEFAULT, H5P_DEFAULT);
    if (aid < 0) throw Exception("Cannot create attribute", ioda_Here());

    aDimList = HH_Attribute(HH_hid_t(aid, Handles::Closers::CloseHDF5Attribute::CloseP));
    // Initialize dimlist_in_data to nulls. Used in merge operation later.
    for (auto & d : dimlist_in_data) {
      d.len = 0;
      d.p   = nullptr;
    }
  } else {
    // Read
    if (H5Aread(aDimList.get()(), tid(), static_cast<void*>(dimlist_in_data.data())) < 0)
      throw Exception("Cannot read attribute", ioda_Here());
  }
  
  // Allocate a new list that combines any previous DIMENSION_LIST with ref_axis.
  vector<hvl_t> dimlist_out_data(dims.dimensionality);
  // Merge the new allocations with any previous DIMENSION_LIST entries.
  // NOTE: Memory is explicitly freed at the end of the function. Since we are
  // using pure C function calls, throws will not happen even if we run out of memory.
  for (size_t dim = 0; dim < gsl::narrow<size_t>(dims.dimensionality); ++dim) {
    View_hvl_t<hobj_ref_t> olddims(dimlist_in_data[dim]);
    const std::vector<ref_t>& newdims = new_dim_list[dim];
    View_hvl_t<hobj_ref_t> outdims(dimlist_out_data[dim]);
    
    // TODO(ryan)!!!!!
    // When merging, check if any references are equal.
    // Given our intended usage of this function, this may never happen.

    // Note the names here. old + new = out;
    outdims.resize(olddims.size() + newdims.size());
    for (size_t i = 0; i < olddims.size(); ++i) {
      *outdims[i] = *olddims[i];
    }
    for (size_t i = 0; i < newdims.size(); ++i) {
      *outdims[i + olddims.size()] = newdims[i];
    }
  }

  // Write the new list. Deallocate the "old" lists after write, and then check for any errors.
  // Pure C code in between, so the deallocation always occurs.
  herr_t write_success = H5Awrite(aDimList.get()(), tid(), static_cast<void*>(dimlist_out_data.data()));

  // Deallocate old memory
  H5Dvlen_reclaim(tid.get(), sid.get(), H5P_DEFAULT, reinterpret_cast<void*>(dimlist_in_data.data()));
  for (size_t dim = 0; dim < gsl::narrow<size_t>(dims.dimensionality); ++dim) {
    View_hvl_t<hobj_ref_t> outdims(dimlist_out_data[dim]);
    // The View_hvl_t is a "view" that allows us to reinterpret dimlist_out_data[dim]
    // as a sequence of hobj_ref_t objects.
    // We use the resize method to trigger a deliberate free of held memory
    // that can result from the above merge step.
    outdims.resize(0);
  }

  if (write_success < 0) throw Exception("Failed to write DIMENSION_LIST.", ioda_Here());
}

HH_hid_t attr_reference_list_type() {
  static HH_hid_t tid{-1, Handles::Closers::CloseHDF5Datatype::CloseP};
  if (!tid.isValid()) {
    // The attribute's datatype
    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(ds_list_t))).get() < 0)
      throw Exception("Cannot create compound datatype.", ioda_Here());
    if (H5Tinsert(tid(), "dataset", HOFFSET(ds_list_t, ref), H5T_STD_REF_OBJ) < 0)
      throw Exception("Cannot create compound datatype.", ioda_Here());
    if (H5Tinsert(tid(), "dimension", HOFFSET(ds_list_t, dim_idx), H5T_NATIVE_INT) < 0)
      throw Exception("Cannot create compound datatype.", ioda_Here());
  }
  return tid;
}

HH_hid_t attr_reference_list_space(hsize_t numrefs) {
  HH_hid_t sid{-1, Handles::Closers::CloseHDF5Dataspace::CloseP};
  
  // dimension of the "REFERENCE_LIST" array
  const hsize_t hdims[1] = {numrefs};
  // The attribute's dataspace
  if ((sid = H5Screate_simple(1, hdims, NULL)).get() < 0)
    throw Exception("Cannot create simple dataspace.", ioda_Here());
  return sid;
}

void attr_update_reference_list(HH_Variable* scale, const std::vector<ds_list_t>& ref_var_axis) {
  using std::vector;
  HH_hid_t type  = attr_reference_list_type();
  hid_t scale_id = scale->get()();

  // The REFERENCE_LIST attribute must be deleted and re-created each time
  // new references are added.
  // For the append operation, first check whether the attribute exists.
  vector<ds_list_t> oldrefs;
  HH_Attribute aDimListOld
    = iterativeAttributeSearchAndOpen(scale_id, H5O_TYPE_DATASET, REFERENCE_LIST);
  if (aDimListOld.get().isValid()) {
    oldrefs.resize(gsl::narrow<size_t>(aDimListOld.getDimensions().numElements));
    if (H5Aread(aDimListOld.get()(), type(), oldrefs.data()) < 0)
      throw Exception("Cannot read REFERENCE_LIST attribute.", ioda_Here());
    // Release the object handle and delete the attribute.
    aDimListOld = HH_Attribute();
    scale->atts.remove(REFERENCE_LIST);
  }

  vector<ds_list_t> refs = oldrefs;
  refs.reserve(ref_var_axis.size() + oldrefs.size());
  std::copy(ref_var_axis.begin(), ref_var_axis.end(), std::back_inserter(refs));

  // Create the new REFERENCE_LIST attribute.
  HH_hid_t sid = attr_reference_list_space(gsl::narrow<hsize_t>(refs.size()));
  hid_t aid    = H5Acreate(scale_id, REFERENCE_LIST, type(), sid(), H5P_DEFAULT, H5P_DEFAULT);
  if (aid < 0) throw Exception("Cannot create new REFERENCE_LIST attribute.", ioda_Here());
  HH_Attribute newAtt(HH_hid_t(aid, Handles::Closers::CloseHDF5Attribute::CloseP));
  if (H5Awrite(newAtt.get()(), type(), static_cast<void*>(refs.data())) < 0)
    throw Exception("Cannot write REFERENCE_LIST attribute.", ioda_Here());
}

std::string getNameFromIdentifier(hid_t obj_id) {
  ssize_t sz = H5Iget_name(obj_id, nullptr, 0);
  if (sz < 0) throw Exception("Cannot get object name", ioda_Here());
  std::vector<char> data(sz + 1, 0);
  ssize_t ret = H5Iget_name(obj_id, data.data(), data.size());
  if (ret < 0) throw Exception("Cannot get object name", ioda_Here());
  return std::string(data.data());
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
