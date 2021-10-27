#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_variable Variables, Data Access, and Selections
 * \brief The main data storage methods and objects in IODA.
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file Variable.h
 * \brief @link ioda_cxx_variable Interfaces @endlink for ioda::Variable and related classes.
 */
#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/Exception.h"
#include "ioda/Misc/Eigen_Compat.h"
#include "ioda/Python/Var_ext.h"
#include "ioda/Types/Marshalling.h"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/Variables/Fill.h"
#include "ioda/Variables/Selection.h"
#include "ioda/defs.h"

namespace ioda {
class Attribute;
class Variable;
struct VariableCreationParameters;

struct Named_Variable;

namespace detail {
class Attribute_Backend;
class Variable_Backend;

/// \brief Exists to prevent constructor conflicts when passing a backend into
///   a frontend object.
/// \ingroup ioda_cxx_variable
/// \bug Need to add a virtual resize function.
template <class Variable_Implementation = Variable>
class Variable_Base {
protected:
  /// Using an opaque object to implement the backend.
  std::shared_ptr<Variable_Backend> backend_;

  /// @name General Functions
  /// @{

  Variable_Base(std::shared_ptr<Variable_Backend>);

public:
  virtual ~Variable_Base();

  /// @}
  /// @name Metadata manipulation
  /// @{

  /// Attributes
  Has_Attributes atts;

  /// @}

  /// @name General Functions
  /// @{

  /// Gets a handle to the underlying object that implements the backend functionality.
  std::shared_ptr<Variable_Backend> get() const;

  /// @}
  /// @name Type-querying Functions
  /// @{

  /// Get type
  virtual Type getType() const;
  /// Get type
  inline Type type() const { return getType(); }

  /// Query the backend and get the type provider.
  virtual detail::Type_Provider* getTypeProvider() const;

  /// \brief Convenience function to check a Variable's storage type.
  /// \param DataType is the type of the data. I.e. float, int, int32_t, uint16_t, std::string, etc.
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
  inline bool isA(BasicTypes dataType) const { return isA(Type(dataType, getTypeProvider())); }
  /// \internal pybind11
  inline bool _py_isA2(BasicTypes dataType) { return isA(dataType); }
  /// Convenience function to query type
  BasicTypes getBasicType() const;

  /// @}
  /// @name Querying Functions for Fill Values, Chunking and Compression
  /// @{

  /// @brief Convenience function to get fill value, attributes,
  ///   chunk sizes, and compression in a collective call.
  /// @details This function has better performance on some engines
  ///   for bulk operations than calling separately.
  /// @param doAtts includes attributes in the creation parameters.
  /// @param doDims includes dimension scales in the creation parameters.
  /// Although dimensions are attributes on some backends, we treat them
  /// separately in this function.
  /// @returns the filled-in VariableCreationParameters object
  virtual VariableCreationParameters getCreationParameters(bool doAtts = true,
                                                           bool doDims = true) const;

  /// \brief Check if a variable has a fill value set
  /// \returns true if a fill value is set, false otherwise.
  virtual bool hasFillValue() const;

  /// Remap fill value storage type into this class.
  typedef detail::FillValueData_t FillValueData_t;

  /// \brief Retrieve the fill value
  /// \returns an object of type FillValueData_t that stores
  ///   the fill value. If there is no fill value, then
  ///   FillValueData_t::set_ will equal false. The fill
  ///   value data will be stored in
  ///   FillValueData_t::fillValue_ (for simple types), or in
  ///   FillValueData_t::stringFillValue_ (for strings only).
  /// \note Recommend querying isA to make sure that you
  ///   are reading the fill value as the correct type.
  virtual FillValueData_t getFillValue() const;

  /// \brief Retrieve the chunking options for the Variable.
  /// \note Not all backends support chunking, but they should
  ///   all store the desired chunk size information in case the
  ///   Variable is copied to a new backend.
  /// \returns a vector containing the chunk sizes.
  /// \returns an empty vector if chunking is not used.
  virtual std::vector<Dimensions_t> getChunkSizes() const;

  /// \brief Retrieve the GZIP compression options for the Variable.
  /// \note Not all backends support compression (and those that
  ///   do also require chunking support). They should store this
  ///   information anyways, in case the Variable is copied to
  ///   a new backend.
  /// \returns a pair indicating 1) whether GZIP is requested and
  ///   2) the compression level that is desired.
  virtual std::pair<bool, int> getGZIPCompression() const;

  /// \brief Retrieve the SZIP compression options for the Variable.
  /// \note Not all backends support compression (and those that
  ///   do also require chunking support). They should store this
  ///   information anyways, in case the Variable is copied to
  ///   a new backend.
  /// \returns a tuple indicating 1) whether SZIP is requested,
  ///   2) PixelsPerBlock, and 3) general SZIP filter option flags.
  virtual std::tuple<bool, unsigned, unsigned> getSZIPCompression() const;

  /// @}
  /// @name Data Space-Querying Functions
  /// @{

  // Get dataspace
  // JEDI_NODISCARD DataSpace getSpace() const;

  /// Get current and maximum dimensions, and number of total points.
  /// \note In Python, see the dims property.
  virtual Dimensions getDimensions() const;

  /// \brief Resize the variable.
  /// \note Not all variables are resizable. This depends
  ///   on backend support. For HDF5, the variable must be chunked
  ///   and must not exceed max_dims.
  /// \note Bad things may happen if a variable's dimension scales
  ///   have different lengths than its dimensions. Resize them
  ///   together, preferably using the ObsSpace resize function.
  /// \see ObsSpace::resize
  /// \param newDims are the new dimensions.
  virtual Variable resize(const std::vector<Dimensions_t>& newDims);

  /// Attach a dimension scale to this Variable.
  virtual Variable attachDimensionScale(unsigned int DimensionNumber, const Variable& scale);
  /// Detach a dimension scale
  virtual Variable detachDimensionScale(unsigned int DimensionNumber, const Variable& scale);
  /// Set dimensions (convenience function to several invocations of attachDimensionScale).
  Variable setDimScale(const std::vector<Variable>& dims);
  /// Set dimensions (convenience function to several invocations of attachDimensionScale).
  Variable setDimScale(const std::vector<Named_Variable>& dims);
  /// Set dimensions (convenience function to several invocations of attachDimensionScale).
  Variable setDimScale(const Variable& dims);
  /// Set dimensions (convenience function to several invocations of attachDimensionScale).
  Variable setDimScale(const Variable& dim1, const Variable& dim2);
  /// Set dimensions (convenience function to several invocations of attachDimensionScale).
  Variable setDimScale(const Variable& dim1, const Variable& dim2, const Variable& dim3);

  /// Is this Variable used as a dimension scale?
  virtual bool isDimensionScale() const;

  /// Designate this table as a dimension scale
  virtual Variable setIsDimensionScale(const std::string& dimensionScaleName);
  /// Get the name of this Variable's defined dimension scale
  inline std::string getDimensionScaleName() const {
    std::string r;
    getDimensionScaleName(r);
    return r;
  }
  virtual Variable getDimensionScaleName(std::string& res) const;

  /// Is a dimension scale attached to this Variable in a certain position?
  virtual bool isDimensionScaleAttached(unsigned int DimensionNumber, const Variable& scale) const;

  /// \brief Which dimensions are attached at which positions? This function may offer improved
  /// performance on some backends compared to serial isDimensionScaleAttached calls.
  /// \param scalesToQueryAgainst is a vector containing the scales. You can pass in
  ///   "tagged" strings that map the Variable to a name.
  ///   If you do not pass in a scale to check against, then this scale will not be checked and
  ///   will not be present in the function output.
  /// \param firstOnly is specified when only one dimension can be attached to each axis (the
  /// default).
  /// \returns a vector with the same length as the variable's dimensionality.
  ///   Each variable dimension in the vector can have one or more attached scales. These scales are
  ///   returned as their own, inner, vector of pair<string, Variable>.
  virtual std::vector<std::vector<Named_Variable>> getDimensionScaleMappings(
    const std::list<Named_Variable>& scalesToQueryAgainst,
    bool firstOnly = true) const;

  /// @}
  /// @name Writing Data
  /// @{

  /// \brief The fundamental write function. Backends overload this function to implement
  ///   all write operations.
  ///
  /// \details This function writes a span of bytes (characters) to the backend attribute
  ///   storage. No type conversions take place here (see the templated conversion function,
  ///   below).
  ///
  /// \param data is a span of data.
  /// \param in_memory_datatype is an opaque (backend-level) object that describes the
  ///   placement of the data in memory. Usually ignorable - needed for complex data structures.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \throws ioda::xError if data has the wrong size.
  /// \returns The variable (for chaining).
  virtual Variable write(gsl::span<const char> data, const Type& in_memory_dataType,
                         const Selection& mem_selection  = Selection::all,
                         const Selection& file_selection = Selection::all);

  /// \brief Write the Variable
  /// \note Ensure that the correct dimension ordering is preserved.
  /// \note With default parameters, the entire Variable is written.
  /// \tparam DataType is the type of the data to be written.
  /// \tparam Marshaller is a class that serializes / deserializes data.
  /// \tparam TypeWrapper translates DataType into a form that the backend understands.
  /// \param data is a span of data.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \throws ioda::xError if data has the wrong size.
  /// \returns The variable (for chaining).
  template <class DataType, class Marshaller = Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Variable_Implementation write(const gsl::span<DataType> data,
                                const Selection& mem_selection  = Selection::all,
                                const Selection& file_selection = Selection::all) {
    try {
      Marshaller m;
      auto d = m.serialize(data);
      return write(gsl::make_span<const char>(
                      reinterpret_cast<const char*>(d->DataPointers.data()),
                     d->DataPointers.size() * Marshaller::bytesPerElement_),
                   TypeWrapper::GetType(getTypeProvider()), mem_selection, file_selection);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Write the Variable
  /// \note Ensure that the correct dimension ordering is preserved.
  /// \note With default parameters, the entire Variable is written.
  /// \tparam DataType is the type of the data to be written.
  /// \tparam Marshaller is a class that serializes / deserializes data.
  /// \tparam TypeWrapper translates DataType into a form that the backend understands.
  /// \param data is a span of data.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \throws ioda::xError if data has the wrong size.
  /// \returns The variable (for chaining).
  template <class DataType, class Marshaller = Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Variable_Implementation write(const gsl::span<const DataType> data,
                                const Selection& mem_selection  = Selection::all,
                                const Selection& file_selection = Selection::all) {
    try {
      Marshaller m;
      auto d = m.serialize(data);
      return write(gsl::make_span<const char>(
                      reinterpret_cast<const char*>(d->DataPointers.data()),
                     d->DataPointers.size() * Marshaller::bytesPerElement_),
                   TypeWrapper::GetType(getTypeProvider()), mem_selection, file_selection);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Write the variable
  /// \tparam DataType is the type of the data to be written.
  /// \tparam Marshaller is a class that serializes / deserializes data.
  /// \tparam TypeWrapper translates DataType into a form that the backend understands.
  /// \param data is a span of data.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \throws ioda::xError if data has the wrong size.
  /// \returns The variable (for chaining).

  template <class DataType, class Marshaller = Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Variable_Implementation write(const std::vector<DataType>& data,
                                const Selection& mem_selection  = Selection::all,
                                const Selection& file_selection = Selection::all) {
    try {
      return this->write<DataType, Marshaller, TypeWrapper>(gsl::make_span(data), mem_selection,
                                                            file_selection);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Write an Eigen object (a Matrix, an Array, a Block, a Map).
  /// \tparam EigenClass is the type of the Eigen object being written.
  /// \param d is the data to be written.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \throws ioda::xError on a dimension mismatch.
  /// \returns the variable
  template <class EigenClass>
  Variable_Implementation writeWithEigenRegular(const EigenClass& d,
                                                const Selection& mem_selection  = Selection::all,
                                                const Selection& file_selection = Selection::all) {
#if 1  //__has_include("Eigen/Dense")
    try {
      typedef typename EigenClass::Scalar ScalarType;
      // If d is already in Row Major form, then this is optimized out.
      Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> dout;
      dout.resize(d.rows(), d.cols());
      dout               = d;
      const auto& dconst = dout;  // To make some compilers happy.
      auto sp            = gsl::make_span(dconst.data(), static_cast<int>(d.rows() * d.cols()));

      return write<ScalarType>(sp, mem_selection, file_selection);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
#else
    static_assert(false, "The Eigen headers cannot be found, so this function cannot be used.");
#endif
  }

  /// \brief Write an Eigen Tensor-like object
  /// \tparam EigenClass is the type of the Eigen object being written.
  /// \param d is the data to be written.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \throws ioda::xError on a dimension mismatch.
  /// \returns the variable
  template <class EigenClass>
  Variable_Implementation writeWithEigenTensor(const EigenClass& d,
                                               const Selection& mem_selection  = Selection::all,
                                               const Selection& file_selection = Selection::all) {
#if 1  //__has_include("unsupported/Eigen/CXX11/Tensor")
    try {
      ioda::Dimensions dims = detail::EigenCompat::getTensorDimensions(d);

      auto sp  = (gsl::make_span(d.data(), dims.numElements));
      auto res = write(sp, mem_selection, file_selection);
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

  /// \brief Read the Variable - as char array. Ordering is row-major.
  /// \details This is the fundamental read function that has to be implemented.
  /// \param data is a byte-array that will hold the read data.
  /// \param in_memory_dataType describes how ioda should arrange the read data in memory.
  ///   As floats? As doubles? Strings?
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \note Ensure that the correct dimension ordering is preserved
  /// \note With default parameters, the entire Variable is read
  virtual Variable read(gsl::span<char> data, const Type& in_memory_dataType,
                        const Selection& mem_selection  = Selection::all,
                        const Selection& file_selection = Selection::all) const;

  /// \brief Read the variable into a span (range) or memory. Ordering is row-major.
  /// \tparam DataType is the type of the data to be written.
  /// \tparam Marshaller is a class that serializes / deserializes data.
  /// \tparam TypeWrapper translates DataType into a form that the backend understands.
  /// \param data is a byte-array that will hold the read data.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \todo Add in the dataspaces!
  template <class DataType, class Marshaller = ioda::Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Variable_Implementation read(gsl::span<DataType> data,
                               const Selection& mem_selection  = Selection::all,
                               const Selection& file_selection = Selection::all) const {
    try {
      const size_t numObjects = data.size();

      detail::PointerOwner pointerOwner = getTypeProvider()->getReturnedPointerOwner();
      Marshaller m(pointerOwner);
      auto p = m.prep_deserialize(numObjects);
      read(gsl::make_span<char>(
             reinterpret_cast<char*>(p->DataPointers.data()),
             // Logic note: sizeof mutable data type. If we are
             // reading in a string, then mutable data type is char*,
             // which works because address pointers have the same size.
             p->DataPointers.size() * Marshaller::bytesPerElement_),
           TypeWrapper::GetType(getTypeProvider()), mem_selection, file_selection);
      m.deserialize(p, data);

      return Variable_Implementation{backend_};
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Read the variable into a vector. Resize if needed. For a non-resizing
  ///   version, use a gsl::span.
  /// \details Ordering is row-major.
  /// \tparam DataType is the type of the data to be written.
  /// \tparam Marshaller is a class that serializes / deserializes data.
  /// \tparam TypeWrapper translates DataType into a form that the backend understands.
  /// \param data is a byte-array that will hold the read data.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \bug Resize only if needed, and resize to the proper extent depending on
  ///   mem_selection and file_selection.
  template <class DataType, class Marshaller = ioda::Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Variable_Implementation read(std::vector<DataType>& data,
                               const Selection& mem_selection  = Selection::all,
                               const Selection& file_selection = Selection::all) const {
    data.resize(getDimensions().numElements); // TODO(Ryan): remove
    return read<DataType, Marshaller, TypeWrapper>(gsl::make_span(data.data(), data.size()),
                                                   mem_selection, file_selection);
  }

  /// \brief Read the variable into a new vector. Python convenience function.
  /// \bug Get correct size based on selection operands.
  /// \tparam DataType is the type of the data to be written.
  /// \tparam Marshaller is a class that serializes / deserializes data.
  /// \tparam TypeWrapper translates DataType into a form that the backend understands.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  template <class DataType, class Marshaller = ioda::Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  std::vector<DataType> readAsVector(const Selection& mem_selection  = Selection::all,
                                     const Selection& file_selection = Selection::all) const {
    std::vector<DataType> data(getDimensions().numElements);
    read<DataType, Marshaller, TypeWrapper>(gsl::make_span(data.data(), data.size()), mem_selection,
                                            file_selection);
    return data;
  }

  /// \brief Valarray read convenience function. Resize if needed.
  ///   For a non-resizing version, use a gsl::span.
  /// \tparam DataType is the type of the data to be written.
  /// \tparam Marshaller is a class that serializes / deserializes data.
  /// \tparam TypeWrapper translates DataType into a form that the backend understands.
  /// \param data is a valarray acting as a data buffer that is filled with the
  ///   metadata's contents. It gets resized as needed.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \returns Another instance of this Attribute. Used for operation chaining.
  /// \note data will be stored in row-major order.
  template <class DataType, class Marshaller = ioda::Object_Accessor<DataType>,
            class TypeWrapper = Types::GetType_Wrapper<DataType>>
  Variable_Implementation read(std::valarray<DataType>& data,
                               const Selection& mem_selection  = Selection::all,
                               const Selection& file_selection = Selection::all) const {
    /// \bug Resize only if needed, and resize to the proper extent depending on
    /// mem_selection and file_selection.
    data.resize(getDimensions().numElements); // TODO(Ryan): remove
    return read<DataType, Marshaller, TypeWrapper>(gsl::make_span(std::begin(data), std::end(data)),
                                                   mem_selection, file_selection);
  }

  /// \brief Read data into an Eigen::Array, Eigen::Matrix, Eigen::Map, etc.
  /// \tparam EigenClass is a template pointing to the Eigen object.
  ///   This template must provide the EigenClass::Scalar typedef.
  /// \tparam Resize indicates whether the Eigen object should be resized
  ///   if there is a dimension mismatch. Not all Eigen objects can be resized.
  /// \param res is the Eigen object.
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \returns Another instance of this Variable. Used for operation chaining.
  /// \throws ioda::xError if the variable's dimensionality is
  ///   too high.
  /// \throws ioda::xError if resize = false and there is a dimension mismatch.
  /// \note When reading in a 1-D object, the data are read as a column vector.
  template <class EigenClass, bool Resize = detail::EigenCompat::CanResize<EigenClass>::value>
  Variable_Implementation readWithEigenRegular(EigenClass& res,
                                               const Selection& mem_selection = Selection::all,
                                               const Selection& file_selection
                                               = Selection::all) const {
    /// \bug Resize only if needed, and resize to the proper extent depending on
    /// mem_selection and file_selection.
#if 1  //__has_include("Eigen/Dense")
    try {
      typedef typename EigenClass::Scalar ScalarType;

      static_assert(
        !(Resize && !detail::EigenCompat::CanResize<EigenClass>::value),
        "This object cannot be resized, but you have specified that a resize is required.");

      // Check that the dimensionality is 1 or 2.
      const auto dims = getDimensions();
      if (dims.dimensionality > 2)
        throw Exception("Dimensionality too high for a regular Eigen read. Use "
          "Eigen::Tensor reads instead.", ioda_Here());

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

      auto ret = read<ScalarType>(gsl::span<ScalarType>(data_in.data(), dims.numElements),
                                  mem_selection, file_selection);
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
  /// \param mem_selection is the user's memory layout representing the location where
  ///   the data is read from.
  /// \param file_selection is the backend's memory layout representing the
  ///   location where the data are written to.
  /// \returns Another instance of this Variable. Used for operation chaining.
  /// \throws ioda::Exception if there is a size mismatch.
  /// \note When reading in a 1-D object, the data are read as a column vector.
  template <class EigenClass>
  Variable_Implementation readWithEigenTensor(EigenClass& res,
                                              const Selection& mem_selection = Selection::all,
                                              const Selection& file_selection
                                              = Selection::all) const {
#if 1  //__has_include("unsupported/Eigen/CXX11/Tensor")
    try {
      // Check dimensionality of source and destination
      const auto ioda_dims  = getDimensions();
      const auto eigen_dims = ioda::detail::EigenCompat::getTensorDimensions(res);
      if (ioda_dims.numElements != eigen_dims.numElements)
        throw Exception("Size mismatch for Eigen Tensor-like read.", ioda_Here());

      auto sp = (gsl::make_span(res.data(), eigen_dims.numElements));
      return read(sp, mem_selection, file_selection);
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
  EigenClass _readWithEigenRegular_python(const Selection& mem_selection  = Selection::all,
                                          const Selection& file_selection = Selection::all) const {
    EigenClass data;
    readWithEigenRegular(data, mem_selection, file_selection);
    return data;
  }

  /// @brief Convert a selection into its backend representation
  /// @param sel is the frontend selection.
  /// @return The cached backend selection.
  virtual Selections::SelectionBackend_t instantiateSelection(const Selection& sel) const;

  /// @}

private:
  /// \brief get the fill value from the netcdf specification (_FillValue attribute)
  FillValueData_t getNcFillValue() const;

  /// \brief check if fill data objects match, print warning if they don't match
  /// \param hdfFill fill value obtained from the hdf fill value property
  /// \param ncFill fill value obtained from the netcdf fill value attribute
  void checkWarnFillValue(FillValueData_t & hdfFill, FillValueData_t & ncFill) const;

  /// \brief run an action according to the current variable data type
  /// \details this function will call the function passed to it through the action
  /// parameter, giving this action a typed entity that can be identified by decltype.
  /// At this point, all of the types supported by Variable_Base::getBasicType() except
  /// for bool are handled in this function.
  /// \param action Function object callable with a single argument which is identifiable
  /// by decltype
  // TODO(srh) Additional development is required to properly handle the bool data type.
  template <typename Action>
  auto runForVarType(const Action & action) const {
    if (isA<int>()) {
      int typeMe;
      return action(typeMe);
    } else if (isA<unsigned int>()) {
      unsigned int typeMe;
      return action(typeMe);
    } else if (isA<float>()) {
      float typeMe;
      return action(typeMe);
    } else if (isA<double>()) {
      double typeMe;
      return action(typeMe);
    } else if (isA<std::string>()) {
      std::string typeMe;
      return action(typeMe);
    } else if (isA<long>()) {
      long typeMe;
      return action(typeMe);
    } else if (isA<unsigned long>()) {
      unsigned long typeMe;
      return action(typeMe);
    } else if (isA<short>()) {
      short typeMe;
      return action(typeMe);
    } else if (isA<unsigned short>()) {
      unsigned short typeMe;
      return action(typeMe);
    } else if (isA<long long>()) {
      long long typeMe;
      return action(typeMe);
    } else if (isA<unsigned long long>()) {
      unsigned long long typeMe;
      return action(typeMe);
    } else if (isA<int32_t>()) {
      int32_t typeMe;
      return action(typeMe);
    } else if (isA<uint32_t>()) {
      uint32_t typeMe;
      return action(typeMe);
    } else if (isA<int16_t>()) {
      int16_t typeMe;
      return action(typeMe);
    } else if (isA<uint16_t>()) {
      uint16_t typeMe;
      return action(typeMe);
    } else if (isA<int64_t>()) {
      int64_t typeMe;
      return action(typeMe);
    } else if (isA<uint64_t>()) {
      uint64_t typeMe;
      return action(typeMe);
    } else if (isA<long double>()) {
      long double typeMe;
      return action(typeMe);
    } else if (isA<char>()) {
      char typeMe;
      return action(typeMe);
    } else if (isA<unsigned char>()) {
      unsigned char typeMe;
      return action(typeMe);
    } else {
      std::throw_with_nested(Exception("Unsupported variable data type", ioda_Here()));
    }
  }

};
// extern template class Variable_Base<Variable>;
}  // namespace detail

/** \brief Variables store data!
 * \ingroup ioda_cxx_variable
 *
 * A variable represents a single field of data. It can be multi-dimensional and
 * usually has one or more attached **dimension scales**.
 *
 * Variables have Metadata, which describe the variable (i.e. valid_range, long_name, units).
 * Variables can have different data types (i.e. int16_t, float, double, string, datetime).
 * Variables can be resized. Depending on the backend, the data in a variable can be stored
 * using chunks, and may also be compressed.
 *
 * The backend manages how variables are stored in memory or on disk. The functions in the
 * Variable class provide methods to query and set data. The goal is to have data transfers
 * involve as few copies as possible.
 *
 * Variable objects themselves are lightweight handles that may be passed across different
 * parts of a program. Variables are always stored somewhere in a Group (or ObsSpace), so you
 * can always re-open a handle.
 *
 * \note Thread and MPI safety depend on the specific backends used to implement a variable.
 * \note A variable may be linked to multiple groups and listed under multiple names, so long as
 * the storage backends are all the same.
 **/
class IODA_DL Variable : public detail::Variable_Base<Variable> {
public:
  /// @name General Functions
  /// @{

  Variable();
  Variable(std::shared_ptr<detail::Variable_Backend> b);
  Variable(const Variable&);
  Variable& operator=(const Variable&);
  virtual ~Variable();

  /// @}
  /// @name Python compatability objects
  /// @{

  detail::python_bindings::VariableIsA<Variable> _py_isA;

  detail::python_bindings::VariableReadVector<Variable> _py_readVector;
  detail::python_bindings::VariableReadNPArray<Variable> _py_readNPArray;

  detail::python_bindings::VariableWriteVector<Variable> _py_writeVector;
  detail::python_bindings::VariableWriteNPArray<Variable> _py_writeNPArray;

  detail::python_bindings::VariableScales<Variable> _py_scales;

  /// @}
};

namespace detail {
/// \brief Variable backends inherit from this.
class IODA_DL Variable_Backend : public Variable_Base<Variable> {
public:
  virtual ~Variable_Backend();

  /// Default, trivial implementation. Customizable by backends for performance.
  std::vector<std::vector<Named_Variable>> getDimensionScaleMappings(
    const std::list<Named_Variable>& scalesToQueryAgainst,
    bool firstOnly = true) const override;

  /// Default implementation. Customizable by backends for performance.
  VariableCreationParameters getCreationParameters(bool doAtts = true,
                                                   bool doDims = true) const override;

protected:
  Variable_Backend();

  /// @brief This function de-encapsulates an Attribute's backend storage object.
  ///   This function is used by Variable_Backend's derivatives when accessing a
  ///   Variable's Attributes. IODA-internal use only.
  /// @tparam Attribute_Implementation is a dummy parameter used by Attributes.
  /// @param base is the Attribute whose backend you want.
  /// @return The encapsulated backend object.
  template <class Attribute_Implementation = Attribute>
  static std::shared_ptr<Attribute_Backend> _getAttributeBackend(
    const Attribute_Implementation& att) {
    return att.backend_;
  }

  /// @brief This function de-encapsulates a Has_Attributes backend storage object.
  ///   IODA-internal use only.
  /// @tparam Has_Attributes_Implementation is a dummy parameter for template resolution.
  /// @param hatts is the Has_Attributes whose backend you want.
  /// @return The encapsulated backend object.
  template <class Has_Attributes_Implementation = Has_Attributes>
  static std::shared_ptr<Has_Attributes_Backend> _getHasAttributesBackend(
    const Has_Attributes_Implementation& hatts) {
    return hatts.backend_;
  }
};
}  // namespace detail

/// @brief A named pair of (variable_name, ioda::Variable).
struct Named_Variable {
  std::string name;
  ioda::Variable var;
  bool operator<(const Named_Variable& rhs) const { return name < rhs.name; }

  Named_Variable() = default;
  Named_Variable(const std::string& name, const ioda::Variable& var) : name(name), var(var) {}
};

}  // namespace ioda

/// @}
