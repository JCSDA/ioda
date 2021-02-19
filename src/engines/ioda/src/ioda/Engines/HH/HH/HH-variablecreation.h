#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file HH-variablecreation.h
/// \brief HDF5 engine variable creation parameters.

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "./Handles.h"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
class HH_Type;

/// \brief This encapsulates dataset creation parameters. Used for generating HDF5 property lists
///   for variable creation.
class IODA_HIDDEN VariableCreation {
  friend class HH_HasVariables;

  typedef std::vector<hsize_t> VecHS_t;
  typedef std::vector<ioda::Dimensions_t> Vec_t;
  VecHS_t dims_, max_dims_;
  HH_hid_t dcp_;

  /// @brief Manages property lists for HDF5 variable creation.
  /// @param p represents the ioda frontend variable creation parameters. These are specified by
  ///   the user. Chunking, compression, and fill values are specified here.
  /// @param dims are the dimensions at creation time.
  /// @param max_dims are the max dimensions, specified at variable creation time.
  /// @param data_type represents the internal HDF5 data type. Needed when setting fill values.
  VariableCreation(const VariableCreationParameters& p, const Vec_t& dims, const Vec_t& max_dims,
                   std::shared_ptr<HH_Type> data_type);

  /// @brief Generates a dataset creation property list, which encodes the chunking options,
  ///   compression, and the initial fill value used for the dataset.
  /// @return A smart handle to the dataset creation property list.
  HH_hid_t datasetCreationPlist() const;
  /// @brief Generate a dataspace for the constructor-provided dimensions and max dimensions.
  /// @return A smart handle to the dataspace.
  HH_hid_t dataspace() const;
  /// @brief The default dataset access property list. Currently a nullop.
  /// @return A handle to the default HDF5 property list.
  static HH_hid_t datasetAccessPlist();
  /// @brief The ioda-default link creation property list.
  /// @detail This just sets a property to create missing intermediate groups.
  /// @return A handle to the HDF5 property list.
  static HH_hid_t linkCreationPlist();
};

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
