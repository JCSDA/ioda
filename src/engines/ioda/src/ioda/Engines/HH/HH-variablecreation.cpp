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
 * \file HH-variablecreation.cpp
 * \brief HDF5 engine Variable creation and property lists.
 */

#include "./HH/HH-variablecreation.h"

#include <hdf5.h>

#include <algorithm>
#include <numeric>
#include <set>

#include "./HH/HH-Filters.h"
#include "./HH/HH-hasvariables.h"
#include "./HH/HH-types.h"
#include "./HH/Handles.h"
#include "ioda/Misc/DimensionScales.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
VariableCreation::VariableCreation(const VariableCreationParameters& p, const Vec_t& dims,
                                   const Vec_t& max_dims, std::shared_ptr<HH_Type> data_type) {
  // This constructor generates and stores the dataset creation property list.
  // We only have to do this once per variable, so we might as well do it here.

  dcp_ = HH_hid_t(H5Pcreate(H5P_DATASET_CREATE), Handles::Closers::CloseHDF5PropertyList::CloseP);
  if (!dcp_.isValid()) throw;

  // Convert dims and max_dims to HDF5 equivalents
  // Data dimensions and max dimensions
  for (const auto& d : dims) dims_.push_back(gsl::narrow<hsize_t>(d));
  for (const auto& d : max_dims) {
    max_dims_.push_back((d != ioda::Unlimited) ? gsl::narrow<hsize_t>(d) : H5S_UNLIMITED);
  }

  // Chunking
  if (p.chunk) {
    // Either the user can specify chunk sizes manually, or we calculate these by
    // taking hints from the dimensions that the variable will be attached to.
    // Those hints are determined using a function.

    /// \todo Revise chunking calculation.
    // TODO(ryan): Revise and combine with the dimension scale hint.
    // Bit of an awkward call. If chunks are manually set, it uses those. Otherwise,
    // it uses the initial variable size as a hint. Problematic because we get to override it
    // if zero.

    auto chunksizes = p.getChunks(dims);
    std::vector<hsize_t> hcs(chunksizes.size());  // chunksizes converted to hsize_t.
    for (size_t i = 0; i < chunksizes.size(); ++i) {
      hcs[i] = gsl::narrow<hsize_t>(  // Always narrow to hsize_t.
        (chunksizes[i] > 0) ? chunksizes[i] : dims_[i]);
      if (hcs[i] == 0) throw;  // We need a hint at this point.
    }

    if (H5Pset_chunk(dcp_(), static_cast<int>(hcs.size()), hcs.data()) < 0) throw;
  }

  // Filters (compression, shuffle, scale-offset)
  //
  // We only expose basic compression options for now, but could add support for
  // more advanced compression plugins and additional filters in the future. This
  // depends on user needs and a redesign of how the user specifies these options when
  // creating a variable. The current method is already rather complex.
  {
    if ((p.gzip_ || p.szip_) && !p.chunk)
      throw;  // Compression filters are only allowed when chunking is used.

    Filters filt(dcp_.get());
    if (p.gzip_) filt.setGZIP(p.gzip_level_);
    if (p.szip_) filt.setSZIP(p.szip_options_, p.szip_PixelsPerBlock_);
  }

  // Initial fill value
  if (p.fillValue_.set_) {
    // finalization makes sure that strings and other odd types are handled properly.
    const FillValueData_t::FillValueUnion_t fvdata = p.fillValue_.finalize();
    if (0 > H5Pset_fill_value(dcp_(), data_type->handle(), &(fvdata))) throw;
  }
}

HH_hid_t VariableCreation::datasetCreationPlist() const { return dcp_; }

HH_hid_t VariableCreation::dataspace() const {
  auto space_id = H5Screate_simple(gsl::narrow<int>(dims_.size()), dims_.data(), max_dims_.data());
  if (space_id < 0) throw;

  HH_hid_t res(space_id, Handles::Closers::CloseHDF5Dataspace::CloseP);
  return res;
}

HH_hid_t VariableCreation::datasetAccessPlist() {
  static HH_hid_t res(H5P_DEFAULT, Handles::Closers::DoNotClose::CloseP);
  return res;
}

HH_hid_t VariableCreation::linkCreationPlist() {
  // Always default to create intermediate groups if these do not already exist.
  HH_hid_t res(H5Pcreate(H5P_LINK_CREATE), Handles::Closers::CloseHDF5PropertyList::CloseP);
  if (H5Pset_create_intermediate_group(res.get(), 1) < 0) throw;
  return res;
}
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
