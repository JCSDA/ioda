#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Complex.h
/// \brief Type system example that implements support for std::complex<T> objects.
/// \todo Implement this.
#error "File under development. Do not use."

#include <memory>
#include <vector>

namespace ioda {
namespace detail {
template <class DataType, class value_type = DataType::value_type>
struct Object_Accessor_Complex {
  typedef typename std::remove_const<DataType>::type mutable_DataType;
  // typedef typename std::remove_const<typename DataType>::type mutable_DataType;

  typedef std::shared_ptr<Marshalled_Data<DataType, mutable_DataType>> serialized_type;
  typedef std::shared_ptr<const Marshalled_Data<DataType, mutable_DataType>> const_serialized_type;

public:
  /// \brief Converts an object into a void* byte stream.
  /// \note The shared_ptr takes care of "deallocation" when we no longer need the "buffer".
  const_serialized_type serialize(::gsl::span<const DataType> d) {
    auto res = std::make_shared<Marshalled_Data<DataType, mutable_DataType>>();
    res->DataPointers = std::vector<mutable_DataType>(d.size());
    for (size_t i = 0; i < (size_t)d.size(); ++i) res->DataPointers[i] = d[i];
    // res->DataPointers = std::vector<mutable_value_type>(d.begin(), d.end()); //return (const
    // void*)d.data();
    return res;
  }
  /// \brief Construct an object from a byte stream,
  /// and deallocate any temporary buffer.
  /// \note For trivial (POD) objects, there is no need to do anything.
  serialized_type prep_deserialize(size_t numObjects) {
    auto res = std::make_shared<Marshalled_Data<DataType, mutable_DataType>>();
    res->DataPointers = std::vector<mutable_DataType>(numObjects);
    return res;
  }
  /// Unpack the data. For POD, nothing special here.
  void deserialize(serialized_type p, gsl::span<DataType> data) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    Expects(ds == dp);
    for (size_t i = 0; i < (size_t)data.size(); ++i) {
      data[i] = p->DataPointers[i];
    }
  }
};

}  // namespace detail
}  // namespace ioda
