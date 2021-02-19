#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Attribute_Creator.h
/// \brief Flywheel creation of ioda::Attribute. Used by ioda::Has_Attributes
///        and ioda::VariableCreationParameters.

#include <gsl/gsl-lite.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ioda/Attributes/Attribute.h"
#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Types/Type.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
/// \brief Flywheel creation of ioda::Attribute.
class IODA_DL Attribute_Creator_Base {
protected:
  std::string name_;

public:
  virtual ~Attribute_Creator_Base();
  virtual void apply(Has_Attributes& obj) const = 0;
  Attribute_Creator_Base(const std::string& name);
};
}  // namespace detail

/// \brief Flywheel creation of ioda::Attribute.
template <class DataType>
class Attribute_Creator : public detail::Attribute_Creator_Base {
private:
  ::std::vector<Dimensions_t> dimensions_;
  ::std::vector<DataType> data_;

public:
  virtual ~Attribute_Creator() {}
  void apply(Has_Attributes& obj) const override { obj.add<DataType>(name_, data_, dimensions_); }

  /// \details Totally taking advantage of vector constructors.
  template <class DataInput, class DimensionInput>
  Attribute_Creator(const std::string& name, DataInput data, DimensionInput dimensions)
      : Attribute_Creator_Base(name), data_(data.begin(), data.end()), dimensions_(dimensions) {}

  template <class DimensionInput>
  Attribute_Creator(const std::string& name, DimensionInput dimensions)
      : Attribute_Creator_Base(name), dimensions_(dimensions) {}

  template <class DataInput_junk_param = DataType>
  void write(const ::gsl::span<const DataType>& data) {
    data_ = ::std::vector<DataType>(data.begin(), data.end());
  }
};

/// \brief Flywheel creation of ioda::Attribute objects.
/// This is needed because you might want to make the same Attribute in multiple places.
class IODA_DL Attribute_Creator_Store : public detail::CanAddAttributes<Attribute_Creator_Store> {
  std::vector<std::shared_ptr<detail::Attribute_Creator_Base>> atts_;

public:
  Attribute_Creator_Store();
  virtual ~Attribute_Creator_Store();

  void apply(Has_Attributes& obj) const;

  /// @name Convenience functions for adding attributes
  /// @{
  /// \see CanAddAttributes
  /// \see CanAddAttributes::add

  template <class DataType>
  Attribute_Creator_Store& create(const std::string& attrname, ::gsl::span<const DataType> data,
                                  ::std::initializer_list<Dimensions_t> dimensions) {
    atts_.push_back(std::make_shared<Attribute_Creator<DataType>>(attrname, data, dimensions));
    return *this;
  }

  template <class DataType>
  Attribute_Creator_Store& create(const std::string& attrname,
                                  ::std::initializer_list<DataType> data,
                                  ::std::initializer_list<Dimensions_t> dimensions) {
    atts_.push_back(std::make_shared<Attribute_Creator<DataType>>(attrname, data, dimensions));
    return *this;
  }

  template <class DataType2>
  struct AttWrapper {
    std::shared_ptr<Attribute_Creator<DataType2>> inner;
    AttWrapper(std::shared_ptr<Attribute_Creator<DataType2>> d) : inner(d) {}
    template <class DataInput_junk_param = DataType2>
    void write(const ::gsl::span<const DataType2>& data) {
      inner->write(data);
    }
  };

  template <class DataType>
  AttWrapper<DataType> create(const std::string& attrname, ::std::vector<Dimensions_t> dimensions) {
    auto res = std::make_shared<Attribute_Creator<DataType>>(attrname, dimensions);
    atts_.push_back(res);
    return AttWrapper<DataType>{res};
  }

  /// @}
};

}  // namespace ioda
