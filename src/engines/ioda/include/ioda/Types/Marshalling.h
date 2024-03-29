#pragma once
/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_types
 *
 * @{
 * \file Marshalling.h
 * \brief Classes and functions that implement the type system and allow
 *   for frontend/backend communication.
 */
#include <algorithm>
#include <chrono>
#include <complex>
#include <cstring>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "ioda/Exception.h"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/defs.h"

namespace ioda {

class Has_Attributes;

/// \ingroup ioda_internals_engines_types
template <class DataType, bool FreeOnClose>
void FreeType(DataType, typename std::enable_if<!std::is_pointer<DataType>::value>::type* = 0) {}
template <class DataType, bool FreeOnClose>
void FreeType(DataType, typename std::enable_if<std::is_pointer<DataType>::value>::type* = 0,
              typename std::enable_if<!FreeOnClose>::type* = 0) {}
template <class DataType, bool FreeOnClose>
void FreeType(DataType d, typename std::enable_if<std::is_pointer<DataType>::value>::type* = 0,
              typename std::enable_if<FreeOnClose>::type* = 0) {
  free(d);
}

/// \brief Structure used to pass data between the frontend and the backend engine.
/// \ingroup ioda_internals_engines_types
template <class T, class value_type = T, bool FreeOnClose = false>
struct Marshalled_Data {
  std::vector<value_type> DataPointers;
  detail::PointerOwner pointerOwner_;
  Marshalled_Data(detail::PointerOwner pointerOwner = detail::PointerOwner::Caller)
      : pointerOwner_(pointerOwner) {}
  ~Marshalled_Data() {
    if (pointerOwner_ == detail::PointerOwner::Engine) return;
    if (FreeOnClose)
      for (auto& p : DataPointers) FreeType<value_type, FreeOnClose>(p);
  }
};

namespace detail {
/// \note We want character streams, and these void* types are horribly hacked :-(
///   Using void* because we want to preserve a semantic difference between
///   serialized / deserialized data.
/// \note By default, we are using the POD accessor. Valid for simple data types,
/// where multiple objects are in the same dataspace, and each object is a
/// singular instance of the base data type.
/// \ingroup ioda_internals_engines_types
template <class DataType, class value_type = DataType>
struct Object_Accessor_Regular {
  typedef typename std::remove_const<DataType>::type mutable_DataType;
  typedef typename std::remove_const<value_type>::type mutable_value_type;
  static constexpr size_t bytesPerElement_ = sizeof(mutable_value_type);

  typedef std::shared_ptr<Marshalled_Data<DataType, mutable_DataType>> serialized_type;
  typedef std::shared_ptr<const Marshalled_Data<DataType, mutable_DataType>> const_serialized_type;

  detail::PointerOwner pointerOwner_;

public:
  Object_Accessor_Regular(detail::PointerOwner pointerOwner = detail::PointerOwner::Caller)
      : pointerOwner_(pointerOwner) {}
  /// \brief Converts an object into a void* byte stream.
  /// \note The shared_ptr takes care of "deallocation" when we no longer need the "buffer".
  const_serialized_type serialize(::gsl::span<const DataType> d, const Has_Attributes* = nullptr) {
    auto res          = std::make_shared<Marshalled_Data<DataType, mutable_DataType>>();
    res->DataPointers = std::vector<mutable_DataType>(d.size());
    // Forcible memset of the data to zero. Needed for long double type, which is typically 80
    // bits but takes up 96 or 128 bits of storage space. This triggers a Valgrind warning for
    // uninitialized memory access by HDF5.
    // See https://en.wikipedia.org/wiki/Long_double
    memset(res->DataPointers.data(), 0, sizeof(mutable_DataType) * d.size());
    for (size_t i = 0; i < (size_t)d.size(); ++i) res->DataPointers[i] = d[i];
    // res->DataPointers = std::vector<mutable_value_type>(d.begin(), d.end()); //return (const
    // void*)d.data();
    return res;
  }
  /// \brief Construct an object from a byte stream,
  /// and deallocate any temporary buffer.
  /// \note For trivial (POD) objects, there is no need to do anything.
  serialized_type prep_deserialize(size_t numObjects) {
    auto res          = std::make_shared<typename serialized_type::element_type>(pointerOwner_);
    res->DataPointers = std::vector<mutable_DataType>(numObjects);
    return res;
  }
  /// Unpack the data. For POD, nothing special here.
  void deserialize(serialized_type p, gsl::span<DataType> data, const Has_Attributes * = nullptr) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    if (ds != dp) throw Exception("ds != dp", ioda_Here());
    for (size_t i = 0; i < (size_t)data.size(); ++i) {
      data[i] = p->DataPointers[i];
    }
  }
};

/// \ingroup ioda_internals_engines_types
template <class DataType, class value_type = std::remove_pointer<std::decay<DataType>>>
struct Object_Accessor_Fixed_Array {
  typedef typename std::remove_const<DataType>::type mutable_DataType;
  typedef typename std::remove_const<value_type>::type mutable_value_type;
  static constexpr size_t bytesPerElement_ = sizeof(mutable_value_type);
  typedef std::shared_ptr<Marshalled_Data<DataType, mutable_DataType>> serialized_type;
  typedef std::shared_ptr<const Marshalled_Data<DataType, mutable_DataType>> const_serialized_type;
  detail::PointerOwner pointerOwner_;

public:
  Object_Accessor_Fixed_Array(detail::PointerOwner pointerOwner = detail::PointerOwner::Caller)
      : pointerOwner_(pointerOwner) {}
  /// \brief Converts an object into a void* byte stream.
  /// \note The shared_ptr takes care of "deallocation" when we no longer need the "buffer".
  const_serialized_type serialize(::gsl::span<const DataType> d, const Has_Attributes * = nullptr) {
    auto res          = std::make_shared<Marshalled_Data<DataType, mutable_DataType>>();
    res->DataPointers = std::vector<mutable_DataType>(d.size());
    std::copy_n(reinterpret_cast<char*>(d.data()), d.size_bytes(),
                reinterpret_cast<char*>(res->DataPointers.data()));
    // Cannot do this for int[2].
    // res->DataPointers = std::vector<mutable_value_type>(d.begin(), d.end());

    // for (size_t i = 0; i < (size_t)d.size(); ++i)
    //	res->DataPointers[i] = d[i];
    // res->DataPointers = std::vector<mutable_value_type>(d.begin(), d.end()); //return (const
    // void*)d.data();
    return res;
  }
  /// \brief Construct an object from a byte stream,
  /// and deallocate any temporary buffer.
  /// \note For trivial (POD) objects, there is no need to do anything.
  serialized_type prep_deserialize(size_t numObjects) {
    auto res          = std::make_shared<typename serialized_type::element_type>(pointerOwner_);
    res->DataPointers = std::vector<mutable_DataType>(numObjects);
    return res;
  }
  /// Unpack the data. For POD, nothing special here.
  void deserialize(serialized_type p, gsl::span<DataType> data, const Has_Attributes * = nullptr) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    if (ds != dp) throw Exception("ds != dp", ioda_Here());
    std::copy_n(reinterpret_cast<char*>(p->DataPointers.data()), data.size_bytes(),
                reinterpret_cast<char*>(data.data()));

    // for (size_t i = 0; i < (size_t)data.size(); ++i)
    //	data[i] = p->DataPointers[i];
  }
};

/// \ingroup ioda_internals_engines_types
template <class DataType, class value_type = DataType*>
struct Object_Accessor_Variable_Array_With_Data_Method {
  typedef typename std::remove_const<DataType>::type mutable_DataType;
  typedef typename std::remove_const<value_type>::type mutable_value_type;
  static constexpr size_t bytesPerElement_ = sizeof(mutable_value_type);
  typedef std::shared_ptr<const Marshalled_Data<DataType, mutable_value_type, false>>
    const_serialized_type;
  typedef std::shared_ptr<Marshalled_Data<DataType, mutable_value_type, true>> serialized_type;
  detail::PointerOwner pointerOwner_;

public:
  Object_Accessor_Variable_Array_With_Data_Method(detail::PointerOwner pointerOwner
                                                  = detail::PointerOwner::Caller)
      : pointerOwner_(pointerOwner) {}
  const_serialized_type serialize(::gsl::span<const DataType> d, const Has_Attributes * = nullptr) {
    auto res = std::make_shared<Marshalled_Data<DataType, mutable_value_type, false>>();
    for (const auto& i : d) {
      res->DataPointers.push_back(const_cast<mutable_value_type>(i.data()));
    }
    return res;
  }
  serialized_type prep_deserialize(size_t numObjects) {
    auto res          = std::make_shared<typename serialized_type::element_type>(pointerOwner_);
    res->DataPointers = std::vector<mutable_value_type>(numObjects);
    return res;
  }
  void deserialize(serialized_type p, gsl::span<DataType> data, const Has_Attributes * = nullptr) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    if (ds != dp) throw Exception("ds != dp", ioda_Here());
    for (size_t i = 0; i < ds; ++i) {
      if (p->DataPointers[i])  // Odd Valgrind detection. Maybe a false positive.
        data[i] = p->DataPointers[i];
    }
  }
};

/// \ingroup ioda_internals_engines_types
template <class DataType, class value_type = DataType*>
struct Object_Accessor_Variable_Raw_Array {
  typedef typename std::remove_const<DataType>::type mutable_DataType;
  typedef typename std::remove_const<value_type>::type mutable_value_type;
  static constexpr size_t bytesPerElement_ = sizeof(mutable_value_type);
  typedef std::shared_ptr<const Marshalled_Data<DataType, mutable_value_type, false>>
    const_serialized_type;
  typedef std::shared_ptr<Marshalled_Data<DataType, mutable_value_type, true>> serialized_type;
  detail::PointerOwner pointerOwner_;

public:
  Object_Accessor_Variable_Raw_Array(detail::PointerOwner pointerOwner
                                     = detail::PointerOwner::Caller)
      : pointerOwner_(pointerOwner) {}
  const_serialized_type serialize(::gsl::span<const DataType> d, const Has_Attributes * = nullptr) {
    auto res = std::make_shared<Marshalled_Data<DataType, mutable_value_type, false>>();
    for (const auto& i : d) {
      res->DataPointers.push_back(const_cast<mutable_value_type>(&i[0]));
    }
    return res;
  }
  serialized_type prep_deserialize(size_t numObjects) {
    auto res          = std::make_shared<typename serialized_type::element_type>(pointerOwner_);
    res->DataPointers = std::vector<mutable_value_type>(numObjects);
    return res;
  }
  void deserialize(serialized_type p, gsl::span<DataType> data, const Has_Attributes * = nullptr) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    if (ds != dp) throw Exception("ds != dp", ioda_Here());
    for (size_t i = 0; i < (size_t)data.size(); ++i) {
      data[i] = p->DataPointers[i];
    }
  }
};

/// \brief Determines the epoch time used when reading / writing a variable.
/// \details The epoch time may vary across systems, but is commonly Jan 1, 1970 at midnight.
///   For consistency inside of IODA data, we encode the epoch used as an ISO fixed-string
///   attribute.
/// \note We cannot encode a reference epoch when reading/writing attributes. Because of this,
///   attribute datetimes should be encoded as strings.
/// \param atts is the attribute container for the variable. The "units" attribute is checked
///   for a string "seconds since *****" which is used to compute the epoch. If atts is NULL or
///   if "units" does not exist, then the local system's epoch is returned.
///   If "units" is not parsable, then an exception is thrown.
/// \returns A time point representing the selected epoch.
/// \note This function needs to perform some time zone environment variable manipulation to
///   function properly. This is a limitation of C++14's available time functions. Once C++20
///   becomes widely available, then we should switch to a better system.
/// \todo Allow prefixes other than "seconds since ". This change will require broader OOPS and
///   IODA changes outside of the scope of ioda-engines.
IODA_DL ioda::Types::Chrono_Time_Point_t getEpoch(const Has_Attributes *atts = nullptr);


/// \brief Binding code to allow reads and writes directly to
/// Chrono_Time_Point_t objects
/// \details See Type.h for the definition of the Chrono_Time_Point_t type. This
/// Accessor is for the C++ API where the caller is using std::chrono objects.
struct Object_Accessor_Chrono_Time_Point_t {
  typedef std::shared_ptr<Marshalled_Data<ioda::Types::Chrono_Time_Rep_t>> serialized_type;
  typedef std::shared_ptr<const Marshalled_Data<ioda::Types::Chrono_Time_Rep_t>> const_serialized_type;
  detail::PointerOwner pointerOwner_;
  static constexpr size_t elementsPerObject_ = 1;
  static constexpr size_t bytesPerElement_ = sizeof(ioda::Types::Chrono_Time_Rep_t);

  Object_Accessor_Chrono_Time_Point_t(detail::PointerOwner pointerOwner
                                          = detail::PointerOwner::Caller)
      : pointerOwner_(pointerOwner) {}

  const_serialized_type serialize(::gsl::span<const ioda::Types::Chrono_Time_Point_t> d, const Has_Attributes* atts = nullptr) {
    auto res          = std::make_shared<Marshalled_Data<Types::Chrono_Time_Rep_t>>();
    res->DataPointers = std::vector<Types::Chrono_Time_Rep_t>(d.size() * elementsPerObject_);

    const auto epoch = getEpoch(atts);

    for (size_t i = 0; i < (size_t)d.size(); ++i) {
      res->DataPointers[i] = static_cast<Types::Chrono_Time_Rep_t>(
        std::chrono::duration_cast<std::chrono::seconds>(d[i] - epoch).count());
    }

    return res;
  }
  serialized_type prep_deserialize(size_t numObjects) {
    auto res          = std::make_shared<Marshalled_Data<Types::Chrono_Time_Rep_t>>(pointerOwner_);
    res->DataPointers = std::vector<Types::Chrono_Time_Rep_t>(numObjects * elementsPerObject_);
    return res;
  }
  void deserialize(serialized_type p, gsl::span<Types::Chrono_Time_Point_t> data, const Has_Attributes * atts = nullptr) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    if (ds != dp / elementsPerObject_)
      throw Exception("You are reading the wrong amount of data!", ioda_Here())
        .add("data.size()", ds)
        .add("p->DataPointers.size()", dp);
    
    // Convert times from durations with units of *seconds* and cast.
    const auto epoch = getEpoch(atts);

    for (size_t i = 0; i < (size_t)data.size(); ++i)
      data[i] = epoch + std::chrono::seconds{p->DataPointers[i]};
  }
};


/// \ingroup ioda_cxx_types
template <typename T>
struct Object_AccessorTypedef {
  typedef Object_Accessor_Regular<T> type;
};
/// \ingroup ioda_cxx_types
template <>
struct Object_AccessorTypedef<std::string> {
  typedef Object_Accessor_Variable_Array_With_Data_Method<std::string, char*> type;
};
/// \ingroup ioda_cxx_types
template <>
struct Object_AccessorTypedef<Types::Chrono_Time_Point_t> {
  typedef Object_Accessor_Chrono_Time_Point_t type;
};

// Used in an example
template <>
struct Object_AccessorTypedef<int[2]> {
  /// \todo Make a fixed-length array type
  typedef Object_Accessor_Fixed_Array<int[2]> type;
};

// Used in an example
template <>
struct Object_AccessorTypedef<std::array<int, 2>> {
  /// \todo Make a fixed-length array type
  typedef Object_Accessor_Fixed_Array<std::array<int, 2>, int> type;
};
}  // namespace detail

/// \ingroup ioda_cxx_types
template <typename DataType>
using Object_Accessor = typename detail::Object_AccessorTypedef<DataType>::type;

}  // namespace ioda

/// @}
