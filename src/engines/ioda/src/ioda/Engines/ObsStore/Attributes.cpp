/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Attributes.cpp
 * \brief Functions for ObsStore Attribute and Has_Attributes
 */
#include <functional>
#include <numeric>

#include "./Attributes.hpp"
#include "ioda/Exception.h"


namespace ioda {
namespace ObsStore {
//*********************************************************************
//                          Attribute functions
//*********************************************************************
Attribute::Attribute(const std::vector<std::size_t>& dimensions, const ObsTypes& dtype)
    : dimensions_(dimensions), dtype_(dtype), attr_data_() {
  // Get a typed storage object based on dtype
  attr_data_.reset(createVarAttrStore(dtype_));

  // Set the size of the attribute value
  std::size_t numElements = std::accumulate(dimensions_.begin(), dimensions_.end(), (size_t)1,
                                            std::multiplies<std::size_t>());
  attr_data_->resize(numElements);
}

std::vector<std::size_t> Attribute::get_dimensions() const { return dimensions_; }

bool Attribute::isOfType(ObsTypes dtype) const { return (dtype == dtype_); }

std::shared_ptr<Attribute> Attribute::write(gsl::span<char> data, ObsTypes dtype) {
  if (dtype != dtype_)
    throw Exception("Requested data type not equal to storage datatype.", ioda_Here());
  
  // Create select objects for all elements. Ie, attributes don't use
  // selection, but the VarAttrStore object is also used by variables which
  // do use selection.
  std::size_t start   = 0;
  std::size_t npoints = 1;
  for (std::size_t idim = 0; idim < dimensions_.size(); ++idim) {
    npoints *= dimensions_[idim];
  }
  Selection m_select(start, npoints);
  Selection f_select(start, npoints);
  attr_data_->write(data, m_select, f_select);
  return shared_from_this();
}

std::shared_ptr<Attribute> Attribute::read(gsl::span<char> data, ObsTypes dtype) {
  if (dtype != dtype_) 
    throw Exception("Requested data type not equal to storage datatype", ioda_Here());
  
  // Create select objects for all elements. Ie, attributes don't use
  // selection, but the VarAttrStore object is also used by variables which
  // do use selection.
  std::size_t start   = 0;
  std::size_t npoints = 1;
  for (std::size_t idim = 0; idim < dimensions_.size(); ++idim) {
    npoints *= dimensions_[idim];
  }
  Selection m_select(start, npoints);
  Selection f_select(start, npoints);
  attr_data_->read(data, m_select, f_select);
  return shared_from_this();
}

//*********************************************************************
//                        Has_Attributes function
//*********************************************************************
std::shared_ptr<Attribute> Has_Attributes::create(const std::string& name,
                                                  const ioda::ObsStore::ObsTypes& dtype,
                                                  const std::vector<std::size_t>& dims) {
  std::shared_ptr<Attribute> att(new Attribute(dims, dtype));
  attributes_.insert(std::pair<std::string, std::shared_ptr<Attribute>>(name, att));
  return att;
}

std::shared_ptr<Attribute> Has_Attributes::open(const std::string& name) const {
  auto iattr = attributes_.find(name);
  if (iattr == attributes_.end())
    throw Exception("Attribute not found.", ioda_Here()).add("name", name);

  return iattr->second;
}

bool Has_Attributes::exists(const std::string& name) const {
  return (attributes_.find(name) != attributes_.end());
}

void Has_Attributes::remove(const std::string& name) { attributes_.erase(name); }

void Has_Attributes::rename(const std::string& oldName, const std::string& newName) {
  std::shared_ptr<Attribute> att = open(oldName);
  attributes_.erase(oldName);
  attributes_.insert(std::pair<std::string, std::shared_ptr<Attribute>>(newName, att));
}

std::vector<std::string> Has_Attributes::list() const {
  std::vector<std::string> attrList;
  for (auto iattr = attributes_.begin(); iattr != attributes_.end(); ++iattr) {
    attrList.push_back(iattr->first);
  }
  return attrList;
}
}  // namespace ObsStore
}  // namespace ioda

/// @}
