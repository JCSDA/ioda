#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_layout
 *
 * @{
 * \file ObsGroup.h
 * \brief Interfaces for ioda::ObsGroup and related classes.
 */

#include <memory>
#include <utility>
#include <vector>

#include "./Group.h"
#include "./Misc/DimensionScales.h"
#include "./defs.h"

namespace ioda {
class ObsGroup;
namespace detail {
class DataLayoutPolicy;
}  // namespace detail

/// \brief An ObsGroup is a specialization of a ioda::Group. It provides convenience functions
///   and guarantees that the ioda data are well-formed.
/// \ingroup ioda_cxx_layout
class IODA_DL ObsGroup : public Group {
  /// Identifies the current version of the ObsGroup schema.
  static const int current_schema_version_;

  /// Set the mapping policy to determine the Layout of Variables stored under this Group.
  void setLayout(std::shared_ptr<const detail::DataLayoutPolicy>);
  /// Mapping policy
  std::shared_ptr<const detail::DataLayoutPolicy> layout_;

public:
  /// \see generate for parameters.
  ObsGroup(Group g, std::shared_ptr<const detail::DataLayoutPolicy> layout = nullptr);
  ObsGroup();
  virtual ~ObsGroup();

  /// \brief Create an empty ObsGroup and populate it with
  /// the fundamental dimensions.
  /// \param emptyGroup is an empty Group that will be filled
  ///   with the ObsGroup.
  /// \param fundamentalDims is a collection of dimension names,
  ///   data types and dimension types (horizontal, vertical,
  ///   temporal, other) that define the basic dimensiosn of
  ///   the ObsGroup.
  /// \param layout describes how the ObsGroup arranges its data internally.
  ///   Use nullptr to select the default policy.
  static ObsGroup generate(Group& emptyGroup, const NewDimensionScales_t& fundamentalDims,
                           std::shared_ptr<const detail::DataLayoutPolicy> layout = nullptr);

  /// \brief Resize a Dimension and every Variable that
  ///   depends on it.
  /// \details This operation is recursive on all objects within
  ///   the Group.
  /// \throws if the input is not a dimension.
  /// \param newDims is a vector of pairs of the Dimension and
  ///   its new size. If the dimension shrinks, then any data are
  ///   truncated. If it grows, then data are set to the fill value.
  ///
  void resize(const std::vector<std::pair<Variable, ioda::Dimensions_t>>& newDims);

private:
  /// \brief recusively visit all groups and resize variables according
  /// to newDims.
  /// \param group Current group in traversal
  /// \param newDims Vector of pairs of Dimension and new size
  static void resizeVars(Group& g,
                         const std::vector<std::pair<Variable, ioda::Dimensions_t>>& newDims);

  /// Create ObsGroup objects
  void setup(const NewDimensionScales_t& fundamentalDims,
             std::shared_ptr<const detail::DataLayoutPolicy> layout);
};

}  // namespace ioda

/// @}
