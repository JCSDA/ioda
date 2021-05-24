/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Variables.hpp
 * \brief Functions for ObsStore Variable and Has_Variables
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "./Attributes.hpp"
#include "./Selection.hpp"
#include "./Types.hpp"
#include "./VarAttrStore.hpp"
#include "ioda/Variables/Fill.h"
#include "ioda/defs.h"

namespace ioda {
namespace ObsStore {
// Spurious warning on Intel compilers:
// https://stackoverflow.com/questions/2571850/why-does-enable-shared-from-this-have-a-non-virtual-destructor
#if defined(__INTEL_COMPILER)
#  pragma warning(push)
#  pragma warning(disable : 444)
#endif
/// \brief parameters for creating a new variable
/// \ingroup ioda_internals_engines_obsstore
struct VarCreateParams {
public:
  // Data type element size
  std::size_t dtype_size = 0;

  // Fill value
  detail::FillValueData_t fvdata;
  gsl::span<char> fill_value;
};

/// \ingroup ioda_internals_engines_obsstore
class Variable : public std::enable_shared_from_this<Variable> {
private:
  /// \brief dimension sizes (length is rank of dimensions)
  std::vector<Dimensions_t> dimensions_;
  /// \brief maximum dimension sizes (for resizing)
  std::vector<Dimensions_t> max_dimensions_;
  /// \brief ObsStore data type
  ObsTypes dtype_ = ObsTypes::NOTYPE;
  /// \brief ObsStore data type
  std::size_t dtype_size_ = 0;

  /// \brief Fill value information
  detail::FillValueData_t fvdata_;

  /// \brief container for variable data values
  std::unique_ptr<VarAttrStore_Base> var_data_;

  /// \brief pointers to associated dimension scales
  std::vector<std::shared_ptr<Variable>> dim_scales_;

  /// \brief true if this variable is a dimension scale
  bool is_scale_ = false;

  /// \brief alias for this variable when it is serving as a dimension scale
  std::string scale_name_;

public:
  Variable() : atts(std::make_shared<Has_Attributes>()) {}
  Variable(const std::vector<Dimensions_t>& dimensions,
           const std::vector<Dimensions_t>& max_dimensions, const ObsTypes& dtype,
           const VarCreateParams& params);
  ~Variable() {}

  /// \brief container for variable attributes
  std::shared_ptr<Has_Attributes> atts;
  /// \brief implementation-specific attribute storage. Fill values, chunking,
  ///   compression settings, etc. Stuff that shouldn't be directly visible
  ///   to the client without using a dedicated function.
  std::shared_ptr<Has_Attributes> impl_atts;

  /// \brief returns dimension sizes
  std::vector<Dimensions_t> get_dimensions() const;
  /// \brief returns maximum dimension sizes
  std::vector<Dimensions_t> get_max_dimensions() const;
  /// \brief resizes dimensions (but cannot change dimensions themselves)
  /// \param new_dim_sizes new extents for each dimension
  void resize(const std::vector<Dimensions_t>& new_dim_sizes);
  /// \brief returns true if requested type matches stored type
  bool isOfType(ObsTypes dtype) const;
  /// \brief returns the data type.
  inline std::pair<ObsTypes, size_t> dtype() const { return std::make_pair(dtype_, dtype_size_); }

  /// \brief Is there an associated fill value?
  /// \returns true if yes, false if no.
  bool hasFillValue() const;

  /// \brief Get the fill value.
  detail::FillValueData_t getFillValue() const;

  /// \brief attach another variable to serve as a scale (holds coordinate values)
  /// \param dim_number dimension (by position) of which scale is associated
  /// \param scale variable serving as a scale
  void attachDimensionScale(const std::size_t dim_number, const std::shared_ptr<Variable> scale);

  /// \brief detach another variable that is servingas a scale (coordinate values)
  /// \param dim_number dimension (by position) of which scale is associated
  /// \param scale variable serving as a scale
  void detachDimensionScale(const std::size_t dim_number, const std::shared_ptr<Variable> scale);

  /// \brief returns true if this is being used as a scale for another variable
  bool isDimensionScale() const;

  /// \brief set this variable as a dimension scale
  /// \param name name for this dimension scale
  void setIsDimensionScale(const std::string& name);

  /// \brief get the dimension scale name
  /// \param name name for this dimension scale
  void getDimensionScaleName(std::string& name) const;

  /// \brief return true if the scale is attached to this variable
  /// \param name name for this dimension scale
  bool isDimensionScaleAttached(const std::size_t dim_number,
                                const std::shared_ptr<Variable> scale) const;

  /// \brief transfer data to variable storage
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select from data argument
  /// \param f_select Selection ojbect: how to select to variable storage
  std::shared_ptr<Variable> write(gsl::span<char> data, ObsTypes dtype, Selection& m_select,
                                  Selection& f_select);
  /// \brief transfer data from variable storage
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select to data argument
  /// \param f_select Selection ojbect: how to select from variable storage
  std::shared_ptr<Variable> read(gsl::span<char> data, ObsTypes dtype, Selection& m_select,
                                 Selection& f_select);
};

class Group;
/// \ingroup ioda_internals_engines_obsstore
class Has_Variables {
private:
  /// \brief container of variables
  std::map<std::string, std::shared_ptr<Variable>> variables_;

  /// \brief pointer to parent group
  std::weak_ptr<Group> parent_group_;

  /// \brief split a path into groups and variable pieces
  /// \param path Hierarchical path
  static std::vector<std::string> splitGroupVar(const std::string& path);

public:
  Has_Variables() {}
  ~Has_Variables() {}

  /// \brief create a new variable
  /// \param name name of new variable
  /// \param dtype ObsStore data type of new variable
  /// \param dims dimensions of new variable (length is rank of dimensions)
  /// \param max_dims maximum dimensions of new variable (for resizing)
  /// \param params parameters for creating new variable
  std::shared_ptr<Variable> create(const std::string& name, const ioda::ObsStore::ObsTypes& dtype,
                                   const std::vector<Dimensions_t>& dims,
                                   const std::vector<Dimensions_t>& max_dims,
                                   const VarCreateParams& params);

  /// \brief open an existing variable (throws exception if not found)
  std::shared_ptr<Variable> open(const std::string& name) const;

  /// \brief returns true if variable exists in the container
  /// \param name name of variable to check
  bool exists(const std::string& name) const;

  /// \brief remove variable
  /// \param name name of variable to remove
  void remove(const std::string& name);

  /// \brief rename variable
  /// \param oldName current name of variable
  /// \param newName new name of variable
  void rename(const std::string& oldName, const std::string& newName);

  /// \brief returns a list of names of the variables in the container
  std::vector<std::string> list() const;

  /// \brief set parent group pointer
  /// \param parentGroup pointer to group that owns this Has_Variables object
  void setParentGroup(const std::shared_ptr<Group>& parentGroup);
};
#if defined(__INTEL_COMPILER)
#  pragma warning(pop)
#endif
}  // namespace ObsStore
}  // namespace ioda

/// @}
