/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Variables.cpp
 * \brief Functions for ObsStore Variable and Has_Variables
 */

#include "./Variables.hpp"

#include <exception>
#include <functional>
#include <numeric>

#include "./Group.hpp"

namespace ioda {
namespace ObsStore {
//***************************************************************************
// Variable methods
//****************************************************************************
Variable::Variable(const std::vector<Dimensions_t>& dimensions,
                   const std::vector<Dimensions_t>& max_dimensions, const ObsTypes& dtype,
                   const VarCreateParams& params)
    : dimensions_(dimensions),
      max_dimensions_(max_dimensions),
      dtype_(dtype),
      dtype_size_(0),
      var_data_(),
      is_scale_(false),
      atts(std::make_shared<Has_Attributes>()),
      impl_atts(std::make_shared<Has_Attributes>()) {
  // Get a typed storage object based on dtype
  var_data_.reset(createVarAttrStore(dtype_));
  dtype_size_ = params.dtype_size;

  // If have a fill value, save in an attribute. Do this before resizing
  // because resize() will check for the fill value.
  if (params.fvdata.set_) {
    auto fv = impl_atts->create("_fillValue", dtype_, {1});
    fv->write(params.fill_value, dtype_);

    this->fvdata_ = params.fvdata;
  }

  // Set the size of the variable value
  this->resize(dimensions_);

  // Anticipate attaching dimension scales. The dim_scales_ data member won't
  // be used if this variable is a dimension scale.
  dim_scales_.assign(dimensions_.size(), nullptr);
}

std::vector<Dimensions_t> Variable::get_dimensions() const { return dimensions_; }

std::vector<Dimensions_t> Variable::get_max_dimensions() const { return max_dimensions_; }

void Variable::resize(const std::vector<Dimensions_t>& new_dim_sizes) {
  // Check new_dim_sizes versus max_dimensions.
  for (std::size_t i = 0; i < max_dimensions_.size(); ++i) {
    if (max_dimensions_[i] >= 0) {
      if (new_dim_sizes[i] > max_dimensions_[i]) {
        throw;  // jedi_throw.add("Reason", "new_dim_sizes exceeds max_dimensions_");
      }
    }
  }

  // Set the dimensions_ data member
  dimensions_ = new_dim_sizes;

  // Allow for the total number of elements to change. If there are
  // addtional elements (total size is growing), then fill those elements
  // with the variable's fill value (if exists).
  std::size_t numElements = std::accumulate(new_dim_sizes.begin(), new_dim_sizes.end(), (size_t)1,
                                            std::multiplies<std::size_t>());

  if (impl_atts->exists("_fillValue")) {
    std::vector<char> fvalue(dtype_size_);
    gsl::span<char> fillValue(fvalue.data(), dtype_size_);
    impl_atts->open("_fillValue")->read(fillValue, dtype_);
    var_data_->resize(numElements, fillValue);
  } else {
    var_data_->resize(numElements);
  }
}

bool Variable::isOfType(ObsTypes dtype) const { return (dtype == dtype_); }

bool Variable::hasFillValue() const { return fvdata_.set_; }

detail::FillValueData_t Variable::getFillValue() const { return fvdata_; }

void Variable::attachDimensionScale(const std::size_t dim_number,
                                    const std::shared_ptr<Variable> scale) {
  dim_scales_[dim_number] = scale;
}

void Variable::detachDimensionScale(const std::size_t dim_number,
                                    const std::shared_ptr<Variable> scale) {
  if (dim_scales_[dim_number] == scale) {
    dim_scales_[dim_number] = nullptr;
  } else {
    std::string ErrMsg = std::string("ioda::ObsStore::Variable::detachDimensionScale: ")
                         + std::string("specified scale is not found");
    throw;  // jedi_throw.add("Reason", ErrMsg);
  }
}

bool Variable::isDimensionScale() const { return is_scale_; }

void Variable::setIsDimensionScale(const std::string& name) {
  is_scale_   = true;
  scale_name_ = name;
}

void Variable::getDimensionScaleName(std::string& name) const { name = scale_name_; }

bool Variable::isDimensionScaleAttached(const std::size_t dim_number,
                                        const std::shared_ptr<Variable> scale) const {
  return (dim_scales_[dim_number] == scale);
}

std::shared_ptr<Variable> Variable::write(gsl::span<char> data, ObsTypes dtype, Selection& m_select,
                                          Selection& f_select) {
  if (dtype != dtype_) {
    std::string ErrMsg = std::string("ioda::ObsStore::Variable::write: ")
                         + std::string("Requested data type not equal to storage datatype");
    throw std::logic_error(
      "Requested data type not equal to storage datatype");  // jedi_throw.add("Reason", ErrMsg);
  }

  var_data_->write(data, m_select, f_select);
  return shared_from_this();
}

std::shared_ptr<Variable> Variable::read(gsl::span<char> data, ObsTypes dtype, Selection& m_select,
                                         Selection& f_select) {
  if (dtype != dtype_) {
    std::string ErrMsg = std::string("ioda::ObsStore::Variable::read: ")
                         + std::string("Requested data type not equal to storage datatype");
    throw std::logic_error(ErrMsg);  // jedi_throw.add("Reason", ErrMsg);
  }

  var_data_->read(data, m_select, f_select);
  return shared_from_this();
}

//***************************************************************************
// Has_Variable methods
//****************************************************************************
std::shared_ptr<Variable> Has_Variables::create(const std::string& name,
                                                const ioda::ObsStore::ObsTypes& dtype,
                                                const std::vector<Dimensions_t>& dims,
                                                const std::vector<Dimensions_t>& max_dims,
                                                const VarCreateParams& params) {
  // If have hiearchical name, then go to the parent group to create the
  // intermediate groups and create the varible at the group at the end
  // of the intermediate group chain. Otherwise create the varible in this
  // this group.
  std::shared_ptr<Variable> var;
  std::vector<std::string> splitPaths = splitGroupVar(name);
  if (splitPaths.size() > 1) {
    // Have intermediate groups, create variable in "bottom" group
    std::shared_ptr<Group> parentGroup = parent_group_.lock();
    std::shared_ptr<Group> group       = parentGroup->create(splitPaths[0]);
    var = group->vars->create(splitPaths[1], dtype, dims, max_dims, params);
  } else {
    // No intermediate groups, create variable here
    var = std::make_shared<Variable>(dims, max_dims, dtype, params);
    variables_.insert(std::pair<std::string, std::shared_ptr<Variable>>(name, var));
  }
  return var;
}

std::shared_ptr<Variable> Has_Variables::open(const std::string& name) const {
  std::shared_ptr<Variable> var;
  std::vector<std::string> splitPaths = splitGroupVar(name);
  if (splitPaths.size() > 1) {
    std::shared_ptr<Group> parentGroup = parent_group_.lock();
    std::shared_ptr<Group> group       = parentGroup->open(splitPaths[0]);
    var                                = group->vars->open(splitPaths[1]);
  } else {
    auto ivar = variables_.find(name);
    if (ivar == variables_.end()) {
      std::string ErrMsg = std::string("ioda::ObsStore::Has_Variables::open: ")
                           + std::string("Variable not found: ") + name;
      throw std::logic_error(ErrMsg);  // jedi_throw.add("Reason", ErrMsg);
    }
    var = ivar->second;
  }
  return var;
}

bool Has_Variables::exists(const std::string& name) const {
  bool varExists                      = false;
  std::vector<std::string> splitPaths = splitGroupVar(name);
  if (splitPaths.size() > 1) {
    std::shared_ptr<Group> parentGroup = parent_group_.lock();
    if (parentGroup->exists(splitPaths[0])) {
      std::shared_ptr<Group> group = parentGroup->open(splitPaths[0]);
      varExists                    = group->vars->exists(splitPaths[1]);
    }
  } else {
    varExists = (variables_.find(name) != variables_.end());
  }
  return varExists;
}

void Has_Variables::remove(const std::string& name) {
  std::vector<std::string> splitPaths = splitGroupVar(name);
  if (splitPaths.size() > 1) {
    std::shared_ptr<Group> parentGroup = parent_group_.lock();
    std::shared_ptr<Group> group       = parentGroup->open(splitPaths[0]);
    group->vars->remove(splitPaths[1]);
  } else {
    variables_.erase(name);
  }
}

void Has_Variables::rename(const std::string& oldName, const std::string& newName) {
  std::vector<std::string> splitPaths = splitGroupVar(oldName);
  if (splitPaths.size() > 1) {
    std::shared_ptr<Group> parentGroup = parent_group_.lock();
    std::shared_ptr<Group> group       = parentGroup->open(splitPaths[0]);
    group->vars->rename(splitPaths[1], newName);
  } else {
    std::shared_ptr<Variable> var = open(oldName);
    variables_.erase(oldName);
    variables_.insert(std::pair<std::string, std::shared_ptr<Variable>>(newName, var));
  }
}

std::vector<std::string> Has_Variables::list() const {
  std::vector<std::string> varList;
  for (auto ivar = variables_.begin(); ivar != variables_.end(); ++ivar) {
    varList.push_back(ivar->first);
  }
  return varList;
}

void Has_Variables::setParentGroup(const std::shared_ptr<Group>& parentGroup) {
  parent_group_ = parentGroup;
}

// private methods
std::vector<std::string> Has_Variables::splitGroupVar(const std::string& path) {
  std::vector<std::string> splitPath;
  auto pos = path.find_last_of('/');
  splitPath.push_back(path.substr(0, pos));
  if (pos != std::string::npos) {
    splitPath.push_back(path.substr(pos + 1));
  }
  return splitPath;
}
}  // namespace ObsStore
}  // namespace ioda

/// @}
