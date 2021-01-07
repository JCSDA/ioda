#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Has_Attributes.h
/// \brief Interfaces for ioda::Has_Attributes and related classes.

#include <gsl/gsl-lite.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ioda/Attributes/Attribute.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Misc/Eigen_Compat.h"
#include "ioda/Types/Type.h"
#include "ioda/defs.h"

namespace ioda {
class Has_Attributes;
namespace detail {
class Has_Attributes_Backend;
class Has_Attributes_Base;

/// \brief Describes the functions that can add attributes
/// \details Using the (Curiously Recurring Template
/// Pattern)[https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern] to implement this since we
/// want to use the same functions for placeholder attribute construction. \see Has_Attributes \see
/// Attribute_Creator
template <class DerivedHasAtts>
class CanAddAttributes {
public:
  /// @name Convenience functions for adding attributes
  /// @{

  /// \brief Create and write an Attribute, for arbitrary dimensions.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be created.
  /// \param data is a gsl::span (a pointer-length pair) that contains the data to be written.
  /// \param dimensions is an initializer list representing the size of the metadata. Each element is a
  /// dimension with a certain size. \param in_memory_dataType is the memory layout needed to parse data's
  /// type. \returns Another instance of this Has_Attribute. Used for operation chaining. \throws jedi::xError
  /// if data.size() does not match the number of total elements described by dimensions. \see gsl::span for
  /// details of how to make a span. \see gsl::make_span
  template <class DataType>
  DerivedHasAtts add(const std::string& attrname, ::gsl::span<const DataType> data,
                     const ::std::vector<Dimensions_t>& dimensions) {
    auto derivedThis = static_cast<DerivedHasAtts*>(this);
    auto att = derivedThis->template create<DataType>(attrname, dimensions);
    att.template write<DataType>(data);
    return *derivedThis;
  }

  /// \brief Create and write an Attribute, for arbitrary dimensions.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be created.
  /// \param data is an initializer list that contains the data to be written.
  /// \param dimensions is an initializer list representing the size of the metadata. Each element is a
  /// dimension with a certain size. \returns Another instance of this Has_Attribute. Used for operation
  /// chaining. \throws jedi::xError if data.size() does not match the number of total elements described by
  /// dimensions.
  template <class DataType>
  DerivedHasAtts add(const std::string& attrname, ::std::initializer_list<DataType> data,
                     const ::std::vector<Dimensions_t>& dimensions) {
    auto derivedThis = static_cast<DerivedHasAtts*>(this);
    auto att = derivedThis->template create<DataType>(attrname, dimensions);
    att.template write<DataType>(data);
    return *derivedThis;
  }

  /// \brief Create and write an Attribute, for a single-dimensional span of 1-D data.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be created.
  /// \param data is a gsl::span (a pointer-length pair) that contains the data to be written.
  ///   The new Attribute will be one-dimensional and the length of the overall span.
  /// \returns Another instance of this Has_Attribute. Used for operation chaining.
  /// \see gsl::span for details of how to make a span.
  /// \see gsl::make_span
  template <class DataType>
  DerivedHasAtts add(const std::string& attrname, ::gsl::span<const DataType> data) {
    return add(attrname, data, {gsl::narrow<Dimensions_t>(data.size())});
  }

  /// \brief Create and write an Attribute, for a 1-D initializer list.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be created.
  /// \param data is an initializer list that contains the data to be written.
  ///   The new Attribute will be one-dimensional and the length of the overall span.
  /// \returns Another instance of this Has_Attribute. Used for operation chaining.
  template <class DataType>
  DerivedHasAtts add(const std::string& attrname, ::std::initializer_list<DataType> data) {
    return add(attrname, data, {gsl::narrow<Dimensions_t>(data.size())});
  }

  /// \brief Create and write a single datum of an Attribute.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be created.
  /// \param data is the data to be written. The new Attribute will be zero-dimensional and will contain only
  /// this datum. \note Even 0-dimensional data have a type, which may be a compound array (i.e. a single
  /// string of variable length). \returns Another instance of this Has_Attribute. Used for operation
  /// chaining.
  template <class DataType>
  DerivedHasAtts add(const std::string& attrname, const DataType& data) {
    return add(attrname, gsl::make_span(&data, 1), {1});
  }

  template <class EigenClass>
  DerivedHasAtts addWithEigenRegular(const std::string& attrname, const EigenClass& data, bool is2D = true) {
#if 1  //__has_include("Eigen/Dense")
    typedef typename EigenClass::Scalar ScalarType;
    // If d is already in Row Major form, then this is optimized out.
    Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> dout;
    dout.resize(data.rows(), data.cols());
    dout = data;
    const auto& dconst = dout;  // To make some compilers happy.
    auto sp = gsl::make_span(dconst.data(), static_cast<int>(data.rows() * data.cols()));

    if (is2D)
      return add(attrname, sp,
                 {gsl::narrow<Dimensions_t>(data.rows()), gsl::narrow<Dimensions_t>(data.cols())});
    else
      return add(attrname, sp);
#else
    static_assert(false, "The Eigen headers cannot be found, so this function cannot be used.");
#endif
  }

  template <class EigenClass>
  DerivedHasAtts addWithEigenTensor(const std::string& attrname, const EigenClass& data) {
#if 1  //__has_include("unsupported/Eigen/CXX11/Tensor")
    typedef typename EigenClass::Scalar ScalarType;
    ioda::Dimensions dims = detail::EigenCompat::getTensorDimensions(data);

    auto derived_this = static_cast<DerivedHasAtts*>(this);
    Attribute att = derived_this->template create<ScalarType>(attrname, dims.dimsCur);

    att.writeWithEigenTensor(data);
    return *derived_this;
#else
    static_assert(false, "The Eigen unsupported/ headers cannot be found, so this function cannot be used.");
#endif
  }

  /// @}
};

/// \brief Describes the functions that can read attributes
/// \details Using the (Curiously Recurring Template
/// Pattern)[https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern] to implement this since we
/// want to use the same functions for placeholder attribute construction. \see Has_Attributes
template <class DerivedHasAtts>
class CanReadAttributes {
protected:
  CanReadAttributes() {}

public:
  virtual ~CanReadAttributes() {}

  /// @name Convenience functions for reading attributes
  /// @{

  /// \brief Open and read an Attribute, for expected dimensions.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be read.
  /// \param data is a pointer-size pair to the data buffer that is filled with the metadata's contents. It
  /// should be
  ///   pre-sized to accomodate all of the matadata. See getDimensions().numElements. Data will be filled
  ///   in row-major order.
  /// \throws jedi::xError on a size mismatch between Attribute dimensions and data.size().
  template <class DataType>
  const DerivedHasAtts read(const std::string& attrname, gsl::span<DataType> data) const {
    // Attribute att = this->open(attrname);
    Attribute att = static_cast<const DerivedHasAtts*>(this)->open(attrname);
    att.read(data);
    return *(static_cast<const DerivedHasAtts*>(this));
  }

  /// \brief Open and read an Attribute, with unknown dimensions.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be read.
  /// \param data is a vector acting as a data buffer that is filled with the metadata's contents. It gets
  /// resized as needed.
  ///   data will be filled in row-major order.
  template <class DataType>
  const DerivedHasAtts read(const std::string& attrname, std::vector<DataType>& data) const {
    Attribute att = static_cast<const DerivedHasAtts*>(this)->open(attrname);
    att.read(data);
    return *(static_cast<const DerivedHasAtts*>(this));
  }

  /// \brief Open and read an Attribute, with unknown dimensions.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be read.
  /// \param data is a valarray acting as a data buffer that is filled with the metadata's contents. It gets
  /// resized as needed.
  ///   data will be filled in row-major order.
  template <class DataType>
  const DerivedHasAtts read(const std::string& attrname, std::valarray<DataType>& data) const {
    Attribute att = static_cast<const DerivedHasAtts*>(this)->open(attrname);
    att.read(data);
    return *(static_cast<const DerivedHasAtts*>(this));
  }

  /// \brief Read a datum of an Attribute.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be read.
  /// \param data is a datum of type DataType.
  /// \throws jedi::xError if the underlying data have size > 1.
  template <class DataType>
  const DerivedHasAtts read(const std::string& attrname, DataType& data) const {
    Attribute att = static_cast<const DerivedHasAtts*>(this)->open(attrname);
    att.read<DataType>(data);
    return *(static_cast<const DerivedHasAtts*>(this));
  }

  /// \brief Read a datum of an Attribute.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute to be read.
  /// \returns A datum of type DataType.
  /// \throws jedi::xError if the underlying data have size > 1.
  template <class DataType>
  DataType read(const std::string& attrname) const {
    Attribute att = static_cast<const DerivedHasAtts*>(this)->open(attrname);
    return att.read<DataType>();
  }

  template <class EigenClass, bool Resize = detail::EigenCompat::CanResize<EigenClass>::value>
  DerivedHasAtts readWithEigenRegular(const std::string& attrname, EigenClass& data) {
#if 1  // __has_include("Eigen/Dense")
    Attribute att = static_cast<const DerivedHasAtts*>(this)->open(attrname);
    att.readWithEigenRegular<EigenClass, Resize>(data);
    return *(static_cast<const DerivedHasAtts*>(this));
#else
    static_assert(false, "The Eigen headers cannot be found, so this function cannot be used.")
#endif
  }

  template <class EigenClass>
  DerivedHasAtts readWithEigenTensor(const std::string& attrname, EigenClass& data) {
#if 1  //__has_include("unsupported/Eigen/CXX11/Tensor")
    Attribute att = static_cast<const DerivedHasAtts*>(this)->open(attrname);
    att.readWithEigenTensor<EigenClass>(data);
    return *(static_cast<const DerivedHasAtts*>(this));
#else
    static_assert(false, "The Eigen unsupported/ headers cannot be found, so this function cannot be used.");
#endif
  }

  /// @}
};

class IODA_DL Has_Attributes_Base {
private:
  /// Using an opaque object to implement the backend.
  std::shared_ptr<Has_Attributes_Backend> backend_;

protected:
  Has_Attributes_Base(std::shared_ptr<Has_Attributes_Backend>);

public:
  virtual ~Has_Attributes_Base();

  /// Query the backend and get the type provider.
  virtual detail::Type_Provider* getTypeProvider() const;

  /// @name General Functions
  /// @{
  ///

  /// List all attributes
  /// \returns an unordered vector of attribute names for an object.
  virtual std::vector<std::string> list() const;
  /// List all attributes
  /// \returns an unordered vector of attribute names for an object.
  inline std::vector<std::string> operator()() const { return list(); }

  /// \brief Does an Attribute with the specified name exist?
  /// \param attname is the name of the Attribute that we are looking for.
  /// \returns true if it exists.
  /// \returns false otherwise.
  virtual bool exists(const std::string& attname) const;
  /// \brief Delete an Attribute with the specified name.
  /// \param attname is the name of the Attribute that we are deleting.
  /// \throws jedi::xError if no such attribute exists.
  virtual void remove(const std::string& attname);
  /// \brief Open an Attribute by name
  /// \param name is the name of the Attribute to be opened.
  /// \returns An instance of an Attribute that can be queried (with getDimensions()) and read.
  virtual Attribute open(const std::string& name) const;
  /// \brief Open Attribute by name
  /// \param name is the name of the Attribute to be opened.
  /// \returns An instance of Attribute that can be queried (with getDimensions()) and read.
  inline Attribute operator[](const std::string& name) const { return open(name); }

  /// \brief Create an Attribute without setting its data.
  /// \param attrname is the name of the Attribute.
  /// \param dimensions is a vector representing the size of the metadata. Each element of the vector is a
  /// dimension with a certain size. \param in_memory_datatype is the runtime description of the Attribute's
  /// data type. \returns An instance of Attribute that can be written to.
  virtual Attribute create(const std::string& attrname, const Type& in_memory_dataType,
                           const std::vector<Dimensions_t>& dimensions = {1});

  /// Python compatability function
  inline Attribute _create_py(const std::string& attrname, BasicTypes dataType,
                              const std::vector<Dimensions_t>& dimensions = {1}) {
    return create(attrname, Type(dataType, getTypeProvider()), dimensions);
  }

  /// \brief Create an Attribute without setting its data.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param attrname is the name of the Attribute.
  /// \param dimensions is a vector representing the size of the metadata. Each element of the vector is a
  /// dimension with a certain size. \returns An instance of Attribute that can be written to.
  template <class DataType, class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Attribute create(const std::string& attrname, const std::vector<Dimensions_t>& dimensions = {1}) {
    Type in_memory_dataType = TypeWrapper::GetType(getTypeProvider());
    auto att = create(attrname, in_memory_dataType, dimensions);
    return att;
  }

  /// \brief Rename an Attribute
  /// \param oldName is the name of the Attribute to be changed.
  /// \param newName is the new name of the Attribute.
  /// \throws jedi::xError if oldName is not found.
  /// \throws jedi::xError if newName already exists.
  virtual void rename(const std::string& oldName, const std::string& newName);

  /// @}
};

class IODA_DL Has_Attributes_Backend : public Has_Attributes_Base {
protected:
  Has_Attributes_Backend();

public:
  virtual ~Has_Attributes_Backend();
};
}  // namespace detail

/** \brief This class exists inside of ioda::Group or ioda::Variable and provides the interface to
 *manipulating Attributes.
 *
 * \note It should only be constructed inside of a Group or Variable. It has no meaning elsewhere.
 * \see Attribute for the class that represents individual attributes.
 * \throws jedi::xError on all exceptions.
 **/
class IODA_DL Has_Attributes : public detail::CanAddAttributes<Has_Attributes>,
                               public detail::CanReadAttributes<Has_Attributes>,
                               public detail::Has_Attributes_Base {
public:
  Has_Attributes();
  Has_Attributes(std::shared_ptr<detail::Has_Attributes_Backend>);
  virtual ~Has_Attributes();
};

}  // namespace ioda
