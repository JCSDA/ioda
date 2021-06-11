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
 * \file HH-hasvariables.h
 * \brief HDF5 engine implementation of Has_Variables.
 */

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
/// \brief This encapsulates dataset creation parameters
/// \ingroup ioda_internals_engines_hh
class IODA_HIDDEN DatasetParameterPack {
  friend class HH_HasVariables;

  DatasetParameterPack(const VariableCreationParameters&);

  HH_hid_t datasetCreationPlist() const;
  /// @brief The default dataset access property list. Currently a nullop.
  /// @return A handle to the default HDF5 property list.
  static HH_hid_t datasetAccessPlist();
  /// @brief The ioda-default link creation property list.
  /// @detail This just sets a property to create missing intermediate groups.
  /// @return A handle to the HDF5 property list.
  static HH_hid_t linkCreationPlist();
};

/// \brief This is the implementation of Has_Variables using HDF5.
class IODA_HIDDEN HH_HasVariables : public ioda::detail::Has_Variables_Backend,
                                    public std::enable_shared_from_this<HH_HasVariables> {
  HH_hid_t base_;
  HH_hid_t fileroot_;

public:
  HH_HasVariables();
  HH_HasVariables(HH_hid_t grp, HH_hid_t fileroot);
  virtual ~HH_HasVariables();
  detail::Type_Provider* getTypeProvider() const final;
  FillValuePolicy getFillValuePolicy() const final;
  bool exists(const std::string& name) const final;
  void remove(const std::string& name) final;
  Variable open(const std::string& name) const final;
  std::vector<std::string> list() const final;
  Variable create(const std::string& name, const Type& in_memory_dataType,
                  const std::vector<Dimensions_t>& dimensions     = {1},
                  const std::vector<Dimensions_t>& max_dimensions = {},
                  const VariableCreationParameters& params = VariableCreationParameters()) final;

  /*! HDF5-optimized collective variable version of attachDimensionScales.
* 
* This function exists to improve performance. When attaching many
* variables to the same dimension scale, HDF5's HL library performs
* suboptimally. Each time a new variable is attached to a scale, that
* scale's REFERENCE_LIST attribute must be resized and recreated. This collective
* function call avoids this issue by rewriting H5DSattach_scale to attach multiple
* variables to a scale at the same time.
* 
* @see https://github.com/HDFGroup/hdf5/blob/develop/hl/src/H5DS.c#L107 for the HDF5 function.
* @param mapping is a sequence of variables along with their dimension scales.
*/
  void attachDimensionScales(
    const std::vector<std::pair<Variable, std::vector<Variable>>>& mapping)
    final;
};
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda

/// @}
