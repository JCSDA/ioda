#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_hh
 *
 * @{
 * \file HH-util.h
 * \brief Utility functions for HDF5.
 */

#include <string>
#include <utility>
#include <vector>

#include <hdf5.h>
#include <gsl/gsl-lite.hpp>

#include "./Handles.h"
#include "ioda/defs.h"
#include "ioda/Exception.h"
#include "ioda/Types/Marshalling.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
class HH_Attribute;
class HH_Variable;

//#if H5_VERSION_GE(1, 12, 0)
//typedef H5R_ref_t ref_t;
//#else
typedef hobj_ref_t ref_t;
//#endif

// brief Conveys information that a variable (or scale) is attached along a specified axis.
//typedef std::pair<ref_t, unsigned> ref_axis_t;

/// @brief Duplicate the HDF5 dataset list structure for REFERENCE_LISTs.
struct ds_list_t {
  hobj_ref_t ref;       /* object reference  */
  unsigned int dim_idx; /* dimension index of the dataset */
};

/// Data to pass to/from iterator classes.
struct IODA_HIDDEN Iterator_find_attr_data_t {
  std::string search_for;
  hsize_t idx  = 0;
  bool success = false;
};

/// Callback function for H5Aiterate / H5Aiterate2 / H5Aiterate1.
#if H5_VERSION_GE(1, 8, 0)
IODA_HIDDEN herr_t iterate_find_attr(hid_t loc_id, const char* name, const H5A_info_t* info,
                                     void* op_data);
#else
IODA_HIDDEN herr_t iterate_find_attr(hid_t loc_id, const char* name, void* op_data);
#endif

/*! @brief Determine attribute creation order for a dataset.
 *  @param obj is the dataset being queried
 *  @param objType is the type of object (Dataset or Group).
 *  @return H5_INDEX_CRT_ORDER if creation order is tracked.
 *  @return H5_INDEX_NAME if creation order is not tracked.
 *  @details Check if the variable has attribute creation order stored and/or indexed.
 * This is not the default, but it can speed up object list accesses considerably.
 */
IODA_HIDDEN H5_index_t getAttrCreationOrder(hid_t obj, H5O_type_t objType);

/// @brief Linear search to find an attribute.
/// @param baseObject is the object that could contain the attribute.
/// @param attname is the name of the attribute.
/// @param iteration_type is the type of iteration for the search. See getAttrCreationOrder.
/// @return A pair of (success_flag, index).
IODA_HIDDEN std::pair<bool, hsize_t> iterativeAttributeSearch(hid_t baseObject,
                                                              const char* attname,
                                                              H5_index_t iteration_type);

/// @brief Linear search to find and open an attribute, if it exists.
/// @param baseObject is the object that could contain the attribute.
/// @param objType is the type of object (Dataset or Group).
/// @param attname is the name of the attribute.
/// @return An open handle to the attribute, if it exists.
/// @return An invalid handle if the attribute does not exist or upon general failure.
/// @details This function is useful because it is faster than the regular attribute
///   open by name routine, which does not take advantage of attribute creation order
///   indexing. Performance is particularly good when there are few attributes attached
///   to the base object.
IODA_HIDDEN HH_Attribute iterativeAttributeSearchAndOpen(hid_t baseObject, H5O_type_t objType,
                                                         const char* attname);

/*! @brief Attribute DIMENSION_LIST update function
* 
* @details This function exists to update DIMENSION_LISTs without updating the
* mirrored REFERENCE_LIST entry in the variable's scales. This is done for
* performance reasons, as attaching dimension scales for hundreds of
* variables sequentially is very slow.
*
* NOTE: This code does not use the regular atts.open(...) call
* for performance reasons when we have to repeat this call for hundreds or
* thousands of variables. We instead do a creation-order-preferred search.
*
* @param var is the variable of interest.
* @param new_dim_list is the mapping of dimensions that should be added to the variable.
*/
IODA_HIDDEN void attr_update_dimension_list(HH_Variable* var,
                                            const std::vector<std::vector<ref_t>>& new_dim_list);

/*! @brief Attribute REFERENCE_LIST update function
* 
* @details This function exists to update REFERENCE_LISTs without updating the
* mirrored DIMENSION_LIST entry. This is done for
* performance reasons, as attaching dimension scales for hundreds of
* variables sequentially is very slow.
*
* NOTE: This code does not use the regular atts.open(...) call
* for performance reasons when we have to repeat this call for hundreds or
* thousands of variables. We instead do a creation-order-preferred search.
*
* @param scale is the scale of interest.
* @param ref_var_axis_list is the mapping of variables-dimension numbers that
*   should be added to the scale's REFERENCE_LIST attribute.
*/
IODA_HIDDEN void attr_update_reference_list(HH_Variable* scale,
                                            const std::vector<ds_list_t>& ref_var_axis);



/// @brief A "view" of hvl_t objects. Adds C++ conveniences to an otherwise troublesome class.
/// @tparam Inner is the datatype that we are really manipulating.
template <class Inner>
struct IODA_HIDDEN View_hvl_t {
  hvl_t &obj;
  View_hvl_t(hvl_t& obj) : obj{obj} {}
  size_t size() const { return obj.len; }
  void resize(size_t newlen) {
    if (newlen) {
      obj.p = (obj.p) ? H5resize_memory(obj.p, newlen * sizeof(Inner))
        : H5allocate_memory(newlen * sizeof(Inner), false);
      if (!obj.p) throw Exception("Failed to allocate memory", ioda_Here());
    }
    else {
      if (obj.p)
        if (H5free_memory(obj.p) < 0) throw Exception("Failed to free memory", ioda_Here());
      obj.p = nullptr;
    }
    obj.len = newlen;
  }
  void clear() { resize(0); }
  Inner* at(size_t i) {
    if (i >= obj.len) throw Exception("Out-of-bounds access", ioda_Here())
      .add("i", i).add("obj.len", obj.len);
    return operator[](i);
  }
  Inner* operator[](size_t i) {
    return &(static_cast<Inner*>(obj.p)[i]);
  }
};

/*! Internal structure to encapsulate resources and prevent leaks.
* 
* When reading dimension scales, calls to H5Aread return variable-length arrays
* that must be reclaimed. We use a custom object to encapsulate this.
*/
struct IODA_HIDDEN Vlen_data {
  std::unique_ptr<hvl_t[]> buf;  // NOLINT: C array and visibility warnings.
  HH_hid_t typ, space;           // NOLINT: C visibility warnings.
  size_t sz;
  Vlen_data(size_t sz, HH_hid_t typ, HH_hid_t space)
      : buf(new hvl_t[sz]), typ{typ}, space{space}, sz{sz} {
    if (!buf) throw Exception("Failed to allocate buf", ioda_Here());
    for (size_t i = 0; i < sz; i++) {
      buf[i].len = 0;
      buf[i].p   = nullptr;
    }
  }
  ~Vlen_data() {
    if (buf) {
      /*
      for (size_t i = 0; i < sz; i++) {
        if (buf[i].p) delete[] buf[i].p;
        buf[i].len = 0;
        buf[i].p   = nullptr;
      }
      */

      H5Dvlen_reclaim(typ.get(), space.get(), H5P_DEFAULT, reinterpret_cast<void*>(buf.get()));
    }
  }
  Vlen_data(const Vlen_data&) = delete;
  Vlen_data(Vlen_data&&)      = delete;
  Vlen_data operator=(const Vlen_data&) = delete;
  Vlen_data operator=(Vlen_data&&) = delete;

  hvl_t& operator[](size_t idx) { return (buf).get()[idx]; }
};

/// @brief Gets a variable / group / link name from an id. Useful for debugging.
/// @param obj_id is the object.
/// @return One of the possible object names.
/// @throws ioda::Exception if obj_id is invalid.
IODA_HIDDEN std::string getNameFromIdentifier(hid_t obj_id);

/// @brief Convert from variable-length data to fixed-length data.
/// @param in_buf is the input buffer. Buffer has a sequence of pointers, serialized as chars.
/// @param unitLength is the length of each fixed-length element.
/// @param lengthsAreExact signifies that padding is not needed between elements. Each
///   variable-length string is already of length unitLength. If false, then strlen
///   is calculated for each element (which finds the first null byte).
/// @returns the output buffer.
IODA_HIDDEN std::vector<char> convertVariableLengthToFixedLength(
  gsl::span<const char> in_buf, size_t unitLength, bool lengthsAreExact);

/// @brief Convert from fixed-length data to variable-length data.
/// @param in_buf is the input buffer. Buffer is a sequence of fixed-length elements (*not* pointers).
/// @param unitLength is the length of each fixed-length element.
/// @returns the converted buffer.
IODA_HIDDEN Marshalled_Data<char*, char*, true> convertFixedLengthToVariableLength(
  gsl::span<const char> in_buf, size_t unitLength);

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
