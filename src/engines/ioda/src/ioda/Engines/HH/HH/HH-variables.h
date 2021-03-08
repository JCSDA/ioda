#pragma once
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
 * \file HH-variables.h
 * \brief HDF5 engine implementation of Variable.
 */

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "./HH-attributes.h"
#include "./Handles.h"
#include "ioda/Group.h"
#include "ioda/Types/Type.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
class HH_HasVariables;

/// \brief This is the implementation of Variables using HDF5.
/// \ingroup ioda_internals_engines_hh
class IODA_HIDDEN HH_Variable : public ioda::detail::Variable_Backend,
                                public std::enable_shared_from_this<HH_Variable> {
  HH_hid_t var_;
  std::weak_ptr<const HH_HasVariables> container_;

public:
  HH_Variable();
  HH_Variable(HH_hid_t var, std::shared_ptr<const HH_HasVariables> container);
  virtual ~HH_Variable();

  HH_hid_t get() const;
  bool isVariable() const;

  HH_hid_t type() const;
  detail::Type_Provider* getTypeProvider() const final;

  /// General function to check type matching. Approximate match in the HDF5 context
  /// to account for different floating point and string types across systems.
  bool isA(Type lhs) const final;

  /// Convenience function to check an dataset's type.
  /// \ttype DataType is the typename.
  /// \returns True if the type matches
  /// \returns False (0) if the type does not match
  /// \returns <0 if an error occurred.
  template <class DataType>
  bool isA() const {
    auto ttype = Types::GetType<DataType>(getTypeProvider());
    return isA(ttype);
  }

  /// Convenience function to check an attribute's type.
  /// \param ttype is the type to test against.
  /// \returns True if the type matches
  /// \returns False if the type does not match
  bool isExactlyA(HH_hid_t ttype) const;

  /// Convenience function to check an dataset's type.
  /// \returns True if the type matches
  /// \returns False (0) if the type does not match
  /// \returns <0 if an error occurred.
  template <class DataType>
  bool isExactlyA() const {
    auto ttype     = Types::GetType<DataType>(getTypeProvider());
    HH_hid_t otype = type();
    auto ret       = H5Tequal(ttype(), otype());
    if (ret < 0) throw;  // HH_throw;
    return (ret > 0) ? true : false;
  }

  HH_hid_t space() const;
  Dimensions getDimensions() const final;

  bool hasFillValue() const final;
  FillValueData_t getFillValue() const final;
  std::vector<Dimensions_t> getChunkSizes() const final;
  std::pair<bool, int> getGZIPCompression() const final;
  std::tuple<bool, unsigned, unsigned> getSZIPCompression() const final;

  Variable resize(const std::vector<Dimensions_t>& newDims) final;

  Variable attachDimensionScale(unsigned int DimensionNumber, const Variable& scale) final;
  Variable detachDimensionScale(unsigned int DimensionNumber, const Variable& scale) final;
  bool isDimensionScale() const final;
  Variable setIsDimensionScale(const std::string& dimensionScaleName) final;
  Variable getDimensionScaleName(std::string& res) const final;

  /// HDF5-generalized function, with emphasis on performance. Acts as the real function for
  /// both getDimensionScaleMappings and isDimensionScaleAttached.
  /// \param scalesToQueryAgainst is the list of scales that are being queried against.
  /// \param firstOnly reports only the first match along each axis
  /// \param dimensionNumbers is the list of dimensions to scan. An empty value means scan
  ///   everything.
  /// \returns a vector of size dimensionNumbers if dimensionNumbers is specified. If
  ///   not, then returns a vector with length equaling the dimensionality of the variable.
  std::vector<std::vector<std::pair<std::string, Variable>>> getDimensionScaleMappings(
    const std::vector<std::pair<std::string, Variable>>& scalesToQueryAgainst, bool firstOnly,
    const std::vector<unsigned>& dimensionNumbers) const;
  /// HDF5-specific, performance-focused implementation.
  bool isDimensionScaleAttached(unsigned int DimensionNumber, const Variable& scale) const final;
  /// HDF5-specific, performance-focused implementation.
  std::vector<std::vector<std::pair<std::string, Variable>>> getDimensionScaleMappings(
    const std::list<std::pair<std::string, Variable>>& scalesToQueryAgainst,
    bool firstOnly = true) const final;

  Variable write(gsl::span<char> data, const Type& in_memory_dataType,
                 const Selection& mem_selection, const Selection& file_selection) final;
  Variable read(gsl::span<char> data, const Type& in_memory_dataType,
                const Selection& mem_selection, const Selection& file_selection) const final;

  HH_hid_t getSpaceWithSelection(const Selection& sel) const;
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
