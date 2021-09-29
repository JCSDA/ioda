#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_attribute Attributes and Has_Attributes
 * \brief Ancillary data attached to variables and groups.
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file Attribute.h
 * \brief @link ioda_cxx_attribute Interfaces @endlink for ioda::Attribute and related classes.
 */
#include <functional>
#include <gsl/gsl-lite.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <valarray>
#include <vector>

#include "ioda/Exception.h"
#include "ioda/Misc/Dimensions.h"
#include "ioda/Misc/Eigen_Compat.h"
#include "ioda/Python/Att_ext.h"
#include "ioda/Types/Marshalling.h"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/defs.h"

namespace ioda {
class Attribute;

namespace detail {
class Attribute_Backend;
class Variable_Backend;

/// \brief Base class for Attributes
/// \ingroup ioda_cxx_attribute
///
/// \details You might wonder why we have this class
/// as a template. This is because we are using
/// a bit of compile-time template polymorphism to return
/// Attribute objects from base classes before Attribute is fully declared.
/// This is a variation of the (Curiously Recurring Template
/// Pattern)[https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern].
template <class Attribute_Implementation = Attribute>
class Attribute_Base {
protected:
  /// Using an opaque object to implement the backend.
  std::shared_ptr<Attribute_Backend> backend_;

  // Variable_Backend's derived classes do occasionally need access to the Attribute_Backend object.
  friend class Variable_Backend;

  /// @name General Functions
  /// @{

  Attribute_Base(std::shared_ptr<Attribute_Backend>);

public:
  virtual ~Attribute_Base();

  /// @}

  /// @name Writing Data
  /// \note Writing metadata is an all-or-nothing-process, unlike writing
  /// segments of data to a variable.
  /// \note Dimensions are fixed. Attribute are not resizable.
  /// @{
  ///

  /// \brief The fundamental write function. Backends overload this function to implement all write
  /// operations.
  ///
  /// \details This function writes a span of bytes (characters) to the backend attribute storage.
  /// No type conversions take place here (see the templated conversion function, below).
  ///
  /// \param data is a span of data.
  /// \param in_memory_datatype is an opaque (backend-level) object that describes the placement of
  ///   the data in memory. Usually ignorable - needed for complex data structures.
  /// \throws ioda::Exception if data has the wrong size.
  /// \returns The attribute (for chaining).
  virtual Attribute_Implementation write(gsl::span<const char> data, const Type& type);

  /// \brief Write data.
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \tparam Marshaller is the class that serializes the data type into something that the backend
  /// library can use.
  /// \tparam TypeWrapper is a helper class that creates Type objects for the backend.
  /// \param data is a gsl::span (a pointer-length pair) that contains the data to be written.
  /// \param in_memory_dataType is the memory layout needed to parse data's type.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if data.size() does not match getDimensions().numElements.
  /// \see gsl::span for details of how to make a span.
  /// \see gsl::make_span
  template <class DataType, class Marshaller = ioda::Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Attribute_Implementation write(gsl::span<const DataType> data) {
    try {
      Marshaller m;
      auto d   = m.serialize(data);
      auto spn = gsl::make_span<const char>(reinterpret_cast<const char*>(d->DataPointers.data()),
                                            d->DataPointers.size() * Marshaller::bytesPerElement_);
      write(spn, TypeWrapper::GetType(getTypeProvider()));
      return Attribute_Implementation{backend_};
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Write data
  /// \note Normally the gsl::span write is fine. This one exists for easy Python binding.
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \tparam Marshaller is the class that serializes the data type into something that the backend
  /// library can use.
  /// \tparam TypeWrapper is a helper class that creates Type objects for the backend.
  /// \param data is a std::vector that contains the data to be written.
  /// \param in_memory_dataType is the memory layout needed to parse data's type.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if data.size() does not match getDimensions().numElements.
  /// \see gsl::span for details of how to make a span.
  /// \see gsl::make_span for details on how to make a span.
  template <class DataType, class Marshaller = ioda::Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Attribute_Implementation write(const std::vector<DataType>& data) {
    std::vector<DataType> vd = data;
    return this->write<DataType, Marshaller, TypeWrapper>(gsl::make_span(vd));
  }

  /// \brief Write data.
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param data is an initializer list that contains the data to be written.
  /// \param in_memory_dataType is the memory layout needed to parse data's type.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if data.size() does not match getDimensions().numElements.
  /// \see gsl::span for details of how to make a span.
  template <class DataType>
  Attribute_Implementation write(std::initializer_list<DataType> data) {
    std::vector<DataType> v(data);
    return this->write<DataType>(gsl::make_span(v));
  }

  /// \brief Write a datum.
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param data is the data to be written.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if the Attribute dimensions are larger than a single point.
  template <class DataType>  //, class Marshaller = HH::Types::Object_Accessor<DataType> >
  Attribute_Implementation write(DataType data) {
    try {
      if (getDimensions().numElements != 1)
        throw Exception("Wrong number of elements. Use a different write() method.", ioda_Here());
      return write<DataType>(gsl::make_span<DataType>(&data, 1));
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Write an Eigen object (a Matrix, an Array, a Block, a Map).
  /// \tparam EigenClass is the Eigen object to write.
  /// \param d is the data to be written.
  /// \throws ioda::Exception on a dimension mismatch.
  /// \returns the attribute
  template <class EigenClass>
  Attribute_Implementation writeWithEigenRegular(const EigenClass& d) {
#if 1  //__has_include("Eigen/Dense")
    try {
      typedef typename EigenClass::Scalar ScalarType;
      // If d is already in Row Major form, then this is optimized out.
      Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> dout;
      dout.resize(d.rows(), d.cols());
      dout               = d;
      const auto& dconst = dout;  // To make some compilers happy.
      auto sp            = gsl::make_span(dconst.data(), static_cast<int>(d.rows() * d.cols()));

      return write<ScalarType>(sp);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
#else
    static_assert(false, "The Eigen headers cannot be found, so this function cannot be used.");
#endif
  }

  /// \brief Write an Eigen Tensor-like object
  /// \tparam EigenClass is the Eigen tensor to write.
  /// \param d is the data to be written.
  /// \throws ioda::Exception on a dimension mismatch.
  /// \returns the attribute
  template <class EigenClass>
  Attribute_Implementation writeWithEigenTensor(const EigenClass& d) {
#if 1  //__has_include("unsupported/Eigen/CXX11/Tensor")
    try {
      ioda::Dimensions dims = detail::EigenCompat::getTensorDimensions(d);

      auto sp  = (gsl::make_span(d.data(), dims.numElements));
      auto res = write(sp);
      return res;
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
#else
    static_assert(
      false, "The Eigen unsupported/ headers cannot be found, so this function cannot be used.");
#endif
  }

  /// @}
  /// @name Reading Data
  /// @{
  ///

  /// \brief The fundamental read function. Backends overload this function to implement all read
  /// operations.
  ///
  /// \details This function reads in a span of characters from the backend attribute storage.
  /// No type conversions take place here (see the templated conversion function, below).
  ///
  /// \param data is a span of data that has length of getStorageSize().
  /// \param in_memory_datatype is an opaque (backend-level) object that describes the placement of
  ///   the data in memory. Usually ignorable - needed for complex data structures.
  /// \throws ioda::Exception if data has the wrong size.
  /// \returns The attribute (for chaining).
  virtual Attribute_Implementation read(gsl::span<char> data, const Type& in_memory_dataType) const;

  /// \brief Read data.
  ///
  /// \details This is a fundamental function that reads a span of characters from backend storage,
  /// and then performs the appropriate type conversion / deserialization into objects in data.
  ///
  /// \tparam DataType is the type if the data to be read. I.e. float, int, int32_t, uint16_t,
  /// std::string, etc.
  /// \tparam Marshaller is the class that performs the deserialization operation.
  /// \tparam TypeWrapper is a helper class that passes Type information to the backend.
  /// \param data is a pointer-size pair to the data buffer that is filled with the metadata's
  /// contents. It should be
  ///   pre-sized to accomodate all of the matadata. See getDimensions().numElements. data will be
  ///   filled in row-major order.
  /// \param in_memory_datatype is an opaque (backend-level) object that describes the placement of
  ///   the data in memory. Usually this does not need to be set to anything other than its default
  ///   value. Kept as a parameter for debugging purposes.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if data.size() != getDimensions().numElements.
  /// \see getDimensions for buffer size information.
  template <class DataType, class Marshaller = ioda::Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Attribute_Implementation read(gsl::span<DataType> data) const {
    try {
      const size_t numObjects = data.size();
      if (getDimensions().numElements != gsl::narrow<ioda::Dimensions_t>(numObjects))
        throw Exception("Size mismatch between underlying object and user-provided data range.",
                        ioda_Here());

      detail::PointerOwner pointerOwner = getTypeProvider()->getReturnedPointerOwner();
      Marshaller m(pointerOwner);
      auto p = m.prep_deserialize(numObjects);
      read(gsl::make_span<char>(reinterpret_cast<char*>(p->DataPointers.data()),
                                p->DataPointers.size() * Marshaller::bytesPerElement_),
           TypeWrapper::GetType(getTypeProvider()));
      m.deserialize(p, data);

      return Attribute_Implementation{backend_};
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Vector read convenience function.
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param data is a vector acting as a data buffer that is filled with the metadata's contents.
  ///   It gets resized as needed.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \note data will be stored in row-major order.
  template <class DataType>
  Attribute_Implementation read(std::vector<DataType>& data) const {
    data.resize(getDimensions().numElements);
    return read<DataType>(gsl::make_span<DataType>(data.data(), data.size()));
  }

  /// \brief Valarray read convenience function.
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param data is a valarray acting as a data buffer that is filled with the metadata's contents.
  ///   It gets resized as needed.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \note data will be stored in row-major order.
  template <class DataType>
  Attribute_Implementation read(std::valarray<DataType>& data) const {
    data.resize(getDimensions().numElements);
    return read<DataType>(gsl::make_span<DataType>(std::begin(data), std::end(data)));
  }

  /// \brief Read into a single value (convenience function).
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \param data is where the datum is read to.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if the underlying data have multiple elements.
  template <class DataType>
  Attribute_Implementation read(DataType& data) const {
    try {
      if (getDimensions().numElements != 1)
        throw Exception("Wrong number of elements. Use a different read() method.", ioda_Here());
      return read<DataType>(gsl::make_span<DataType>(&data, 1));
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Read a single value (convenience function).
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \returns A datum of type DataType.
  /// \throws ioda::Exception if the underlying data have size greater than 1.
  /// \note The Python function is read_datum_*
  template <class DataType>
  DataType read() const {
    DataType ret;
    read<DataType>(ret);
    return ret;
  }

  /// \brief Read into a new vector. Python convenience function.
  /// \tparam DataType is the type of the data.
  /// \note The Python function is read_list_*
  template <class DataType>
  std::vector<DataType> readAsVector() const {
    std::vector<DataType> data(getDimensions().numElements);
    read<DataType>(gsl::make_span<DataType>(data.data(), data.size()));
    return data;
  }

  /// \brief Read data into an Eigen::Array, Eigen::Matrix, Eigen::Map, etc.
  /// \tparam EigenClass is a template pointing to the Eigen object.
  ///   This template must provide the EigenClass::Scalar typedef.
  /// \tparam Resize indicates whether the Eigen object should be resized
  ///   if there is a dimension mismatch. Not all Eigen objects can be resized.
  /// \param res is the Eigen object.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if the attribute's dimensionality is
  ///   too high.
  /// \throws ioda::Exception if resize = false and there is a dimension mismatch.
  /// \note When reading in a 1-D object, the data are read as a column vector.
  template <class EigenClass, bool Resize = detail::EigenCompat::CanResize<EigenClass>::value>
  Attribute_Implementation readWithEigenRegular(EigenClass& res) const {
#if 1  //__has_include("Eigen/Dense")
    try {
      typedef typename EigenClass::Scalar ScalarType;

      static_assert(
        !(Resize && !detail::EigenCompat::CanResize<EigenClass>::value),
        "This object cannot be resized, but you have specified that a resize is required.");

      // Check that the dimensionality is 1 or 2.
      const auto dims = getDimensions();
      if (dims.dimensionality > 2)
        throw Exception(
          "Dimensionality too high for a regular Eigen read. Use "
          "Eigen::Tensor reads instead.",
          ioda_Here());

      int nDims[2] = {1, 1};
      if (dims.dimsCur.size() >= 1) nDims[0] = gsl::narrow<int>(dims.dimsCur[0]);
      if (dims.dimsCur.size() >= 2) nDims[1] = gsl::narrow<int>(dims.dimsCur[1]);

      // Resize if needed.
      if (Resize)
        detail::EigenCompat::DoEigenResize(res, nDims[0],
                                           nDims[1]);  // nullop if the size is already correct.
      else if (dims.numElements != (size_t)(res.rows() * res.cols()))
        throw Exception("Size mismatch", ioda_Here());

      // Array copy to preserve row vs column major format.
      // Should be optimized away by the compiler if unneeded.
      // Note to the reader: We are reading in the data to a temporary object.
      // We can size _this_ temporary object however we want.
      // The temporary is used to swap row / column indices if needed.
      // It should be optimized away if not needed... making sure this happens is a todo.
      Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> data_in(res.rows(),
                                                                                        res.cols());

      auto ret = read<ScalarType>(gsl::span<ScalarType>(data_in.data(), dims.numElements));
      res      = data_in;
      return ret;
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
#else
    static_assert(false, "The Eigen headers cannot be found, so this function cannot be used.");
#endif
  }

  /// \brief Read data into an Eigen::Array, Eigen::Matrix, Eigen::Map, etc.
  /// \tparam EigenClass is a template pointing to the Eigen object.
  ///   This template must provide the EigenClass::Scalar typedef.
  /// \param res is the Eigen object.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \throws ioda::Exception if there is a size mismatch.
  /// \note When reading in a 1-D object, the data are read as a column vector.
  template <class EigenClass>
  Attribute_Implementation readWithEigenTensor(EigenClass& res) const {
#if 1  //__has_include("unsupported/Eigen/CXX11/Tensor")
    try {
      // Check dimensionality of source and destination
      const auto ioda_dims  = getDimensions();
      const auto eigen_dims = ioda::detail::EigenCompat::getTensorDimensions(res);
      if (ioda_dims.numElements != eigen_dims.numElements)
        throw Exception("Size mismatch for Eigen Tensor-like read.", ioda_Here());

      auto sp = (gsl::make_span(res.data(), eigen_dims.numElements));
      return read(sp);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
#else
    static_assert(
      false, "The Eigen unsupported/ headers cannot be found, so this function cannot be used.");
#endif
  }

  /// \internal Python binding function
  template <class EigenClass>
  EigenClass _readWithEigenRegular_python() const {
    EigenClass data;
    readWithEigenRegular(data);
    return data;
  }

  /// @}
  /// @name Type-querying Functions
  /// @{

  /// \brief Get Attribute type.
  virtual Type getType() const;
  /// \brief Get Attribute type.
  inline Type type() const { return getType(); }

  /// Query the backend and get the type provider.
  virtual detail::Type_Provider* getTypeProvider() const;

  /// \brief Convenience function to check an Attribute's storage type.
  /// \tparam DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
  /// \returns True if the type matches
  /// \returns False (0) if the type does not match
  /// \throws ioda::Exception if an error occurred.
  template <class DataType>
  bool isA() const {
    Type templateType = Types::GetType_Wrapper<DataType>::GetType(getTypeProvider());

    return isA(templateType);
  }
  /// Hand-off to the backend to check equivalence
  virtual bool isA(Type lhs) const;

  /// Python compatability function
  inline bool isA(BasicTypes dataType) { return isA(Type(dataType, getTypeProvider())); }
  /// \internal pybind11
  inline bool _py_isA2(BasicTypes dataType) { return isA(dataType); }

  /// @}
  /// @name Data Space-Querying Functions
  /// @{

  // Get an Attribute's dataspace
  // virtual Encapsulated_Handle getSpace() const;

  /// \brief Get Attribute's dimensions.
  virtual Dimensions getDimensions() const;

  /// @}
};

// extern template class Attribute_Base<>;
}  // namespace detail

/** \brief This class represents attributes, which may be attached to both Variables and Groups.
 *  \ingroup ioda_cxx_attribute
 *
 * Attributes are used to store small objects that get tagged to a Variable or a
 * Group to provide context to users and other programs. Attributes include
 * descriptions, units, alternate names, dimensions, and similar constructs.
 * Attributes may have different types (ints, floats, datetimes, strings, etc.),
 * and may be 0- or 1-dimensional.
 *
 * We can open an Attribute from a Has_Attribute object, which is a member of Groups and
 * Variables.
 *
 * \note Multidimensional attributes are supported by some of the underlying backends,
 * like HDF5, but are incompatible with the NetCDF file format.
 * \see Has_Attribute for the class that can create and open new Attribute objects.
 * \throws ioda::Exception on all exceptions.
 **/
class IODA_DL Attribute : public detail::Attribute_Base<> {
public:
  Attribute();
  Attribute(std::shared_ptr<detail::Attribute_Backend> b);
  Attribute(const Attribute&);
  Attribute& operator=(const Attribute&);
  virtual ~Attribute();

  /// @name Python compatability objects
  /// @{

  detail::python_bindings::AttributeIsA<Attribute> _py_isA;

  detail::python_bindings::AttributeReadSingle<Attribute> _py_readSingle;
  detail::python_bindings::AttributeReadVector<Attribute> _py_readVector;
  detail::python_bindings::AttributeReadNPArray<Attribute> _py_readNPArray;

  detail::python_bindings::AttributeWriteSingle<Attribute> _py_writeSingle;
  detail::python_bindings::AttributeWriteVector<Attribute> _py_writeVector;
  detail::python_bindings::AttributeWriteNPArray<Attribute> _py_writeNPArray;

  /// @}
};

namespace detail {
/// \brief Attribute backends inherit from this.
class IODA_DL Attribute_Backend : public Attribute_Base<> {
public:
  virtual ~Attribute_Backend();

protected:
  Attribute_Backend();
};
}  // namespace detail
}  // namespace ioda

/// @} // End Doxygen block
