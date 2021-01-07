/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/HH/HH-variables.h"

#include <algorithm>
#include <numeric>
#include <set>

#include "ioda/Engines/HH/HH-attributes.h"
#include "ioda/Engines/HH/HH-types.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/Dimensions.h"

namespace ioda {
namespace detail {
/// Internal namespace.
namespace HH {
template <class T>
std::vector<T> convertToH5Length(const std::vector<Dimensions_t>& in) {
  std::vector<T> res(in.size());
  for (size_t i = 0; i < in.size(); ++i) res[i] = gsl::narrow<T>(in[i]);
  return res;
}
}  // namespace HH

namespace Engines {
namespace HH {
HH_Variable_Backend::HH_Variable_Backend() = default;
HH_Variable_Backend::~HH_Variable_Backend() = default;
HH_Variable_Backend::HH_Variable_Backend(::HH::Dataset d,
                                         std::shared_ptr<const HH_HasVariables_Backend> container)
    : backend_(d), container_(container) {
  atts = Has_Attributes(std::make_shared<HH_HasAttributes_Backend>(backend_.atts));
}

detail::Type_Provider* HH_Variable_Backend::getTypeProvider() const { return HH_Type_Provider::instance(); }

Dimensions HH_Variable_Backend::getDimensions() const {
  auto back_dims = backend_.getDimensions();
  Dimensions ret;
  ret.numElements = gsl::narrow<decltype(Dimensions::numElements)>(back_dims.numElements);
  ret.dimensionality = gsl::narrow<decltype(Dimensions::dimensionality)>(back_dims.dimensionality);
  for (const auto& d : back_dims.dimsCur) ret.dimsCur.push_back(gsl::narrow<Dimensions_t>(d));
  for (const auto& d : back_dims.dimsMax)
    ret.dimsMax.push_back((d == H5S_UNLIMITED) ? ioda::Unlimited : gsl::narrow<hsize_t>(d));

  return ret;
}

Variable HH_Variable_Backend::resize(const std::vector<Dimensions_t>& newDims) {
  std::vector<hsize_t> hdims = detail::HH::convertToH5Length<hsize_t>(newDims);

  if (H5Dset_extent(backend_.get()(), hdims.data()) < 0)
    throw;  // jedi_throw.add("Reason", "Failure to resize a Variable with the HDF5 backend.");

  return Variable{shared_from_this()};
}

Variable HH_Variable_Backend::attachDimensionScale(unsigned int DimensionNumber, const Variable& scale) {
  try {
    // We are extracting the backend object.
    auto scaleBackendBase = scale.get();
    // If the backend object is an HH object, then we can attach this scale.
    // Otherwise, throw an error because you can't mix Variables from different
    // backends.
    auto scaleBackendDerived = std::dynamic_pointer_cast<HH_Variable_Backend>(scaleBackendBase);

    backend_.attachDimensionScale(DimensionNumber, scaleBackendDerived->backend_);

    return Variable{shared_from_this()};
  } catch (const std::bad_cast&) {
    throw;  // For now, rethrow.
  }
}
Variable HH_Variable_Backend::detachDimensionScale(unsigned int DimensionNumber, const Variable& scale) {
  try {
    // We are extracting the backend object.
    auto scaleBackendBase = scale.get();
    // If the backend object is an HH object, then we can attach this scale.
    // Otherwise, throw an error because you can't mix Variables from different
    // backends.
    auto scaleBackendDerived = std::dynamic_pointer_cast<HH_Variable_Backend>(scaleBackendBase);

    backend_.detachDimensionScale(DimensionNumber, scaleBackendDerived->backend_);

    return Variable{shared_from_this()};
  } catch (const std::bad_cast&) {
    throw;  // For now, rethrow.
  }
}
bool HH_Variable_Backend::isDimensionScale() const { return backend_.isDimensionScale(); }
Variable HH_Variable_Backend::setIsDimensionScale(const std::string& dimensionScaleName) {
  backend_.setIsDimensionScale(dimensionScaleName);
  return Variable{shared_from_this()};
}
Variable HH_Variable_Backend::getDimensionScaleName(std::string& res) const {
  backend_.getDimensionScaleName(res);
  return Variable{std::make_shared<HH_Variable_Backend>(*this)};
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
std::vector<std::vector<std::pair<std::string, Variable>>> HH_Variable_Backend::getDimensionScaleMappings(
  const std::vector<std::pair<std::string, Variable>>& scalesToQueryAgainst, bool firstOnly,
  const std::vector<unsigned>& dimensionNumbers_) const {
  try {
    // Extract all of the scalesToQueryAgainst and convert them into the appropriate backend objects.
    // If the backend objects are not from this engine, then this is an error (no mixing Variables
    // and scales across different backends).
    std::vector<std::pair<std::string, std::shared_ptr<HH_Variable_Backend>>> scales;
    for (const auto& scale : scalesToQueryAgainst) {
      auto scaleBackendBase = scale.second.get();
      auto scaleBackendDerived = std::dynamic_pointer_cast<HH_Variable_Backend>(scaleBackendBase);
      scales.push_back(
        {scale.first, scaleBackendDerived});  // NOLINT: macos oddly is missing emplace_back here.
    }

    // The logic here roughly follows H5DSis_attached, but with extra optimizations and added
    // loops to avoid variable repeated variable {open and close} operations.

    // Check that the dimensionality is sufficient to have a
    // DimensionNumber element.
    auto datadims = getDimensions();
    std::vector<unsigned> dimensionNumbers = dimensionNumbers_;
    if (!dimensionNumbers.empty()) {
      auto max_elem_it = std::max_element(dimensionNumbers.cbegin(), dimensionNumbers.cend());
      if (max_elem_it == dimensionNumbers.cend()) throw;   // jedi_throw;
      if (datadims.dimensionality <= *max_elem_it) throw;  // jedi_throw;
    } else {
      dimensionNumbers.resize(datadims.dimensionality);
      std::iota(dimensionNumbers.begin(), dimensionNumbers.end(), 0);
    }

    // The return value. Give it the correct size (the dimensionality of the variable).
    std::vector<std::vector<std::pair<std::string, Variable>>> ret(
      gsl::narrow<size_t>(datadims.dimensionality));

    // Attempt to read the reference list for our variable's
    // DIMENSION_LIST attribute at index DimensionNumber.
    if (!backend_.atts.exists("DIMENSION_LIST"))
      return ret;  // Fallthrough returning a vector of empty vectors.
    ::HH::Attribute aDims = backend_.atts.open("DIMENSION_LIST");

    auto vltyp = aDims.getType();
    auto vldims = aDims.getDimensions();

    // Internal structure to encapsulate resources and prevent leaks.
    // This is because H5Aread is returning a variable-length array
    // data structure that must be reclaimed. This is also why we don't
    // just use a std::vector.
    struct Vlen_data {
      std::unique_ptr<hvl_t[]> buf;        // NOLINT: C array and visibility warnings.
      ::HH::Handles::HH_hid_t typ, space;  // NOLINT: C visibility warnings.
      Vlen_data(size_t sz, ::HH::Handles::HH_hid_t typ, ::HH::Handles::HH_hid_t space)
          : buf(new hvl_t[sz]), typ{typ}, space{space} {}
      ~Vlen_data() {
        if (buf) H5Dvlen_reclaim(typ.get(), space.get(), H5P_DEFAULT, reinterpret_cast<void*>(buf.get()));
      }
      Vlen_data(const Vlen_data&) = delete;
      Vlen_data(Vlen_data&&) = delete;
      Vlen_data operator=(const Vlen_data&) = delete;
      Vlen_data operator=(Vlen_data&&) = delete;
    };
    Vlen_data buf((size_t)datadims.dimensionality, vltyp, aDims.getSpace());

    if (H5Aread(aDims.get()(), vltyp.get(), reinterpret_cast<void*>(buf.buf.get())) < 0)
      throw;  // jedi_throw;

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
      if (H5Oget_info2(scales[i].second->backend_.get()(), &scale_infos[i], H5O_INFO_BASIC) < 0)
        throw;  // jedi_throw.add("Reason", "Bad HDF5 call");
#else
      if (H5Oget_info(scales[i].second->backend_.get()(), &scale_infos[i]) < 0)
        throw;  // jedi_throw.add("Reason", "Bad HDF5 call");
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
#if H5_VERSION_GE(1, 10, 0)
        hid_t deref_scale_id = H5Rdereference2(
          // First parameter is any object id in the same file
          backend_.get()(), H5P_DEFAULT, H5R_OBJECT, &ref);
#else
	hid_t deref_scale_id = H5Rdereference(
	   backend_.get()(), H5R_OBJECT, &ref);
#endif
        Expects(deref_scale_id >= 0);  // Die on failure. Would have to clean up memory otherwise.
        ::HH::Dataset deref_scale(deref_scale_id);  // Move into managed memory

        // Get deref_scale's info and compare to scale_info.
#if H5_VERSION_GE(1, 10, 3)
        if (H5Oget_info2(deref_scale.get()(), &check_info, H5O_INFO_BASIC) < 0)
          throw;  // jedi_throw.add("Reason", "Bad HDF5 call");
#else
        if (H5Oget_info(deref_scale.get()(), &check_info) < 0)
          throw;  // jedi_throw.add("Reason", "Bad HDF5 call");
#endif

        // Iterate over each scalesToQueryAgainst
        // I.e. for each *scale that the user passed*.
        bool foundScale = false;
        for (size_t j = 0; j < scale_infos.size(); ++j) {
          if ((scale_infos[j].fileno == check_info.fileno) && (scale_infos[j].addr == check_info.addr)) {
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
  } catch (const std::bad_cast&) {
    throw;  // For now, rethrow.
  }
}

bool HH_Variable_Backend::isDimensionScaleAttached(unsigned int DimensionNumber,
                                                   const Variable& scale) const {
  const std::vector<std::pair<std::string, Variable>> scalesToQueryAgainst{{"unused_param", scale}};
  auto res = getDimensionScaleMappings(scalesToQueryAgainst, true, {DimensionNumber});
  return !res[DimensionNumber].empty();
}

std::vector<std::vector<std::pair<std::string, Variable>>> HH_Variable_Backend::getDimensionScaleMappings(
  const std::list<std::pair<std::string, Variable>>& scalesToQueryAgainst, bool firstOnly) const {
  return getDimensionScaleMappings({scalesToQueryAgainst.begin(), scalesToQueryAgainst.end()}, firstOnly, {});
}

::HH::HH_hid_t HH_Variable_Backend::getSpaceWithSelection(const Selection& sel) const {
  if (sel.default_ == SelectionState::ALL)
    if (sel.actions_.empty()) return ::HH::HH_hid_t(H5S_ALL);

  ::HH::HH_hid_t space(H5Scopy(backend_.getSpace()()), ::HH::Handles::Closers::CloseHDF5Dataspace::CloseP);
  if (space() < 0) throw HH_throw.add("Reason", "Cannot copy dataspace.");

  if (!sel.extent_.empty()) {
    if (H5Sset_extent_simple(space(), gsl::narrow<int>(sel.extent_.size()),
                             detail::HH::convertToH5Length<hsize_t>(sel.extent_).data(),
                             detail::HH::convertToH5Length<hsize_t>(sel.extent_).data()) < 0)
      throw HH_throw.add("Reason", "Caanot set dataspace extent.");
  }

  if (sel.default_ == SelectionState::ALL) {
    if (H5Sselect_all(space()) < 0) throw HH_throw.add("Reason", "Dataspace selection failed.");
  } else if (sel.default_ == SelectionState::NONE) {
    if (H5Sselect_none(space()) < 0) throw HH_throw.add("Reason", "Dataspace selection failed.");
  }

  static const std::map<SelectionOperator, H5S_seloper_t> op_map = {
    {SelectionOperator::SET, H5S_SELECT_SET},       {SelectionOperator::OR, H5S_SELECT_OR},
    {SelectionOperator::AND, H5S_SELECT_AND},       {SelectionOperator::XOR, H5S_SELECT_XOR},
    {SelectionOperator::NOT_B, H5S_SELECT_NOTB},    {SelectionOperator::NOT_A, H5S_SELECT_NOTA},
    {SelectionOperator::APPEND, H5S_SELECT_APPEND}, {SelectionOperator::PREPEND, H5S_SELECT_PREPEND}};
  bool first_action = true;
  for (const auto& s : sel.actions_) {
    if (!op_map.count(s.op_)) throw std::logic_error("Unimplemented map value.");
    herr_t chk = 0;
    // Is this a hyperslab or a single point selection?
    if (!s.points_.empty()) {  // Single point selection

      size_t dimensionality = s.points_.at(0).size();

      std::vector<hsize_t> elems(dimensionality * s.points_.size());

      // Waiting for std::ranges in C++20
      for (size_t i = 0; i < s.points_.size(); ++i)  // const auto& p : s.points_)
      {
        if (s.points_[i].size() != dimensionality)
          throw std::logic_error("Points have inconsistent dimensionalities.");
        for (size_t j = 0; j < dimensionality; ++j) elems[j + (dimensionality * i)] = s.points_[i][j];
        // std::copy_n(p.data(), dimensionality, elems.data() + (i * dimensionality));
      }

      chk = H5Sselect_elements(space(), op_map.at(s.op_), s.points_.size(), elems.data());
    } else if (!s.dimension_indices_starts_.empty()) {
      // This is a variant of the hyperslab selection code.
      // We have index ranges, and we then convert to the usual
      // start and count.
      // SelectionOperator is a problem here, because the selections
      // that we are making are not commutative. So, we do a two-part selection.
      //  First, clone the dataspace and select everything.
      //  Then, apply this bulk selection to the actual space.
#if H5_VERSION_GE(1, 12, 0)
      ::HH::HH_hid_t cloned_space(H5Scopy(space()), ::HH::Handles::Closers::CloseHDF5Dataspace::CloseP);
      Expects(H5Sselect_none(cloned_space()) >= 0);

      ioda::Dimensions dims = getDimensions();
      Expects(s.dimension_ < (size_t)dims.dimensionality);
      const size_t numSlabs = s.dimension_indices_starts_.size();
      for (size_t i = 0; i < numSlabs; ++i) {
        // Fill with zeros, and set the right starting dimension
        std::vector<hsize_t> hstart;
        if (sel.extent_.empty()) {
          hstart.resize((size_t)dims.dimensionality, 0);
        } else {
          hstart.resize((size_t)sel.extent_.size(), 0);
        }
        hstart[s.dimension_] = s.dimension_indices_starts_[i];

        // Fill with the total size, and then set the right extent
        std::vector<hsize_t> hcount;
        if (sel.extent_.empty()) {
          hcount = detail::HH::convertToH5Length<hsize_t>(dims.dimsCur);
        } else {
          hcount = detail::HH::convertToH5Length<hsize_t>(sel.extent_);
        }
        hcount[s.dimension_] = (i < s.dimension_indices_counts_.size()) ? s.dimension_indices_counts_[i] : 1;

        if (H5Sselect_hyperslab(cloned_space(), op_map.at(SelectionOperator::OR), hstart.data(), NULL,
                                hcount.data(), NULL) < 0)
          throw HH_throw.add("Reason", "Sub-space selection failed.");
      }

      // Once we have looped through then we apply the actual selection operator to our
      // compound data. If on the first action, space will be either an ALL or NONE
      // selection type, which will cause the H5Smodify_select to fail. H5Smodify_select
      // wants both spaces to be HYPERSLAB spaces. So, on the first action, do a select copy
      // instead of a select modify.
      if (first_action) {
        if (H5Sselect_copy(space(), cloned_space()) < 0)
          throw; // HH_throw.add("Reason", "Space copy selection failed");
      } else {
        if (H5Smodify_select(space(), op_map.at(s.op_), cloned_space()) < 0)
          throw; // HH_throw.add("Reason", "Space modify selection failed");
      }
#else
      //throw jedi_throw.add("Reason",
      //                     "The HDF5 engine needs to be backed by at least "
      //                     "HDF5 1.12.0 to do the requested selection properly. Older HDF5 versions "
      //                     "do not have the H5Smodify_select function.");
#endif
    } else {  // Hyperslab selection

      const auto hstart = detail::HH::convertToH5Length<hsize_t>(s.start_);
      const auto hstride = detail::HH::convertToH5Length<hsize_t>(s.stride_);
      const auto hcount = detail::HH::convertToH5Length<hsize_t>(s.count_);
      const auto hblock = detail::HH::convertToH5Length<hsize_t>(s.block_);

      chk = H5Sselect_hyperslab(space(), op_map.at(s.op_), hstart.data(),
                                (s.stride_.size()) ? hstride.data() : NULL, hcount.data(),
                                (s.block_.size()) ? hblock.data() : NULL);
    }
    if (chk < 0) throw; //HH_throw.add("Reason", "Space selection failed.");
    first_action =
      false;  // NOLINT: Compilers inconsistently complain about use/unuse of first_action. Not our bug.
  }

  if (!sel.offset_.empty()) {
    if (H5Soffset_simple(space(), detail::HH::convertToH5Length<hssize_t>(sel.offset_).data()) < 0)
      throw; // HH_throw.add("Reason", "Problem applying offset to space.");
  }

  return space;
}

Variable HH_Variable_Backend::write(gsl::span<char> data, const Type& in_memory_dataType,
                                    const Selection& mem_selection, const Selection& file_selection) {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
    auto memSpace = getSpaceWithSelection(mem_selection);
    auto fileSpace = getSpaceWithSelection(file_selection);
    backend_.writeDirect(data, typeBackend->handle, memSpace, fileSpace);
    return Variable{shared_from_this()};
  } catch (std::bad_cast&) {
    throw;
  }
}

Variable HH_Variable_Backend::read(gsl::span<char> data, const Type& in_memory_dataType,
                                   const Selection& mem_selection, const Selection& file_selection) const {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
    ::HH::HH_hid_t file_type = typeBackend->handle;

    // Override for old-format ioda files:
    // Unfortunately, v0 ioda files have an odd mixture
    // of ascii vs unicode strings, as well as
    // fixed and variable-length strings.
    // We try and fix a few of these issues here.

    // Is this a string of any type?
    H5T_class_t cls_my = H5Tget_class(backend_.getType().get());
    if (cls_my == H5T_STRING) {
      // We always load strings using the hdf5-provided
      // type. This way, we ignore character set flags
      // and other string properties that do not matter
      // in ioda.
      file_type = backend_.getType();

      // However, fixed and variable-length strings need
      // slightly different read calls.

      if (H5Tis_variable_str(file_type.get()) > 0) {
        // Variable-length string.
        // No special read is needed.
        // Character set is specified correctly by
        // matching file_type above.

        backend_.readDirect(data, file_type, getSpaceWithSelection(mem_selection),
                            getSpaceWithSelection(file_selection));
      } else {
        // Fixed-length string
        // Special read is needed.

        // Figure out the size of the data being read.
        auto space = getSpaceWithSelection(mem_selection);
        // This is hard, as the data space selections can be
        // a space or the identifiers H5S
        auto sz_max = gsl::narrow<hssize_t>(getDimensions().numElements);
        hssize_t sz = sz_max;
        // hssize_t sz_max = H5Sget_simple_extent_npoints(space.get());
        // if (sz_max < 0) throw HH_throw;
        if (space.get() != H5S_ALL) {
          H5S_sel_type st = H5Sget_select_type(space.get());

          if (st == H5S_SEL_NONE) sz = 0;
          if (st == H5S_SEL_HYPERSLABS || st == H5S_SEL_POINTS) sz = H5Sget_select_npoints(space.get());
          if (sz < 0) throw; //HH_throw;
        }

        // This is all of the strings concatenated.
        std::vector<char> tmp_buf(gsl::narrow<size_t>(sz));
        backend_.readDirect(gsl::make_span(tmp_buf), file_type, getSpaceWithSelection(mem_selection),
                            getSpaceWithSelection(file_selection));

        // We "fake" the structure that we fill so that
        // it looks like a variable-length string.

        // We have a set of fixed-length strings that we read
        // as characters. We can turn this into strings.

        // Get the size of the enclosed strings, in bytes.
        // This leaves off the terminating NULL.
        const size_t sz_each_str = H5Tget_size(file_type.get());
        const size_t num_strs = tmp_buf.size() / sz_each_str;

        // Reinterpret the input buffer as a sequence of char*s.
        // Assign each string to the appropriate value.

        char** reint_buf = reinterpret_cast<char**>(data.data());  // NOLINT: casting
        for (size_t i = 0; i < num_strs; ++i) {
          std::string s(tmp_buf.data() + (sz_each_str * i), sz_each_str);
          reint_buf[i] = (char*)malloc(sz_each_str + 1);  // NOLINT: quite a deliberate call to malloc here.
          ioda::detail::COMPAT_strncpy_s(reint_buf[i], sz_each_str + 1, s.data(), s.size() + 1);
        }
      }
    } else {
      backend_.readDirect(data, file_type, getSpaceWithSelection(mem_selection),
                          getSpaceWithSelection(file_selection));
    }

    return Variable{std::make_shared<HH_Variable_Backend>(*this)};
  } catch (std::bad_cast&) {
    throw;
  }
}

bool HH_Variable_Backend::isA(Type lhs) const {
  try {
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
    H5T_class_t cls_my = H5Tget_class(backend_.getType().get());
    if (cls_lhs == H5T_STRING && cls_my == H5T_STRING) return true;

    // Another issue: are the types equivalent but not
    // exactly the same? This happens across platforms. Endianness
    // can change.
    if (cls_lhs != cls_my) return false;
    // Now the data are either both integers or floats.
    // For both, are the size (in bytes) the same?
    if (H5Tget_size(typeBackend->handle.get()) != H5Tget_size(backend_.getType().get())) return false;

    // For integers, are they both signed or unsigned?
    if (cls_lhs == H5T_INTEGER)
      if (H5Tget_sign(typeBackend->handle.get()) != H5Tget_sign(backend_.getType().get())) return false;

    // Ignored:
    // Are the precisions the same?
    // Are the offsets the same?
    // Is the padding the same?
    // Basically everything in https://support.hdfgroup.org/HDF5/doc/H5.user/Datatypes.html.

    return true;
    // return backend_.isOfType(typeBackend->handle);
  } catch (std::bad_cast&) {
    throw;
  }
}

bool HH_Variable_Backend::hasFillValue() const {
  try {
    ::HH::HH_hid_t create_plist(H5Dget_create_plist(backend_.get()()),
                                ::HH::Closers::CloseHDF5PropertyList::CloseP);
    H5D_fill_value_t fvstatus;  // NOLINT: HDF5 C interface
    if (H5Pfill_value_defined(create_plist.get(), &fvstatus) < 0) throw; //HH_throw;
    // if H5D_FILL_VALUE_UNDEFINED, return false. In all other cases, return true.
    return (fvstatus != H5D_FILL_VALUE_UNDEFINED);
  } catch (std::bad_cast&) {
    throw;
  }
}

/// \todo Use memcpy_s where available.
HH_Variable_Backend::FillValueData_t HH_Variable_Backend::getFillValue() const {
  try {
    HH_Variable_Backend::FillValueData_t res;

    ::HH::HH_hid_t create_plist(H5Dget_create_plist(backend_.get()()),
                                ::HH::Closers::CloseHDF5PropertyList::CloseP);
    H5D_fill_value_t fvstatus;  // NOLINT: HDF5 C interface
    if (H5Pfill_value_defined(create_plist.get(), &fvstatus) < 0) throw; //HH_throw;

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
      // This is ugly but necessary since we can't use template magic at this level in this direction.
      // Catchall is to assign a zero which is the typical HDF5 default value.
      if (backend_.isOfType<std::string>())
        assignFillValue<std::string>(res, FillValuePolicies::netCDF4_default<std::string>());
      else if (backend_.isOfType<signed char>())
        assignFillValue<signed char>(res, FillValuePolicies::netCDF4_default<signed char>());
      else if (backend_.isOfType<char>())
        assignFillValue<char>(res, FillValuePolicies::netCDF4_default<char>());
      else if (backend_.isOfType<int16_t>())
        assignFillValue<int16_t>(res, FillValuePolicies::netCDF4_default<int16_t>());
      else if (backend_.isOfType<int32_t>())
        assignFillValue<int32_t>(res, FillValuePolicies::netCDF4_default<int32_t>());
      else if (backend_.isOfType<float>())
        assignFillValue<float>(res, FillValuePolicies::netCDF4_default<float>());
      else if (backend_.isOfType<double>())
        assignFillValue<double>(res, FillValuePolicies::netCDF4_default<double>());
      else if (backend_.isOfType<unsigned char>())
        assignFillValue<unsigned char>(res, FillValuePolicies::netCDF4_default<unsigned char>());
      else if (backend_.isOfType<uint16_t>())
        assignFillValue<uint16_t>(res, FillValuePolicies::netCDF4_default<uint16_t>());
      else if (backend_.isOfType<uint32_t>())
        assignFillValue<uint32_t>(res, FillValuePolicies::netCDF4_default<uint32_t>());
      else if (backend_.isOfType<int64_t>())
        assignFillValue<int64_t>(res, FillValuePolicies::netCDF4_default<int64_t>());
      else if (backend_.isOfType<uint64_t>())
        assignFillValue<uint64_t>(res, FillValuePolicies::netCDF4_default<uint64_t>());
      else
        assignFillValue<uint64_t>(res, 0);
    } else {
      // H5D_FILL_VALUE_DEFAULT with HDF5 fill value policy
      // H5D_FILL_VALUE_USER_DEFINED regardless of fill value policy
      auto type = backend_.getType();
      if (!type.isValid()) throw; //HH_throw;

      H5T_class_t cls = H5Tget_class(type());  // NOLINT: HDF5 C interface
      // Check types for support in this function
      const std::set<H5T_class_t> supported{H5T_INTEGER, H5T_FLOAT, H5T_STRING};
      // Unsupported for now: H5T_BITFIELD, H5T_OPAQUE, H5T_COMPOUND,
      // H5T_REFERENCE, H5T_ENUM, H5T_VLEN, H5T_ARRAY.
      if (!supported.count(cls))
        throw; /* jedi_throw
.add("Reason", "HH's getFillValue function only supports "
"basic numeric and string data types. Any other types "
"will require enhancement to FillValueData_t::FillValueUnion_t.");*/

      size_t szType_inBytes = H5Tget_size(type());

      // Basic types and string pointers fit in the union. Fixed-length string
      // types do not, which is why we create a special buffer to accommodate.
      std::vector<char> fvbuf(szType_inBytes, 0);
      if (H5Pget_fill_value(create_plist.get(), type(), reinterpret_cast<void*>(fvbuf.data())) < 0)
        throw; //HH_throw;

      // When recovering the fill value, we need to distinguish between
      // strings and the other types.

      // We do this by checking the type.

      if (cls == H5T_STRING) {
        // Need to distinguish between variable-length and fixed-width data types.
        htri_t str_type = H5Tis_variable_str(type());
        if (str_type < 0) throw; //HH_throw;
        if (str_type > 0) {
          // Variable-length string
          const char** ccp = (const char**)fvbuf.data();  // NOLINT: Casting with HDF5
          // A fill value for a string should always be at the zero element.
          // It makes no sense to have a multidimensional fill.
          res.stringFillValue_ = std::string(ccp[0]);
          // Do proper deallocation of the HDF5-returned string array.
          if (H5free_memory(const_cast<void*>(reinterpret_cast<const void*>(ccp[0]))) < 0) throw; //HH_throw;
        } else {
          // Fixed-length string
          res.stringFillValue_ = std::string(fvbuf.data(), fvbuf.size());
        }
      } else {
        if (szType_inBytes > sizeof(res.fillValue_))
          throw; /* jedi_throw
.add("Reason", "The fill value in HDF5 is too large for the "
"fillValue_ union. ioda-engines currently only supports fill "
"values on fundamental types and strings.")
.add("szType_inBytes", szType_inBytes)
.add("sizeof(res.fillValue_)", sizeof(res.fillValue_)); */
        // Copy the buffer to the fvdata object
        // TODO(ryan): Use memcpy_s when available.
        memcpy(&(res.fillValue_.ui64), fvbuf.data(),
               fvbuf.size());  // NOLINT: Accessing this union member deliberately.
      }
    }
    return res;
  } catch (std::bad_cast&) {
    throw;
  }
}

std::vector<Dimensions_t> HH_Variable_Backend::getChunkSizes() const {
  ::HH::HH_hid_t create_plist(H5Dget_create_plist(backend_.get()()),
                              ::HH::Closers::CloseHDF5PropertyList::CloseP);

  H5D_layout_t layout = H5Pget_layout(create_plist.get());
  if (layout == H5D_CHUNKED) {
    int max_ndims = gsl::narrow<int>(getDimensions().dimensionality);
    std::vector<hsize_t> chunks(max_ndims);
    if (H5Pget_chunk(create_plist.get(), max_ndims, chunks.data()) < 0) throw; //HH_throw;
    std::vector<Dimensions_t> res;
    res.reserve(chunks.size());
    for (const auto& i : chunks)
      res.emplace_back(
        gsl::narrow<Dimensions_t>(i));  // NOLINT: 'res' has space reserved. No need to rewrite the loop.
    return res;
  }
  return {};
}

std::pair<bool, int> HH_Variable_Backend::getGZIPCompression() const {
  ::HH::HH_hid_t create_plist(H5Dget_create_plist(backend_.get()()),
                              ::HH::Closers::CloseHDF5PropertyList::CloseP);

  int nfilters = H5Pget_nfilters(create_plist.get());
  if (nfilters < 0) throw; //HH_throw;

  for (unsigned i = 0; i < (unsigned)nfilters; ++i) {
    // See https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-GetFilter2 for the function
    // signature.

    unsigned flags = 0;                // Unused.
    const size_t cd_nelems_init = 16;  // Size of array
    size_t cd_nelems =
      cd_nelems_init;  // Pass size of array to function. Overwritten with number of values actually read.
    std::vector<unsigned> cd_values(cd_nelems_init);  // Data for filter.
    const size_t namelen = 32;                        // Unused
    std::vector<char> name(namelen);                  // Unused
    unsigned filter_config = 0;                       // Unused

    H5Z_filter_t filt = H5Pget_filter2(create_plist.get(), i, &flags, &cd_nelems, cd_values.data(), namelen,
                                       name.data(), &filter_config);
    if (filt != H5Z_FILTER_DEFLATE) continue;

    if (!cd_nelems) throw; //HH_throw.add("Reason", "Bad deflate filter return options.");

    return std::pair<bool, int>(true, gsl::narrow<int>(cd_values[0]));
  }

  // Fallthrough. No GZIP compression was specified.
  return std::pair<bool, int>(false, 0);
}

std::tuple<bool, unsigned, unsigned> HH_Variable_Backend::getSZIPCompression() const {
  ::HH::HH_hid_t create_plist(H5Dget_create_plist(backend_.get()()),
                              ::HH::Closers::CloseHDF5PropertyList::CloseP);

  int nfilters = H5Pget_nfilters(create_plist.get());
  if (nfilters < 0) throw; //HH_throw;

  for (unsigned i = 0; i < (unsigned)nfilters; ++i) {
    // See https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-GetFilter2 for the function
    // signature.

    unsigned flags = 0;                // Unused.
    const size_t cd_nelems_init = 16;  // Size of array
    size_t cd_nelems =
      cd_nelems_init;  // Pass size of array to function. Overwritten with number of values actually read.
    std::vector<unsigned> cd_values(cd_nelems_init);  // Data for filter.
    const size_t namelen = 32;                        // Unused
    std::vector<char> name(namelen);                  // Unused
    unsigned filter_config = 0;                       // Unused

    H5Z_filter_t filt = H5Pget_filter2(create_plist.get(), i, &flags, &cd_nelems, cd_values.data(), namelen,
                                       name.data(), &filter_config);
    if (filt != H5Z_FILTER_SZIP) continue;

    if (cd_nelems < 2) throw; //HH_throw.add("Reason", "Bad szip filter return options.");

    // TODO(ryan): cd_nelems is actually 4, but the options do not match the H5Pset_szip flags!
    return std::tuple<bool, unsigned, unsigned>(true, cd_values[0], cd_values[1]);
  }

  // Fallthrough. No SZIP compression was specified.
  return std::tuple<bool, unsigned, unsigned>(false, 0, 0);
}

HH_HasVariables_Backend::~HH_HasVariables_Backend() = default;
HH_HasVariables_Backend::HH_HasVariables_Backend() : backend_(::HH::Handles::HH_hid_t::dummy()) {}
HH_HasVariables_Backend::HH_HasVariables_Backend(::HH::Has_Datasets dsets, ::HH::File fileroot)
    : backend_(dsets), fileroot_(fileroot) {}

detail::Type_Provider* HH_HasVariables_Backend::getTypeProvider() const {
  return HH_Type_Provider::instance();
}

bool HH_HasVariables_Backend::exists(const std::string& name) const { return backend_.exists(name); }
void HH_HasVariables_Backend::remove(const std::string& name) { backend_.remove(name); }
Variable HH_HasVariables_Backend::open(const std::string& name) const {
  auto res = backend_.open(name);
  auto b = std::make_shared<HH_Variable_Backend>(res, shared_from_this());
  Variable var{b};
  return var;
}
std::vector<std::string> HH_HasVariables_Backend::list() const { return backend_.list(); }
Variable HH_HasVariables_Backend::create(const std::string& name, const Type& in_memory_dataType,
                                         const std::vector<Dimensions_t>& dimensions,
                                         const std::vector<Dimensions_t>& max_dimensions,
                                         const VariableCreationParameters& params) {
  /// \bug Need to add in some specification of max_dimensions.

  // This function translates ioda parameters to HH parameters.

  // Data type
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());

    // Data dimensions and max dimensions
    std::vector<hsize_t> hDims, hMaxDims;
    for (const auto& d : dimensions) hDims.push_back(gsl::narrow<hsize_t>(d));
    for (const auto& d : max_dimensions) {
      hMaxDims.push_back((d != ioda::Unlimited) ? gsl::narrow<hsize_t>(d) : H5S_UNLIMITED);
    }
    // Dataset creation parameters and chunk sizes
    ::HH::DatasetParameterPack hParams;

    // Always default to create intermediate groups if these do not already exist.
    // TODO(ryan): Expand HH to add this as a property.
    hParams.LinkCreationPlist = ::HH::Handles::HH_hid_t(
      H5Pcreate(H5P_LINK_CREATE), ::HH::Handles::Closers::CloseHDF5PropertyList::CloseP);
    if (H5Pset_create_intermediate_group(hParams.LinkCreationPlist.get(), 1) < 0)
      throw; //HH_throw.add<std::string>("Reason", "H5Pset_create_intermediate_group failed.");

    // Awkward phrasing. Custom chunk size overrides the value produced by the
    // filler function. But, the custom size can be invalid (or < 0), and then it
    // should be replaced by the custom function.
    // An error occurs iff we are using chunking at all.

    if (params.chunk) {
      std::vector<ioda::Dimensions_t> chunkSizesFunction, chunkSizesCustom = params.chunks;
      std::vector<hsize_t> hChunkSizes;
      Expects(params.fChunkingStrategy(dimensions, chunkSizesFunction));

      for (size_t i = 0; i < dimensions.size(); ++i) {
        ioda::Dimensions_t sz = 0;
        if (chunkSizesCustom.size() > i) sz = chunkSizesCustom[i];
        if (sz <= 0) sz = chunkSizesFunction[i];

        hChunkSizes.push_back(gsl::narrow<hsize_t>(sz));
      }

      hParams.datasetCreationProperties.chunk = true;
      hParams.customChunkSizes = hChunkSizes;
    }

    {
      hParams.datasetCreationProperties.hasFillValue = params.fillValue_.set_;
      hParams.datasetCreationProperties.fillValue_type = typeBackend->handle;
      const auto fvdata = params.finalize();
      hParams.datasetCreationProperties.fillValue.ld =
        fvdata.ld;  // NOLINT: std::variant is ideal, but it is a C++17 feature.
      // hParams.datasetCreationProperties.fillValue.ui64 = params.fillValue.ui64; // NOLINT: std::variant is
      // ideal, but it is a C++17 feature.

      hParams.datasetCreationProperties.szip = params.szip_;
      hParams.datasetCreationProperties.szip_options = params.szip_options_;
      hParams.datasetCreationProperties.szip_PixelsPerBlock = params.szip_PixelsPerBlock_;
      hParams.datasetCreationProperties.gzip = params.gzip_;
      hParams.datasetCreationProperties.gzip_level = params.gzip_level_;

      // Shuffle filter unused. TODO?
      // Scale-offset filter unused. TODO?

      // setDims and atts and other members are handled by
      /// the applyImmediatelyAfterVariableCreation function.
    }

    // Note: Char parameter added for HH compatability. Completely unnecessary.
    // Should update HH to remove the need for this.
    auto res = backend_.create<char>(name, hDims, hMaxDims, hParams, typeBackend->handle);
    auto b = std::make_shared<HH_Variable_Backend>(res, shared_from_this());
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
