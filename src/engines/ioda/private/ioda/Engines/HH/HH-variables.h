#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "HH/Files.hpp"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
// Spurious warning on Intel compilers:
// https://stackoverflow.com/questions/2571850/why-does-enable-shared-from-this-have-a-non-virtual-destructor
#if defined(__INTEL_COMPILER)
#pragma warning(push)
#pragma warning(disable : 444)
#endif
class HH_HasVariables_Backend;

/// \brief This is the implementation of Variables using HDF5. Do not use outside of IODA.
class HH_Variable_Backend : public ioda::detail::Variable_Backend,
                            public std::enable_shared_from_this<HH::HH_Variable_Backend> {
  ::HH::Dataset backend_;
  std::weak_ptr<const HH_HasVariables_Backend> container_;

public:
  HH_Variable_Backend();
  HH_Variable_Backend(::HH::Dataset, std::shared_ptr<const HH_HasVariables_Backend>);
  virtual ~HH_Variable_Backend();
  detail::Type_Provider* getTypeProvider() const final;
  bool isA(Type lhs) const final;
  bool hasFillValue() const final;
  FillValueData_t getFillValue() const final;
  std::vector<Dimensions_t> getChunkSizes() const final;
  std::pair<bool, int> getGZIPCompression() const final;
  std::tuple<bool, unsigned, unsigned> getSZIPCompression() const final;
  // Encapsulated_Handle getType() const final;
  // bool isOfType(Encapsulated_Handle lhs, Encapsulated_Handle rhs) const override;
  // Encapsulated_Handle getSpace() const override;
  Dimensions getDimensions() const final;
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
  /// \param dimensionNumbers is the list of dimensions to scan. An empty value means scan everything.
  /// \returns a vector of size dimensionNumbers if dimensionNumbers is specified. If not, then
  ///   returns a vector with length equaling the dimensionality of the variable.
  std::vector<std::vector<std::pair<std::string, Variable>>> getDimensionScaleMappings(
    const std::vector<std::pair<std::string, Variable>>& scalesToQueryAgainst, bool firstOnly,
    const std::vector<unsigned>& dimensionNumbers) const;
  /// HDF5-specific, performance-focused implementation.
  bool isDimensionScaleAttached(unsigned int DimensionNumber, const Variable& scale) const final;
  /// HDF5-specific, performance-focused implementation.
  std::vector<std::vector<std::pair<std::string, Variable>>> getDimensionScaleMappings(
    const std::list<std::pair<std::string, Variable>>& scalesToQueryAgainst,
    bool firstOnly = true) const final;

  Variable write(gsl::span<char> data, const Type& in_memory_dataType, const Selection& mem_selection,
                 const Selection& file_selection) final;
  Variable read(gsl::span<char> data, const Type& in_memory_dataType, const Selection& mem_selection,
                const Selection& file_selection) const final;

  ::HH::HH_hid_t getSpaceWithSelection(const Selection& sel) const;
};

/// \brief This is the implementation of Has_Variables using HDF5. Do not use outside of IODA.
class HH_HasVariables_Backend : public ioda::detail::Has_Variables_Backend,
                                public std::enable_shared_from_this<HH_HasVariables_Backend> {
  ::HH::Has_Datasets backend_;
  ::HH::File fileroot_;

public:
  HH_HasVariables_Backend();
  HH_HasVariables_Backend(::HH::Has_Datasets, ::HH::File);
  virtual ~HH_HasVariables_Backend();
  detail::Type_Provider* getTypeProvider() const final;
  /// \brief Fill value policy in HDF5 depends on the current group and the root location.
  /// \details If the file was created by NetCDF4, then use the NetCDF4 policy.
  ///    If the file was created by HDF5, see if the root is an ObsGroup. If it is, use the NETCDF4 policy.
  ///    Otherwise, use the HDF5 policy.
  FillValuePolicy getFillValuePolicy() const final {
    if (fileroot_.atts.exists("_NCProperties"))
      return ioda::FillValuePolicy::NETCDF4;
    else if (fileroot_.atts.exists("_ioda_layout"))
      return ioda::FillValuePolicy::NETCDF4;
    return FillValuePolicy::HDF5;
  }
  bool exists(const std::string& name) const final;
  void remove(const std::string& name) final;
  Variable open(const std::string& name) const final;
  std::vector<std::string> list() const final;
  Variable create(const std::string& name, const Type& in_memory_dataType,
                  const std::vector<Dimensions_t>& dimensions = {1},
                  const std::vector<Dimensions_t>& max_dimensions = {},
                  const VariableCreationParameters& params = VariableCreationParameters()) final;
};
#if defined(__INTEL_COMPILER)
#pragma warning(pop)
#endif
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
