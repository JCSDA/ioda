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

#include <hdf5.h>  // Type conversion support

#include "./Attributes.hpp"
#include "../HH/HH/HH-util.h"  // Error catching in H5T_convert
#include "ioda/Exception.h"


namespace ioda {
namespace ObsStore {
//*********************************************************************
//                          Attribute functions
//*********************************************************************
Attribute::Attribute(const std::vector<std::size_t>& dimensions,
                     const std::shared_ptr<Type> dtype)
                         : dimensions_(dimensions), dtype_(std::move(dtype)), attr_data_() {
  // Get a typed storage object based on dtype
  attr_data_.reset(createVarAttrStore(dtype_));

  // Set the size of the attribute value
  std::size_t numElements = std::accumulate(dimensions_.begin(), dimensions_.end(), (size_t)1,
                                            std::multiplies<std::size_t>());
  attr_data_->resize(numElements);
}

std::vector<std::size_t> Attribute::get_dimensions() const { return dimensions_; }

bool Attribute::isOfType(const Type & dtype) const {
  return (dtype == *dtype_);
}

std::shared_ptr<Attribute> Attribute::write(gsl::span<const char> data,
                                            const Type & dtype, const bool isFill) {
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

  if (dtype == *dtype_) {
    attr_data_->write(data, m_select, f_select, isFill);
  } else {
    // Convert the ObsStore data type into an HDF5 type equivalent.
    // Then, convert the data to the requested type and return.
    auto internal_type = dtype_->getHDF5Type();
    auto from_type = dtype.getHDF5Type();

    // Determine the necessary size for the converted data buffer. HDF5 uses
    // an in-place conversion strategy.
    size_t nelements = m_select.npoints();
    size_t min_sz_type = std::max(internal_type.getSize(), from_type.getSize());
    size_t buf_sz = nelements * min_sz_type;

    std::vector<char> conversion_buffer(buf_sz);
    std::copy(data.begin(), data.end(), conversion_buffer.begin());

    herr_t cvt_res = H5Tconvert(from_type.handle(), internal_type.handle(),
                                nelements, conversion_buffer.data(), nullptr, H5P_DEFAULT);
    if (cvt_res < 0) ioda::detail::Engines::HH::hdf5_error_check();

    attr_data_->write(gsl::span<const char>(conversion_buffer.data(), conversion_buffer.size()),
                      m_select, f_select, isFill);
  }

  return shared_from_this();
}

std::shared_ptr<Attribute> Attribute::read(gsl::span<char> data, const Type & dtype) {
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

  if (dtype == *dtype_) {
    attr_data_->read(data, m_select, f_select);
  } else {
    // Convert the ObsStore data type into an HDF5 type equivalent.
    // Then, convert the data to the requested type and return.
    auto internal_type = dtype_->getHDF5Type();
    auto to_type = dtype.getHDF5Type();

    // Determine the necessary size for the converted data buffer. HDF5 uses
    // an in-place conversion strategy.
    size_t nelements = m_select.npoints();
    size_t min_sz_type = std::max(internal_type.getSize(), to_type.getSize());
    size_t buf_sz = nelements * min_sz_type;

    std::vector<char> conversion_buffer(buf_sz);
    attr_data_->read(gsl::span<char>(conversion_buffer.data(), conversion_buffer.size()),
                     m_select, f_select);

    herr_t cvt_res = H5Tconvert(internal_type.handle(), to_type.handle(),
                                nelements, conversion_buffer.data(), nullptr, H5P_DEFAULT);
    if (cvt_res < 0) ioda::detail::Engines::HH::hdf5_error_check();
    std::copy_n(conversion_buffer.begin(), to_type.getSize() * nelements, data.begin());
  }


  return shared_from_this();
}

//*********************************************************************
//                        Has_Attributes function
//*********************************************************************
std::shared_ptr<Attribute> Has_Attributes::create(const std::string& name,
                                                  const std::shared_ptr<Type> & dtype,
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
