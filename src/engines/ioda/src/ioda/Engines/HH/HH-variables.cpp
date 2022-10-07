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
 * \file HH-variables.cpp
 * \brief HDF5 engine implementation of Variable.
 */

#include "./HH/HH-variables.h"

#include <hdf5_hl.h>

#include <algorithm>
#include <exception>
#include <numeric>
#include <set>

#include "./HH/HH-Filters.h"
#include "./HH/HH-attributes.h"
#include "./HH/HH-hasattributes.h"
#include "./HH/HH-hasvariables.h"
#include "./HH/HH-types.h"
#include "./HH/HH-util.h"
#include "./HH/Handles.h"
#include "ioda/Exception.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Misc/StringFuncs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {


template <class T>
std::vector<T> convertToH5Length(const std::vector<Dimensions_t>& in) {
  std::vector<T> res(in.size());
  for (size_t i = 0; i < in.size(); ++i) res[i] = gsl::narrow<T>(in[i]);
  return res;
}

HH_Variable::HH_Variable()  = default;
HH_Variable::~HH_Variable() = default;
HH_Variable::HH_Variable(HH_hid_t d, std::shared_ptr<const HH_HasVariables> container)
    : var_(d), container_(container) {
  atts = Has_Attributes(std::make_shared<HH_HasAttributes>(d));
  // auto has_atts_backend = Variable_Backend::_getHasAttributesBackend(backend_->atts);
  // auto hh_atts_backend  = std::dynamic_pointer_cast<HH_HasAttributes>(has_atts_backend);
  // atts                  = Has_Attributes(hh_atts_backend);
  // atts = Has_Attributes(std::make_shared<HH_HasAttributes>(backend_->atts));
}

HH_hid_t HH_Variable::get() const { return var_; }

bool HH_Variable::isVariable() const {
  H5I_type_t typ = H5Iget_type(var_());
  if (typ == H5I_BADID) throw Exception("Cannot determine object type", ioda_Here());
  return (typ == H5I_DATASET);
}

detail::Type_Provider* HH_Variable::getTypeProvider() const { return HH_Type_Provider::instance(); }

HH_hid_t HH_Variable::internalType() const {
  return HH_hid_t(H5Dget_type(var_()), Handles::Closers::CloseHDF5Datatype::CloseP);
}

Type HH_Variable::getType() const {
  return Type{std::make_shared<HH_Type>(internalType()), typeid(HH_Type)};
}

HH_hid_t HH_Variable::space() const {
  return HH_hid_t(H5Dget_space(var_()), Handles::Closers::CloseHDF5Dataspace::CloseP);
}

Dimensions HH_Variable::getDimensions() const {
  Dimensions ret;
  Options errOpts; // Used for tracking parameters that can show up in the error message.
  errOpts.add("variable", getNameFromIdentifier(var_()));

  std::vector<hsize_t> dims, dimsmax;
  htri_t isSimple = H5Sis_simple(space()());
  if (isSimple < 0) throw Exception("Dimension space parameter is invalid.", ioda_Here(), errOpts);
  if (isSimple == 0) throw Exception("Dataspace is not simple. Unsupported case in code. "
    "Complex dataspace support was not available in HDF5 when this function was written.",
      ioda_Here(), errOpts);
  hssize_t numPoints = H5Sget_simple_extent_npoints(space()());
  errOpts.add("numPoints", numPoints);
  if (numPoints < 0)
    throw Exception("H5Sget_simple_extent_npoints error.", ioda_Here(), errOpts);
  int dimensionality = H5Sget_simple_extent_ndims(space()());
  errOpts.add("dimensionality", dimensionality);
  if (dimensionality < 0)
    throw Exception("H5Sget_simple_extent_ndims error.", ioda_Here(), errOpts);
  dims.resize(dimensionality);
  dimsmax.resize(dimensionality);
  if (H5Sget_simple_extent_dims(space()(), dims.data(), dimsmax.data()) < 0)
    throw Exception("H5Sget_simple_extent_dims error.", ioda_Here(), errOpts);

  ret.numElements    = gsl::narrow<decltype(Dimensions::numElements)>(numPoints);
  ret.dimensionality = gsl::narrow<decltype(Dimensions::dimensionality)>(dimensionality);
  for (const auto& d : dims) ret.dimsCur.push_back(gsl::narrow<Dimensions_t>(d));
  for (const auto& d : dimsmax)
    ret.dimsMax.push_back((d == H5S_UNLIMITED) ? ioda::Unlimited : gsl::narrow<Dimensions_t>(d));

  return ret;
}

Variable HH_Variable::resize(const std::vector<Dimensions_t>& newDims) {
  std::vector<hsize_t> hdims = convertToH5Length<hsize_t>(newDims);

  if (H5Dset_extent(var_(), hdims.data()) < 0)
    throw Exception("Failure to resize a Variable with the HDF5 backend.", ioda_Here())
    .add("variable", getNameFromIdentifier(var_()))
    .add("dimensionality", hdims.size());

  return Variable{shared_from_this()};
}

Variable HH_Variable::attachDimensionScale(unsigned int DimensionNumber, const Variable& scale) {
  Options errOpts;
  errOpts.add("variable", getNameFromIdentifier(var_()));
  errOpts.add("DimensionNumber", DimensionNumber);

  try {
    // We are extracting the backend object.
    auto scaleBackendBase = scale.get();
    // If the backend object is an HH object, then we can attach this scale.
    // Otherwise, throw an error because you can't mix Variables from different
    // backends.
    auto scaleBackendDerived = std::dynamic_pointer_cast<HH_Variable>(scaleBackendBase);
    errOpts.add("scale", getNameFromIdentifier(scaleBackendDerived->var_()));

    const herr_t res = H5DSattach_scale(var_(), scaleBackendDerived->var_(), DimensionNumber);
    if (res != 0) throw Exception("Dimension scale attachment failed.", ioda_Here(), errOpts);

    return Variable{shared_from_this()};
  } catch (std::bad_cast) {
    throw Exception("Cannot attach dimension scales across incompatible backends.",
      ioda_Here(), errOpts);
  }
}

Variable HH_Variable::detachDimensionScale(unsigned int DimensionNumber, const Variable& scale) {
  Options errOpts;
  errOpts.add("variable", getNameFromIdentifier(var_()));
  errOpts.add("DimensionNumber", DimensionNumber);

  try {
    // We are extracting the backend object.
    auto scaleBackendBase = scale.get();
    // If the backend object is an HH object, then we can attach this scale.
    // Otherwise, throw an error because you can't mix Variables from different
    // backends.
    auto scaleBackendDerived = std::dynamic_pointer_cast<HH_Variable>(scaleBackendBase);
    errOpts.add("scale", getNameFromIdentifier(scaleBackendDerived->var_()));

    const herr_t res = H5DSdetach_scale(var_(), scaleBackendDerived->var_(), DimensionNumber);
    if (res != 0) throw Exception("Dimension scale detachment failed", ioda_Here(), errOpts);

    return Variable{shared_from_this()};
  } catch (std::bad_cast) {
    throw Exception("Cannot detach dimension scales across incompatible backends.",
      ioda_Here(), errOpts);
  }
}

bool HH_Variable::isDimensionScale() const {
  const htri_t res = H5DSis_scale(var_());
  if (res < 0) {
    Options errOpts;
    try {
      errOpts.add("variable", getNameFromIdentifier(var_()));
    } catch (...) {
      errOpts.add("variable", "unknown / bad id");
    }
    
    throw Exception("Error returned from H5DSis_scale.", ioda_Here(), errOpts);
  }
  return (res > 0);
}

Variable HH_Variable::setIsDimensionScale(const std::string& dimensionScaleName) {
  const htri_t res = H5DSset_scale(var_(), dimensionScaleName.c_str());
  if (res != 0) {
    Options errOpts;
    errOpts.add("dimensionScaleName", dimensionScaleName);
    try {
      errOpts.add("variable", getNameFromIdentifier(var_()));
    } catch (...) {
      errOpts.add("variable", "unknown / bad id");
    }

    throw Exception(
      "Error returned from H5DSset_scale.", ioda_Here(), errOpts);
  }
  return Variable{shared_from_this()};
}

Variable HH_Variable::getDimensionScaleName(std::string& res) const {
  constexpr size_t max_label_size = 1000;
  std::array<char, max_label_size> label{};  // Value-initialized to nulls.
  const ssize_t sz = H5DSget_scale_name(var_(), label.data(), max_label_size);
  if (sz < 0) {
    Options errOpts;
    try {
      errOpts.add("variable", getNameFromIdentifier(var_()));
    } catch (...) {
      errOpts.add("variable", "unknown / bad id");
    }

    throw Exception("Error returned from H5DSget_scale_name.", ioda_Here(), errOpts);
  }
  // sz is the size of the label. The HDF5 documentation does not include whether the label is
  // null-terminated, so I am terminating it manually.
  label[max_label_size - 1] = '\0';
  res                       = std::string(label.data());
  return Variable{std::make_shared<HH_Variable>(*this)};
}


/** \details This function is byzantine, it is performance-critical, and it cannot be split apart.
 *
 * It serves as the common calling point for both the regular getDimensionScaleMappings function and
 * isDimensionScaleAttached. They share a lot of complex code.
 *
 * The code is detailed because we are working around deficiencies in HDF5's dimension scales API.
 * The key problem is that dimension scale mappings are bi-directional, and H5DSis_attached
 * repeatedly re-opens the **scale's** list of variables that it acts as a dimension for.
 * It then has to de-reference and re-open each variable when verifying attachment, and this
 * performs horribly when you have hundreds or thousands of variables.
 *
 * So, the logic here simplifies H5DSis_attached to verify only a uni-directional mapping
 * so see if a Variable is attached to a scale (and not the other way around).
 *
 * Also note: different HDF5 versions use slightly different structs and function calls,
 * hence the #ifdefs.
 **/
std::vector<std::vector<Named_Variable>> HH_Variable::getDimensionScaleMappings(
  const std::vector<Named_Variable>& scalesToQueryAgainst, bool firstOnly,
  const std::vector<unsigned>& dimensionNumbers_) const {
  try {
    // Extract all of the scalesToQueryAgainst and convert them into the appropriate backend
    // objects. If the backend objects are not from this engine, then this is an error (no mixing
    // Variables and scales across different backends).
    std::vector<std::pair<std::string, std::shared_ptr<HH_Variable>>> scales;
    for (const auto& scale : scalesToQueryAgainst) {
      auto scaleBackendBase    = scale.var.get();
      auto scaleBackendDerived = std::dynamic_pointer_cast<HH_Variable>(scaleBackendBase);
      scales.push_back(
        {scale.name, scaleBackendDerived});  // NOLINT: macos oddly is missing emplace_back here.
    }

    // The logic here roughly follows H5DSis_attached, but with extra optimizations and added
    // loops to avoid variable repeated variable {open and close} operations.

    // Check that the dimensionality is sufficient to have a
    // DimensionNumber element.
    auto datadims                          = getDimensions();
    std::vector<unsigned> dimensionNumbers = dimensionNumbers_;
    if (!dimensionNumbers.empty()) {
      auto max_elem_it = std::max_element(dimensionNumbers.cbegin(), dimensionNumbers.cend());
      if (max_elem_it == dimensionNumbers.cend()) throw Exception(ioda_Here());
      if (datadims.dimensionality <= *max_elem_it) throw Exception(ioda_Here());
    } else {
      dimensionNumbers.resize(datadims.dimensionality);
      std::iota(dimensionNumbers.begin(), dimensionNumbers.end(), 0);
    }

    // The return value. Give it the correct size (the dimensionality of the variable).
    std::vector<std::vector<Named_Variable>> ret(
      gsl::narrow<size_t>(datadims.dimensionality));

    // Iterate over all attributes to get the DIMENSION_LIST attribute. Then,
    // attempt to read the reference list inside the DIMENSION_LIST attribute and link
    // dimension references to dimension scales.

    // NOTE: This code does not use the regular atts.open("DIMENSION_LIST") call
    // for performance reasons on large IASI-like files where we have to repeat this
    // call for tens of thousands of variables. We instead do a creation-order-preferred
    // search.

    // Get search order
    H5_index_t iteration_type = getAttrCreationOrder(get()(), H5O_TYPE_DATASET);
    // H5Aiterate2 exists in v1.8 and up.
    hsize_t pos           = 0;
    Iterator_find_attr_data_t search_data_opts;
    search_data_opts.search_for = "DIMENSION_LIST";
    herr_t att_search_ret
      = H5Aiterate2(get()(),                  // Search on this dataset
                    iteration_type,           // Iterate by name or creation order
                    H5_ITER_NATIVE,           // Fastest ordering possible
                    &pos,                     // Initial (and current) index
                    iterate_find_attr,        // C-style search function
      reinterpret_cast<void*>(&search_data_opts)  // Data passed to/from the C-style search function
      );
    if (att_search_ret < 0) throw Exception(ioda_Here());

    if (!search_data_opts.success) return ret; // Fallthrough returning a vector of empty vectors.

    hid_t found_att = H5Aopen_by_idx(get()(), ".", iteration_type, H5_ITER_NATIVE,
                                     search_data_opts.idx, H5P_DEFAULT, H5P_DEFAULT);
    HH_Attribute aDims_HH(
      HH_hid_t(std::move(found_att), Handles::Closers::CloseHDF5Attribute::CloseP));

    // Attempt to read the reference list for our variable's
    // DIMENSION_LIST attribute.
    //if (!atts.exists("DIMENSION_LIST"))
    //  return ret;  // Fallthrough returning a vector of empty vectors.
    //Attribute aDims    = atts.open("DIMENSION_LIST");
    //auto aDims_backend = _getAttributeBackend<ioda::detail::Attribute_Base<Attribute>>(aDims);
    //auto aDims_HH      = std::dynamic_pointer_cast<HH_Attribute>(aDims_backend);

    auto vltyp  = aDims_HH.internalType();
    auto vldims = aDims_HH.getDimensions();

    Vlen_data buf((size_t)datadims.dimensionality, vltyp, aDims_HH.space());

    if (H5Aread(aDims_HH.get()(), vltyp.get(), reinterpret_cast<void*>(buf.buf.get())) < 0)
      throw Exception("Attribute read failure", ioda_Here());

      // We now have the list of object references.
      // We need to query the scale information.

      // Get the information for all of the scales.
#if H5_VERSION_GE(1, 12, 0)
    std::vector<H5O_info1_t> scale_infos(scales.size());
    H5O_info1_t check_info;
#else
    std::vector<H5O_info_t> scale_infos(scales.size());
    H5O_info_t check_info;
#endif
    for (size_t i = 0; i < scales.size(); ++i) {
#if H5_VERSION_GE(1, 10, 3)
      if (H5Oget_info2(scales[i].second->get()(), &scale_infos[i], H5O_INFO_BASIC) < 0)
        throw Exception("H5Oget_info2 failure", ioda_Here());
#else
      if (H5Oget_info(scales[i].second->get()(), &scale_infos[i], H5O_INFO_BASIC) < 0)
        throw Exception("H5Oget_info failure", ioda_Here());
#endif
    }

    // Iterate over each dimension (in the set), and iterate along each scale.
    // See which scales are attached to which dimensions.

    // For each dimension. The remaining dimensions in the returned vector (i.e. entries not in
    // dimensionNumbers) are unfilled in the output.
    for (const auto& curDim : dimensionNumbers) {
      // For each *scale reference* listed in the variable along a particular dimension
      // Note well: this is NOT *each scale that the user passed*.
      for (size_t i = 0; i < buf.buf[curDim].len; ++i) {
        hobj_ref_t ref = ((hobj_ref_t*)buf.buf[curDim].p)[i];  // NOLINT: type conversions

        // Dereference the scale variable stored in DIMENSION_LIST.
        // This opens it.
        hid_t deref_scale_id = H5Rdereference2(
          // First parameter is any object id in the same file
          get()(), H5P_DEFAULT, H5R_OBJECT, &ref);
        Expects(deref_scale_id >= 0);  // Die on failure. Would have to clean up memory otherwise.
        HH_hid_t encap_id(deref_scale_id, Handles::Closers::CloseHDF5Dataset::CloseP);
        HH_Variable deref_scale(encap_id, nullptr);  // Move into managed memory

        // Get deref_scale's info and compare to scale_info.
#if H5_VERSION_GE(1, 10, 3)
        if (H5Oget_info2(deref_scale.get()(), &check_info, H5O_INFO_BASIC) < 0)
          throw Exception("H5Oget_info2 failure", ioda_Here());
#else
        if (H5Oget_info(deref_scale.get()(), &check_info, H5O_INFO_BASIC) < 0)
          throw Exception("H5Oget_info failure", ioda_Here());
#endif

        // Iterate over each scalesToQueryAgainst
        // I.e. for each *scale that the user passed*.
        bool foundScale = false;
        for (size_t j = 0; j < scale_infos.size(); ++j) {
          if ((scale_infos[j].fileno == check_info.fileno)
              && (scale_infos[j].addr == check_info.addr)) {
            // Success! We matched a scale!
            ret[curDim].push_back(scalesToQueryAgainst[j]);

            foundScale = true;
            break;  // No need to check this *scale reference* against the remaining known scales.
          }
        }
        // If we are asking for only the first matched scale in each dimension,
        // and if we just found the first match, then continue on to the next dimension.
        if (firstOnly && foundScale) break;
      }
    }

    return ret;
  } catch (...) {
    Options errOpts;
    try {
      errOpts.add("variable", getNameFromIdentifier(var_()));
    } catch (...) {
      errOpts.add("variable", "unknown / bad id");
    }

    std::throw_with_nested(Exception("Caught an exception.", ioda_Here(), errOpts));
  }
}

bool HH_Variable::isDimensionScaleAttached(unsigned int DimensionNumber,
                                           const Variable& scale) const {
  try {
    const std::vector<Named_Variable> scalesToQueryAgainst{{"unused_param", scale}};
    auto res = getDimensionScaleMappings(scalesToQueryAgainst, true, {DimensionNumber});
    return !res[DimensionNumber].empty();
  } catch (...) {
    Options errOpts;
    try {
      errOpts.add("variable", getNameFromIdentifier(var_()));
    } catch (...) {
      errOpts.add("variable", "unknown / bad id");
    }

    std::throw_with_nested(Exception("Caught an exception.", ioda_Here(), errOpts));
  }
}

std::vector<std::vector<Named_Variable>> HH_Variable::getDimensionScaleMappings(
  const std::list<Named_Variable>& scalesToQueryAgainst, bool firstOnly) const {
  return getDimensionScaleMappings({scalesToQueryAgainst.begin(), scalesToQueryAgainst.end()},
                                   firstOnly, {});
}

Selections::SelectionBackend_t HH_Variable::instantiateSelection(const Selection& sel) const {
  auto res = std::make_shared<HH_Selection>();
  res->sel = getSpaceWithSelection(sel);
  return res;
}

HH_hid_t HH_Variable::getSpaceWithSelection(const Selection& sel) const {
  if (sel.isConcretized()) {
    auto concretized = sel.concretize();
    try {
      // Only return the concretized selection if this is the correct backend.
      auto csel = std::dynamic_pointer_cast<HH_Selection>(concretized);
      return csel->sel;
    } catch (std::bad_cast) {
      sel.invalidate();
    }
  }
  
  if (sel.getDefault() == SelectionState::ALL)
    if (sel.getActions().empty()) return HH_hid_t(H5S_ALL);
  
  HH_hid_t spc(H5Scopy(space()()), Handles::Closers::CloseHDF5Dataspace::CloseP);
  if (spc() < 0) throw Exception("Cannot copy dataspace.", ioda_Here());

  if (!sel.extent().empty()) {
    if (H5Sset_extent_simple(spc(), gsl::narrow<int>(sel.extent().size()),
                             convertToH5Length<hsize_t>(sel.extent()).data(),
                             convertToH5Length<hsize_t>(sel.extent()).data())
        < 0)
      throw Exception("Cannot set dataspace extent.", ioda_Here());
  }

  if (sel.getDefault() == SelectionState::ALL) {
    if (H5Sselect_all(spc()) < 0) throw Exception("Dataspace selection failed.", ioda_Here());
  } else if (sel.getDefault() == SelectionState::NONE) {
    if (H5Sselect_none(spc()) < 0) throw Exception("Dataspace selection failed.", ioda_Here());
  }

  static const std::map<SelectionOperator, H5S_seloper_t> op_map
    = {{SelectionOperator::SET, H5S_SELECT_SET},
       {SelectionOperator::OR, H5S_SELECT_OR},
       {SelectionOperator::AND, H5S_SELECT_AND},
       {SelectionOperator::XOR, H5S_SELECT_XOR},
       {SelectionOperator::NOT_B, H5S_SELECT_NOTB},
       {SelectionOperator::NOT_A, H5S_SELECT_NOTA},
       {SelectionOperator::APPEND, H5S_SELECT_APPEND},
       {SelectionOperator::PREPEND, H5S_SELECT_PREPEND}};
  bool first_action = true;
  for (const auto& s : sel.getActions()) {
    if (!op_map.count(s.op_)) throw Exception("Unimplemented map value.", ioda_Here());
    herr_t chk = 0;
    // Is this a hyperslab or a single point selection?
    if (!s.points_.empty()) {  // Single point selection

      size_t dimensionality = s.points_.at(0).size();

      std::vector<hsize_t> elems(dimensionality * s.points_.size());

      // Waiting for std::ranges in C++20
      for (size_t i = 0; i < s.points_.size(); ++i)  // const auto& p : s.points_)
      {
        if (s.points_[i].size() != dimensionality)
          throw Exception("Points have inconsistent dimensionalities.", ioda_Here())
          .add("dimensionality", dimensionality)
          .add("s.points_[i].size()", s.points_[i].size())
          .add("i", i);
        for (size_t j = 0; j < dimensionality; ++j)
          elems[j + (dimensionality * i)] = s.points_[i][j];
        // std::copy_n(p.data(), dimensionality, elems.data() + (i * dimensionality));
      }

      chk = H5Sselect_elements(spc(), op_map.at(s.op_), s.points_.size(), elems.data());
    } else if (!s.dimension_indices_starts_.empty()) {
      // This is a variant of the hyperslab selection code.
      // We have index ranges, and we then convert to the usual
      // start and count.
      // SelectionOperator is a problem here, because the selections
      // that we are making are not commutative. So, we do a two-part selection.
      //  First, clone the dataspace and select everything.
      //  Then, apply this bulk selection to the actual space.
#if H5_VERSION_GE(1, 12, 0)
      HH_hid_t cloned_space(H5Scopy(spc()), Handles::Closers::CloseHDF5Dataspace::CloseP);
      if(H5Sselect_none(cloned_space()) < 0) throw Exception("Cannot copy space", ioda_Here());

      ioda::Dimensions dims = getDimensions();
      Expects(s.dimension_ < (size_t)dims.dimensionality);
      const size_t numSlabs = s.dimension_indices_starts_.size();
      for (size_t i = 0; i < numSlabs; ++i) {
        // Fill with zeros, and set the right starting dimension
        std::vector<hsize_t> hstart;
        if (sel.extent().empty()) {
          hstart.resize((size_t)dims.dimensionality, 0);
        } else {
          hstart.resize((size_t)sel.extent().size(), 0);
        }
        hstart[s.dimension_] = s.dimension_indices_starts_[i];

        // Fill with the total size, and then set the right extent
        std::vector<hsize_t> hcount;
        if (sel.extent().empty()) {
          hcount = convertToH5Length<hsize_t>(dims.dimsCur);
        } else {
          hcount = convertToH5Length<hsize_t>(sel.extent());
        }
        hcount[s.dimension_]
          = (i < s.dimension_indices_counts_.size()) ? s.dimension_indices_counts_[i] : 1;

        if (H5Sselect_hyperslab(cloned_space(), op_map.at(SelectionOperator::OR), hstart.data(),
                                NULL, hcount.data(), NULL)
            < 0)
          throw Exception("Sub-space selection failed.", ioda_Here());
      }

      // Once we have looped through then we apply the actual selection operator to our
      // compound data. If on the first action, space will be either an ALL or NONE
      // selection type, which will cause the H5Smodify_select to fail. H5Smodify_select
      // wants both spaces to be HYPERSLAB spaces. So, on the first action, do a select copy
      // instead of a select modify.
      if (first_action) {
        if (H5Sselect_copy(spc(), cloned_space()) < 0)
          throw Exception("Space copy selection failed", ioda_Here());
      } else {
        if (H5Smodify_select(spc(), op_map.at(s.op_), cloned_space()) < 0)
          throw Exception("Space modify selection failed", ioda_Here());
      }
#else
      throw Exception(
                  "The HDF5 engine needs to be backed by at least "
                  "HDF5 1.12.0 to do the requested selection properly. Older HDF5 versions "
                  "do not have the H5Smodify_select function.", ioda_Here());
#endif
    } else {  // Hyperslab selection

      const auto hstart  = convertToH5Length<hsize_t>(s.start_);
      const auto hstride = convertToH5Length<hsize_t>(s.stride_);
      const auto hcount  = convertToH5Length<hsize_t>(s.count_);
      const auto hblock  = convertToH5Length<hsize_t>(s.block_);

      chk = H5Sselect_hyperslab(spc(), op_map.at(s.op_), hstart.data(),
                                (s.stride_.size()) ? hstride.data() : NULL, hcount.data(),
                                (s.block_.size()) ? hblock.data() : NULL);
    }
    if (chk < 0) throw Exception("Space selection failed.", ioda_Here());
    first_action = false;  // NOLINT: Compilers inconsistently complain about use/unuse of
                           // first_action. Not our bug.
  }

  if (!sel.getOffset().empty()) {
    if (H5Soffset_simple(spc(), convertToH5Length<hssize_t>(sel.getOffset()).data()) < 0)
      throw Exception("Problem applying offset to space.", ioda_Here());
  }

  auto res = std::make_shared<HH_Selection>();
  res->sel = spc;
  sel.concretize(res);
  return spc;
}

Variable HH_Variable::write(gsl::span<const char> data, const Type& in_memory_dataType,
                      const Selection& mem_selection, const Selection& file_selection) {
  // last arg set to false means we are not using parallel IO
  return writeImpl(data, in_memory_dataType, mem_selection, file_selection, false);
}
Variable HH_Variable::parallelWrite(gsl::span<const char> data, const Type& in_memory_dataType,
                      const Selection& mem_selection, const Selection& file_selection) {
  // last arg set to true means we are using parallel IO
  return writeImpl(data, in_memory_dataType, mem_selection, file_selection, true);
}
Variable HH_Variable::writeImpl(gsl::span<const char> data, const Type& in_memory_dataType,
                      const Selection& mem_selection, const Selection& file_selection,
                      const bool isParallelIo) {
  auto memTypeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
  auto memSpace    = getSpaceWithSelection(mem_selection);
  auto fileSpace   = getSpaceWithSelection(file_selection);

  H5T_class_t memTypeClass = H5Tget_class(memTypeBackend->handle());
  HH_hid_t varType(H5Dget_type(var_()), Handles::Closers::CloseHDF5Datatype::CloseP);
  H5T_class_t varTypeClass = H5Tget_class(varType());

  // Create a data transfer property list to be used in all of the following H5Dwrite
  // commands. If running in parallel io mode, we will use the collective style of
  // writing.
  hid_t plist_id = H5Pcreate(H5P_DATASET_XFER);
  if (plist_id < 0) throw Exception("H5Pcreate failed", ioda_Here());
  if (isParallelIo) {
    herr_t rc = H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
    if (rc < 0) throw Exception("H5Pset_fapl_mpio failed", ioda_Here());
  }
  HH_hid_t xfer_plist(plist_id, Handles::Closers::CloseHDF5PropertyList::CloseP);

  if ((memTypeClass == H5T_STRING) && (varTypeClass == H5T_STRING)) {
    // Both memory and file types are strings. Need to check fixed vs variable length.
    // Variable-length strings (default) are packed as an array of pointers.
    // Fixed-length strings are just a sequence of characters.
    htri_t isMemStrVar = H5Tis_variable_str(memTypeBackend->handle());
    htri_t isVarStrVar = H5Tis_variable_str(varType());
    if (isMemStrVar < 0)
      throw Exception("H5Tis_variable_str failed on memory data type.", ioda_Here());
    if (isVarStrVar < 0)
      throw Exception("H5Tis_variable_str failed on backend (file) variable data type.", ioda_Here());
    
    if ((isMemStrVar && isVarStrVar) || (!isMemStrVar && !isVarStrVar)) {
      // No need to change anything. Pass through.
      // NOTE: Using varType instead of memTypeBackend->handle! This is because strings can have
      //   different character sets (ASCII vs UTF-8), which is entirely unhandled in IODA.
      if (H5Dwrite(var_(), varType(), memSpace(), fileSpace(), xfer_plist(), data.data()) < 0)
        throw Exception("H5Dwrite failed.", ioda_Here());
    }
    else if (isMemStrVar) {
      // Variable-length in memory. Fixed-length in var.
      size_t strLen = H5Tget_size(varType());
      size_t numStrs = getDimensions().numElements;

      std::vector<char> out_buf = convertVariableLengthToFixedLength(data, strLen, false);

      if (H5Dwrite(var_(), varType(), memSpace(), fileSpace(), xfer_plist(), out_buf.data()) < 0)
        throw Exception("H5Dwrite failed.", ioda_Here());
    }
    else if (isVarStrVar) {
      // Fixed-length in memory. Variable-length in file.
      // Rare conversion. Included for completeness.

      size_t strLen      = H5Tget_size(memTypeBackend->handle());
      size_t numStrs     = getDimensions().numElements;
      size_t totalStrLen = strLen * numStrs;

      auto converted_data_holder = convertFixedLengthToVariableLength(data, strLen);
      char* converted_data = reinterpret_cast<char*>(converted_data_holder.DataPointers.data());

      if (H5Dwrite(var_(), varType(), memSpace(), fileSpace(), xfer_plist(), converted_data) < 0)
        throw Exception("H5Dwrite failed.", ioda_Here());
    }

  } else {
    // Pass-through case
    auto ret = H5Dwrite(var_(),                   // dataset id
                        memTypeBackend->handle(), // mem_type_id
                        memSpace(),               // mem_space_id
                        fileSpace(),              // file_space_id
                        xfer_plist(),             // xfer_plist_id
                        data.data()               // data
    );
    if (ret < 0) throw Exception("H5Dwrite failure.", ioda_Here());
  }
  return Variable{shared_from_this()};
}

Variable HH_Variable::read(gsl::span<char> data, const Type& in_memory_dataType,
                           const Selection& mem_selection, const Selection& file_selection) const
{
  auto memTypeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
  auto memSpace    = getSpaceWithSelection(mem_selection);
  auto fileSpace   = getSpaceWithSelection(file_selection);

  H5T_class_t memTypeClass = H5Tget_class(memTypeBackend->handle());
  HH_hid_t varType(H5Dget_type(var_()), Handles::Closers::CloseHDF5Datatype::CloseP);
  H5T_class_t varTypeClass = H5Tget_class(varType());

  if ((memTypeClass == H5T_STRING) && (varTypeClass == H5T_STRING)) {
    // Both memory and file types are strings. Need to check fixed vs variable length.
    // Variable-length strings (default) are packed as an array of pointers.
    // Fixed-length strings are just a sequence of characters.
    htri_t isMemStrVar = H5Tis_variable_str(memTypeBackend->handle());
    htri_t isVarStrVar = H5Tis_variable_str(varType());
    if (isMemStrVar < 0)
      throw Exception("H5Tis_variable_str failed on memory data type.", ioda_Here());
    if (isVarStrVar < 0)
      throw Exception("H5Tis_variable_str failed on backend (file) variable data type.", ioda_Here());
    
    if ((isMemStrVar && isVarStrVar) || (!isMemStrVar && !isVarStrVar)) {
      // No need to change anything. Pass through.
      // NOTE: Using varType instead of memTypeBackend->handle! This is because strings can have
      //   different character sets (ASCII vs UTF-8), which is entirely unhandled in IODA.
      if (H5Dread(var_(), varType(), memSpace(), fileSpace(), H5P_DEFAULT, data.data()) < 0)
        throw Exception("H5Dread failed.", ioda_Here());
    }
    else if (isMemStrVar) {
      // Variable-length in memory. Fixed-length in file.

      size_t strLen = H5Tget_size(varType());
      size_t numStrs = getDimensions().numElements;
      std::vector<char> in_buf(numStrs * strLen);

      if (H5Dread(var_(), varType(), memSpace(), fileSpace(), H5P_DEFAULT, in_buf.data()) < 0)
        throw Exception("H5Dread failed.", ioda_Here());

      // This block of code is a bit of a kludge in that we are switching from a packed
      // structure of strings to a packed structure of pointers of strings.
      // The Marshaller code in ioda/Types/Marshalling.h expects this format.
      // In the future, the string read interface in the frontend should use the Marshalling
      // interface to pass these objects back and forth without excessive data element copies.

      char** reint_buf = reinterpret_cast<char**>(data.data());  // NOLINT: casting
      for (size_t i = 0; i < numStrs; ++i) {
        // The malloced strings will be released in Marshalling.h.
        reint_buf[i] = (char*)malloc(strLen + 1);  // NOLINT: quite a deliberate call to malloc here.
        memset(reint_buf[i], 0, strLen+1);
        memcpy(reint_buf[i], in_buf.data() + (strLen * i), strLen);
      }
    }
    else if (isVarStrVar) {
      // Fixed-length in memory. Variable-length in file.
      // Rare conversion. Included for completeness.

      size_t strLen      = H5Tget_size(memTypeBackend->handle());
      size_t numStrs     = getDimensions().numElements;  // !!!
      size_t totalStrLen = strLen * numStrs;

      std::vector<char> in_buf(numStrs * sizeof(char*));

      if (H5Dread(var_(), varType(), memSpace(), fileSpace(), H5P_DEFAULT, in_buf.data()) < 0)
        throw Exception("H5Dread failed.", ioda_Here());

      // We could avoid using the temporary out_buf and write
      // directly to "data", but there is no strong need to do this, as
      // this is a very rare type conversion. C++ lacks a fixed-length string type!
      std::vector<char> out_buf = convertVariableLengthToFixedLength(in_buf, strLen, false);
      if (out_buf.size() != data.size())
        throw Exception("Unexpected sizes.", ioda_Here())
          .add("data.size()", data.size()).add("out_buf.size()", out_buf.size());
      std::copy(out_buf.begin(), out_buf.end(), data.begin());
    }
  } else {
    // Pass-through case
    auto ret = H5Dread( var_(),                   // dataset id
                        memTypeBackend->handle(), // mem_type_id
                        memSpace(),               // mem_space_id
                        fileSpace(),              // file_space_id
                        H5P_DEFAULT,              // xfer_plist_id
                        data.data()               // data
    );
    if (ret < 0) throw Exception("H5Dread failure.", ioda_Here());
  }

  return Variable{std::make_shared<HH_Variable>(*this)};
}

bool HH_Variable::isA(Type lhs) const {
  auto typeBackend = std::dynamic_pointer_cast<HH_Type>(lhs.getBackend());

  // Override for old-format ioda files:
  // Unfortunately, v0 ioda files have an odd mixture
  // of ascii vs unicode strings, as well as
  // fixed and variable-length strings.
  // We try and fix a few of these issues here.

  // Do we expect a string of any type?
  // Is the object a string of any type?
  // If both are true, then we just return true.
  H5T_class_t cls_lhs = H5Tget_class(typeBackend->handle.get());
  H5T_class_t cls_my  = H5Tget_class(internalType().get());
  if (cls_lhs == H5T_STRING && cls_my == H5T_STRING) return true;

  // Another issue: are the types equivalent but not
  // exactly the same? This happens across platforms. Endianness
  // can change.
  if (cls_lhs != cls_my) return false;
  // Now the data are either both integers or floats.
  // For both, are the size (in bytes) the same?
  size_t backendTypeSize = H5Tget_size(typeBackend->handle.get());
  size_t internalTypeSize = H5Tget_size(internalType().get());
  if (backendTypeSize != internalTypeSize) return false;

  // For integers, there are two cases to check.
  //   1. The integers are both 1 byte which get interpreted as a native char types.
  //      In this case different platforms set their char type to either signed or
  //      unsigned making H5T_NATIVE_CHAR type platform dependent. Because of this,
  //      and for now, we will accept that signed and unsigned chars are equivalent
  //      which boils down to skipping the sign check for 1 byte size.
  //   2. The integers are larger than 1 byte which get interpreted as native integer types.
  //      In this case want to ensure check that both are signed or both are unsigned.
  if ((cls_lhs == H5T_INTEGER) && (backendTypeSize > 1)) {
    if (H5Tget_sign(typeBackend->handle.get()) != H5Tget_sign(internalType().get())) return false;
  }

  // Ignored:
  // Are the precisions the same?
  // Are the offsets the same?
  // Is the padding the same?
  // Basically everything in https://support.hdfgroup.org/HDF5/doc/H5.user/Datatypes.html.

  return true;
  // return backend_.isOfType(typeBackend->handle);
}

bool HH_Variable::isExactlyA(HH_hid_t ttype) const {
  HH_hid_t otype = internalType();
  auto ret       = H5Tequal(ttype(), otype());
  if (ret < 0) throw Exception(ioda_Here());
  return (ret > 0) ? true : false;
}

bool HH_Variable::hasFillValue(HH_hid_t create_plist) {
  H5D_fill_value_t fvstatus;                                            // NOLINT: HDF5 C interface
  if (H5Pfill_value_defined(create_plist.get(), &fvstatus) < 0) throw Exception(ioda_Here());
  // if H5D_FILL_VALUE_UNDEFINED, return false. In all other cases, return true.
  return (fvstatus != H5D_FILL_VALUE_UNDEFINED);
}

bool HH_Variable::hasFillValue() const {
  HH_hid_t create_plist(H5Dget_create_plist(var_()),
                        Handles::Closers::CloseHDF5PropertyList::CloseP);
  return hasFillValue(create_plist);
}

HH_Variable::FillValueData_t HH_Variable::getFillValue(HH_hid_t create_plist) const {
  try {
    HH_Variable::FillValueData_t res;

    H5D_fill_value_t fvstatus;  // NOLINT: HDF5 C interface
    if (H5Pfill_value_defined(create_plist.get(), &fvstatus) < 0) throw Exception(ioda_Here());

    // if H5D_FILL_VALUE_UNDEFINED, false. In all other cases, true.
    res.set_ = (fvstatus != H5D_FILL_VALUE_UNDEFINED);

    // There are two types of fill values that we encounter.
    // Those set to fixed values and those set to a "default" value. Unfortunately,
    // netCDF4-written files set the "default" value but use a default that does
    // not match HDF5, and this causes major problems with file compatability.
    // So, we have to override the behavior here to return the fill values expected by
    // either NetCDF4 or HDF5.

    // Query the containing HH_Has_Variables to get the policy.
    auto fvp = container_.lock()->getFillValuePolicy();
    if ((fvstatus == H5D_FILL_VALUE_DEFAULT) && (fvp == FillValuePolicy::NETCDF4)) {
      // H5D_FILL_VALUE_DEFAULT with NETCDF4 fill value policy
      //
      // This is ugly but necessary since we can't use template magic at this level in this
      // direction. Catchall is to assign a zero which is the typical HDF5 default value.
      if (isA<std::string>())
        assignFillValue<std::string>(res, FillValuePolicies::netCDF4_default<std::string>());
      else if (isA<signed char>())
        assignFillValue<signed char>(res, FillValuePolicies::netCDF4_default<signed char>());
      else if (isA<char>())
        assignFillValue<char>(res, FillValuePolicies::netCDF4_default<char>());
      else if (isA<int16_t>())
        assignFillValue<int16_t>(res, FillValuePolicies::netCDF4_default<int16_t>());
      else if (isA<int32_t>())
        assignFillValue<int32_t>(res, FillValuePolicies::netCDF4_default<int32_t>());
      else if (isA<float>())
        assignFillValue<float>(res, FillValuePolicies::netCDF4_default<float>());
      else if (isA<double>())
        assignFillValue<double>(res, FillValuePolicies::netCDF4_default<double>());
      else if (isA<unsigned char>())
        assignFillValue<unsigned char>(res, FillValuePolicies::netCDF4_default<unsigned char>());
      else if (isA<uint16_t>())
        assignFillValue<uint16_t>(res, FillValuePolicies::netCDF4_default<uint16_t>());
      else if (isA<uint32_t>())
        assignFillValue<uint32_t>(res, FillValuePolicies::netCDF4_default<uint32_t>());
      else if (isA<int64_t>())
        assignFillValue<int64_t>(res, FillValuePolicies::netCDF4_default<int64_t>());
      else if (isA<uint64_t>())
        assignFillValue<uint64_t>(res, FillValuePolicies::netCDF4_default<uint64_t>());
      else
        assignFillValue<uint64_t>(res, 0);
    } else {
      // H5D_FILL_VALUE_DEFAULT with HDF5 fill value policy
      // H5D_FILL_VALUE_USER_DEFINED regardless of fill value policy
      auto hType = internalType();  // Get type as
      if (!hType.isValid()) throw Exception(ioda_Here());

      H5T_class_t cls = H5Tget_class(hType());  // NOLINT: HDF5 C interface
      // Check types for support in this function
      const std::set<H5T_class_t> supported{H5T_INTEGER, H5T_FLOAT, H5T_STRING};
      // Unsupported for now: H5T_BITFIELD, H5T_OPAQUE, H5T_COMPOUND,
      // H5T_REFERENCE, H5T_ENUM, H5T_VLEN, H5T_ARRAY.
      if (!supported.count(cls))
        throw Exception(
          "HH's getFillValue function only supports "
          "basic numeric and string data types. Any other types "
          "will require enhancement to FillValueData_t::FillValueUnion_t.",
          ioda_Here());

      size_t szType_inBytes = H5Tget_size(hType());

      // Basic types and string pointers fit in the union. Fixed-length string
      // types do not, which is why we create a special buffer to accommodate.
      std::vector<char> fvbuf(szType_inBytes, 0);
      if (H5Pget_fill_value(create_plist.get(), hType(),
        reinterpret_cast<void*>(fvbuf.data())) < 0)
        throw Exception(ioda_Here());

      // When recovering the fill value, we need to distinguish between
      // strings and the other types.

      // We do this by checking the type.

      if (cls == H5T_STRING) {
        // Need to distinguish between variable-length and fixed-width data types.
        htri_t str_type = H5Tis_variable_str(hType());
        if (str_type < 0) throw Exception(ioda_Here());
        if (str_type > 0) {
          // Variable-length string
          const char** ccp = (const char**)fvbuf.data();  // NOLINT: Casting with HDF5
          // A fill value for a string should always be at the zero element.
          // It makes no sense to have a multidimensional fill.
          if (ccp[0]) {
            res.stringFillValue_ = std::string(ccp[0]);
            // Do proper deallocation of the HDF5-returned string array.
            if (H5free_memory(const_cast<void*>(reinterpret_cast<const void*>(ccp[0]))) < 0)
              throw Exception(ioda_Here());
          }
        } else {
          // Fixed-length string
          res.stringFillValue_ = std::string(fvbuf.data(), fvbuf.size());
        }
      } else {
        if (szType_inBytes > sizeof(res.fillValue_))
          throw Exception(
            "The fill value in HDF5 is too large for the "
            "fillValue_ union. ioda-engines currently only supports fill "
            "values on fundamental types and strings.",
            ioda_Here())
            .add("szType_inBytes", szType_inBytes)
            .add("sizeof(res.fillValue_)", sizeof(res.fillValue_));
        // Copy the buffer to the fvdata object
        memcpy(&(res.fillValue_.ui64), fvbuf.data(),
               fvbuf.size());  // NOLINT: Accessing this union member deliberately.
      }
    }
    return res;
  } catch (...) {
    std::throw_with_nested(Exception("Caught an exception.", ioda_Here()));
  }
}

HH_Variable::FillValueData_t HH_Variable::getFillValue() const {
  HH_hid_t create_plist(H5Dget_create_plist(var_()),
                        Handles::Closers::CloseHDF5PropertyList::CloseP);
  return getFillValue(create_plist);
}

std::vector<Dimensions_t> HH_Variable::getChunkSizes(HH_hid_t create_plist, const Dimensions &dims) {
  H5D_layout_t layout = H5Pget_layout(create_plist.get());
  if (layout == H5D_CHUNKED) {
    int max_ndims = gsl::narrow<int>(dims.dimensionality);
    std::vector<hsize_t> chunks(max_ndims);
    if (H5Pget_chunk(create_plist.get(), max_ndims, chunks.data()) < 0)
      throw Exception(ioda_Here());
    std::vector<Dimensions_t> res;
    res.reserve(chunks.size());
    for (const auto& i : chunks)
      res.emplace_back(gsl::narrow<Dimensions_t>(
        i));  // NOLINT: 'res' has space reserved. No need to rewrite the loop.
    return res;
  }
  return {};
}

std::vector<Dimensions_t> HH_Variable::getChunkSizes() const {
  HH_hid_t create_plist(H5Dget_create_plist(var_()),
                        Handles::Closers::CloseHDF5PropertyList::CloseP);
  return getChunkSizes(create_plist, getDimensions());
}

std::pair<bool, int> HH_Variable::getGZIPCompression(HH_hid_t create_plist) {
  int nfilters = H5Pget_nfilters(create_plist.get());
  if (nfilters < 0) throw Exception(ioda_Here());

  for (unsigned i = 0; i < (unsigned)nfilters; ++i) {
    // See https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-GetFilter2 for the function
    // signature.

    unsigned flags              = 0;    // Unused.
    const size_t cd_nelems_init = 16;   // Size of array
    size_t cd_nelems = cd_nelems_init;  // Pass size of array to function. Overwritten with number
                                        // of values actually read.
    std::vector<unsigned> cd_values(cd_nelems_init);  // Data for filter.
    const size_t namelen = 32;                        // Unused
    std::vector<char> name(namelen);                  // Unused
    unsigned filter_config = 0;                       // Unused

    H5Z_filter_t filt = H5Pget_filter2(create_plist.get(), i, &flags, &cd_nelems, cd_values.data(),
                                       namelen, name.data(), &filter_config);
    if (filt != H5Z_FILTER_DEFLATE) continue;

    if (!cd_nelems) throw Exception(ioda_Here());

    return std::pair<bool, int>(true, gsl::narrow<int>(cd_values[0]));
  }

  // Fallthrough. No GZIP compression was specified.
  return std::pair<bool, int>(false, 0);
}

std::pair<bool, int> HH_Variable::getGZIPCompression() const {
  HH_hid_t create_plist(H5Dget_create_plist(var_()),
                        Handles::Closers::CloseHDF5PropertyList::CloseP);
  return getGZIPCompression(create_plist);
}

std::tuple<bool, unsigned, unsigned> HH_Variable::getSZIPCompression(HH_hid_t create_plist) {
  int nfilters = H5Pget_nfilters(create_plist.get());
  if (nfilters < 0) throw Exception(ioda_Here());

  for (unsigned i = 0; i < (unsigned)nfilters; ++i) {
    // See https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-GetFilter2 for the function
    // signature.

    unsigned flags              = 0;    // Unused.
    const size_t cd_nelems_init = 16;   // Size of array
    size_t cd_nelems = cd_nelems_init;  // Pass size of array to function. Overwritten with number
                                        // of values actually read.
    std::vector<unsigned> cd_values(cd_nelems_init);  // Data for filter.
    const size_t namelen = 32;                        // Unused
    std::vector<char> name(namelen);                  // Unused
    unsigned filter_config = 0;                       // Unused

    H5Z_filter_t filt = H5Pget_filter2(create_plist.get(), i, &flags, &cd_nelems, cd_values.data(),
                                       namelen, name.data(), &filter_config);
    if (filt != H5Z_FILTER_SZIP) continue;

    if (cd_nelems < 2) throw Exception(ioda_Here());

    // cd_nelems is actually 4, but the options do not match the H5Pset_szip flags!
    return std::tuple<bool, unsigned, unsigned>(true, cd_values[0], cd_values[1]);
  }

  // Fallthrough. No SZIP compression was specified.
  return std::tuple<bool, unsigned, unsigned>(false, 0, 0);
}

std::tuple<bool, unsigned, unsigned> HH_Variable::getSZIPCompression() const {
  HH_hid_t create_plist(H5Dget_create_plist(var_()),
                        Handles::Closers::CloseHDF5PropertyList::CloseP);
  return getSZIPCompression(create_plist);
}

VariableCreationParameters HH_Variable::getCreationParameters(bool doAtts, bool doDims) const {
  HH_hid_t create_plist(H5Dget_create_plist(var_()),
                        Handles::Closers::CloseHDF5PropertyList::CloseP);
  VariableCreationParameters res;

  // Get chunking
  auto chunkinfo = getChunkSizes(create_plist, getDimensions());
  if (chunkinfo.size()) {
    res.chunk  = true;
    res.chunks = chunkinfo;
  }
  // Get compression
  auto gz = getGZIPCompression(create_plist);
  if (gz.first) res.compressWithGZIP(gz.second);
  auto sz = getSZIPCompression(create_plist);
  if (std::get<0>(sz)) res.compressWithSZIP(std::get<1>(sz), std::get<2>(sz));
  // Get fill value
  res.fillValue_ = getFillValue(create_plist);
  // Attributes (optional)
  if (doAtts) {
    throw Exception("Unimplemented doAtts", ioda_Here());
  }
  // Dimensions (optional)
  if (doDims) {
    throw Exception("Unimplemented doDims", ioda_Here());
  }

  return res;
}

HH_Selection::~HH_Selection() = default;

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
