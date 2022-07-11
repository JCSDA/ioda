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
  // Deliberately not an equality comparison. max_dims may be unspecified.
  if (dims.size() < max_dims.size()) throw;  // TODO(ryan): Separate PR for ioda Exceptions class.
  for (size_t i = 0; i < max_dims.size(); ++i) {
    if (max_dims[i] == ioda::Unlimited)
      max_dims_.push_back(H5S_UNLIMITED);
    else if (max_dims[i] == ioda::Unspecified)
      max_dims_.push_back(gsl::narrow<hsize_t>(dims[i]));
    else
      max_dims_.push_back(gsl::narrow<hsize_t>(max_dims[i]));
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
    //
    // Need to handle case where a dimension size and corresponding max dimension size
    // are both zero. This comes up for example when an obs space contains zero obs and
    // we are copying the obs space to a file (ioda writer).
    //
    // We need to avoid setting the chunk size to zero.
    //   First off check if the chunk size in the variable creation parameters is set
    //   to a size > 0. If so use it; if not use the correspoding dims size.
    //
    //   Second limit the chunk size to the max_dims size, unless max_dims size is
    //   is set to unlimited (== -1).
    //
    //   If we still have the chunksize set to zero, use an arbitrary default size
    //   for now (100).

    auto chunksizes = p.getChunks(dims);
    std::vector<hsize_t> hcs(chunksizes.size());  // chunksizes converted to hsize_t.
    for (size_t i = 0; i < chunksizes.size(); ++i) {
      hcs[i] = gsl::narrow<hsize_t>(  // Always narrow to hsize_t.
        (chunksizes[i] > 0) ? chunksizes[i] : dims_[i]);

      if (max_dims_[i] > 0) {
        if (hcs[i] > max_dims_[i]) hcs[i] = gsl::narrow<hsize_t>(max_dims_[i]);
      }

      if (hcs[i] == 0) {
        hcs[i] = 100;
      }
    }
    final_chunks_ = hcs;

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
  hid_t space = (dims_.empty()) ? H5Screate(H5S_SCALAR)
                                : H5Screate_simple(gsl::narrow<int>(dims_.size()), dims_.data(),
                                                   max_dims_.data());
  if (space < 0) throw;

  HH_hid_t res(space, Handles::Closers::CloseHDF5Dataspace::CloseP);
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
