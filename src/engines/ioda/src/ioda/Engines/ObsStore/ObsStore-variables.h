/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file ObsStore-variables.h
 * \brief Functions for ioda::Variable and ioda::Has_Variables backed by ObsStore
 */
#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "./ObsStore-attributes.h"
#include "./ObsStore-selection.h"
#include "./ObsStore-types.h"
#include "./Selection.hpp"
#include "./Types.hpp"
#include "./Variables.hpp"
#include "ioda/Group.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
// Spurious warning on Intel compilers:
// https://stackoverflow.com/questions/2571850/why-does-enable-shared-from-this-have-a-non-virtual-destructor
#if defined(__INTEL_COMPILER)
#  pragma warning(push)
#  pragma warning(disable : 444)
#endif

class ObsStore_HasVariables_Backend;

/// \brief This is the implementation of Variable in ioda::ObsStore
/// \ingroup ioda_internals_engines_obsstore
class ObsStore_Variable_Backend
    : public ioda::detail::Variable_Backend,
      public std::enable_shared_from_this<ObsStore::ObsStore_Variable_Backend> {
private:
  friend class ObsStore_HasVariables_Backend;

  /// \brief ObsStore Variable
  std::shared_ptr<ioda::ObsStore::Variable> backend_;

  /// Extra Attributes
  Has_Attributes impl_atts_;

public:
  ObsStore_Variable_Backend();
  ObsStore_Variable_Backend(std::shared_ptr<ioda::ObsStore::Variable>);
  virtual ~ObsStore_Variable_Backend();

  /// \brief return an ObsStore type marker
  detail::Type_Provider* getTypeProvider() const final;

  /// \brief return true if requested type matches stored type
  /// \param lhs type being requested
  bool isA(Type lhs) const final;

  /// \brief Does the Variable have an associated fill value?
  /// \returns true if yes, false if no.
  bool hasFillValue() const final;

  /// \brief Get the fill value associated with the Variable.
  FillValueData_t getFillValue() const final;

  /// \brief return dimensions of this variable
  Dimensions getDimensions() const final;

  /// \brief Get chunking information
  /// \see Variable_Base
  std::vector<Dimensions_t> getChunkSizes() const final;

  /// \brief Get GZIP compression information
  /// \see Variable_Base
  std::pair<bool, int> getGZIPCompression() const final;

  /// \brief Get SZIP compression information
  /// \see Variable_Base
  std::tuple<bool, unsigned, unsigned> getSZIPCompression() const final;

  /// \brief resize dimensions
  /// \param newDims new dimension sizes
  Variable resize(const std::vector<Dimensions_t>& newDims) final;

  /// \brief attach dimension to this variable
  /// \param DimensionNumber index of dimension (0, 1, ..., num_dims-1)
  /// \param scale existing variable holding dimension coordinate values
  Variable attachDimensionScale(unsigned int DimensionNumber, const Variable& scale) final;
  /// \brief detach dimensions to this variable
  /// \param DimensionNumber index of dimension (0, 1, ..., num_dims-1)
  /// \param scale existing variable holding dimension coordinate values
  Variable detachDimensionScale(unsigned int DimensionNumber, const Variable& scale) final;
  /// \brief is this variable a dimension scale (ie, hold coordinate values)
  bool isDimensionScale() const final;
  /// \brief set flag to denote this variable as a dimension scale
  /// \param res name of dimension scale variable
  Variable setIsDimensionScale(const std::string& dimensionScaleName) final;
  /// \brief given this variable is a dimension scale, return its name
  /// \param res name of this dimension scale
  Variable getDimensionScaleName(std::string& res) const final;
  /// \brief is the given dimension scale attached to the given dimension number
  /// \param DemensionNumber index of dimension (0, 1, ..., num_dims-1)
  /// \param scale dimension scale variable
  bool isDimensionScaleAttached(unsigned int DimensionNumber, const Variable& scale) const final;

  /// \brief transfer data into the ObsStore Variable
  /// \param data contiguous block of data to transfer
  /// \param in_memory_dataType frontend type marker
  /// \param mem_selection ioda::Selection for incoming data
  /// \param file_selection ioda::Selection for target Variable data
  Variable write(gsl::span<char> data, const Type& in_memory_dataType,
                 const Selection& mem_selection, const Selection& file_selection) final;
  /// \brief transfer data from the ObsStore Variable
  /// \param data contiguous block of data to transfer
  /// \param in_memory_dataType frontend type marker
  /// \param mem_selection ioda::Selection for target data
  /// \param file_selection ioda::Selection for incoming Variable data
  Variable read(gsl::span<char> data, const Type& in_memory_dataType,
                const Selection& mem_selection, const Selection& file_selection) const final;
};

/// \brief This is the implementation of Has_Variables in ioda::ObsStore
/// \ingroup ioda_internals_engines_obsstore
class ObsStore_HasVariables_Backend
    : public ioda::detail::Has_Variables_Backend,
      public std::enable_shared_from_this<ObsStore_HasVariables_Backend> {
private:
  /// \brief ObsStore Has_Variables
  std::shared_ptr<ioda::ObsStore::Has_Variables> backend_;

public:
  ObsStore_HasVariables_Backend();
  ObsStore_HasVariables_Backend(std::shared_ptr<ioda::ObsStore::Has_Variables>);
  virtual ~ObsStore_HasVariables_Backend();

  /// \brief return an ObsStore type marker
  detail::Type_Provider* getTypeProvider() const final;

  /// \brief return true if variable exists
  /// \param name name of variable
  bool exists(const std::string& name) const final;

  /// \brief remove variable
  /// \param name name of variable
  void remove(const std::string& name) final;

  /// \brief open variable (throws exception if not found)
  /// \param name name of variable
  Variable open(const std::string& name) const final;

  /// \brief return list of variables in this container
  std::vector<std::string> list() const final;

  /// \brief create a new variable
  /// \param attname name of variable
  /// \param in_memory_dataType fronted type marker
  /// \param dimensions dimensions of variable
  /// \param max_dimensions maximum_dimensions of variable (for resizing)
  /// \param params creation specs (chunking, fill value, compression, etc.)
  Variable create(const std::string& name, const Type& in_memory_dataType,
                  const std::vector<Dimensions_t>& dimensions     = {1},
                  const std::vector<Dimensions_t>& max_dimensions = {},
                  const VariableCreationParameters& params = VariableCreationParameters()) final;
};
#if defined(__INTEL_COMPILER)
#  pragma warning(pop)
#endif
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
