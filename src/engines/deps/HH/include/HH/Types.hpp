#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <hdf5.h>

#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "./defs.hpp"
#include "Errors.hpp"
#include "Handles.hpp"

namespace HH {
namespace _impl {
HH_DL size_t COMPAT_strncpy_s(char* dest, size_t destSz, const char* src, size_t srcSz);
}  // namespace _impl
namespace Types {
using namespace HH::Handles;
/// \todo extend to UTF-8 strings, as HDF5 supports these. No support for UTF-16, but conversion functions may
/// be applied. \todo Fix for "const std::string".
template <typename T>
struct is_string
    : public std::integral_constant<bool, std::is_same<char*, typename std::decay<T>::type>::value ||
                                            std::is_same<const char*, typename std::decay<T>::type>::value> {
};
template <>
struct is_string<std::string> : std::true_type {};

namespace constants {
constexpr int _Variable_Length = -1;
// constexpr int _Not_An_Array_type = -3;
}  // namespace constants

/// Accessor for class-like objects
// template <class DataType, class ObjType = DataType>
// DataType* Accessor(ObjType* e) { return e; }
// template <>
// const char* Accessor<const char*, std::string>(std::string* e) { return e.c_str(); }

/// For fundamental, non-string types.
/// \note Template specializations are implemented for the actual data types, like int32_t, double, etc.
/// \todo Change these signatures to allow for user extensibility into custom structs,
/// or even objects like std::complex<T>.
template <class DataType, int Array_Type_Dimensionality = 0>
HH_hid_t GetHDF5Type(std::initializer_list<hsize_t> Adims = {},
                     typename std::enable_if<!is_string<DataType>::value>::type* = 0) {
  if (Array_Type_Dimensionality <= 0) {
    // static_assert(false, "HH::Types::GetHDF5Type does not understand this data type.");
    throw;  // HH_throw.add("Reason", "HH::Types::GetHDF5Type does not understand this data type.");
    return HH_hid_t(
      -1,
      HH::Handles::Closers::DoNotClose::CloseP);  // Should never reach this. Invalid handle, just in case.
  } else {
    // This is a compound array type of the same object, repeated.
    HH_hid_t fundamental_type = GetHDF5Type<DataType, 0>();
    hid_t t = H5Tarray_create2(fundamental_type(), Array_Type_Dimensionality, Adims.begin());
    if (t < 0) throw;  // HH_throw.add("Reason", "H5Tarray_create2 failed.");
    return HH_hid_t(t, HH::Handles::Closers::CloseHDF5Datatype::CloseP);
  }
}
/// For fundamental string types. These are either constant or variable length arrays. Separate handling
/// elsewhere.
template <class DataType, int String_Type_Length = constants::_Variable_Length>
HH_hid_t GetHDF5Type(int Runtime_String_Type_Length = constants::_Variable_Length,
                     typename std::enable_if<is_string<DataType>::value>::type* = 0) {
  size_t strtlen = String_Type_Length;
  if (Runtime_String_Type_Length != constants::_Variable_Length) strtlen = Runtime_String_Type_Length;
  if (strtlen == constants::_Variable_Length) strtlen = H5T_VARIABLE;
  hid_t t = H5Tcreate(H5T_STRING, strtlen);
  if (t < 0) throw;  // HH_throw.add("Reason", "H5Tcreate failed.");
  return HH_hid_t(t, HH::Handles::Closers::CloseHDF5Datatype::CloseP);
}

template <>
inline HH_hid_t GetHDF5Type<char>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_CHAR);
}
template <>
inline HH_hid_t GetHDF5Type<int8_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_INT8);
}
template <>
inline HH_hid_t GetHDF5Type<uint8_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_UINT8);
}
template <>
inline HH_hid_t GetHDF5Type<int16_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_INT16);
}
template <>
inline HH_hid_t GetHDF5Type<int16_t const>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_INT16);
}
template <>
inline HH_hid_t GetHDF5Type<uint16_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_UINT16);
}
template <>
inline HH_hid_t GetHDF5Type<int32_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_INT32);
}
template <>
inline HH_hid_t GetHDF5Type<int32_t const>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_INT32);
}
template <>
inline HH_hid_t GetHDF5Type<uint32_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_UINT32);
}
template <>
inline HH_hid_t GetHDF5Type<int64_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_INT64);
}
template <>
inline HH_hid_t GetHDF5Type<uint64_t>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_UINT64);
}
template <>
inline HH_hid_t GetHDF5Type<float>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_FLOAT);
}
template <>
inline HH_hid_t GetHDF5Type<float const>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_FLOAT);
}
template <>
inline HH_hid_t GetHDF5Type<double>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_DOUBLE);
}
template <>
inline HH_hid_t GetHDF5Type<bool>(std::initializer_list<hsize_t>, void*) {
  return HH_hid_t(H5T_NATIVE_HBOOL);
}

inline HH_hid_t GetHDF5TypeFixedString(hsize_t sz) {
  hid_t strtype = H5Tcopy(H5T_C_S1);
  if (strtype < 0) throw;  // HH_throw.add("Reason", "H5Tcopy failed.");
  herr_t status = H5Tset_size(strtype, sz);
  if (status < 0) throw;  // HH_throw.add("Reason", "H5Tset_size failed.");
  return HH_hid_t(strtype);
}

template <class DataType, bool FreeOnClose>
void FreeType(DataType d, typename std::enable_if<!std::is_pointer<DataType>::value>::type* = 0) {}
template <class DataType, bool FreeOnClose>
void FreeType(DataType d, typename std::enable_if<std::is_pointer<DataType>::value>::type* = 0,
              typename std::enable_if<!FreeOnClose>::type* = 0) {}
template <class DataType, bool FreeOnClose>
void FreeType(DataType d, typename std::enable_if<std::is_pointer<DataType>::value>::type* = 0,
              typename std::enable_if<FreeOnClose>::type* = 0) {
  free(d);
}

template <class T, class value_type = T, bool FreeOnClose = false>
struct Marshalled_Data {
  std::vector<value_type> DataPointers;
  ~Marshalled_Data() {
    if (FreeOnClose)
      for (auto& p : DataPointers) FreeType<value_type, FreeOnClose>(p);
  }
};

namespace detail {
/// \note HDF5 wants void* types. These are horribly hacked :-(
/// \note By default, we are using the POD accessor. Valid for simple data types,
/// where multiple objects are in the same dataspace, and each object is a
/// singular instance of the base data type.
template <class DataType, class value_type = DataType>
struct Object_Accessor_Regular {
  typedef typename std::remove_const<DataType>::type mutable_DataType;
  // typedef typename std::remove_const<typename DataType>::type mutable_DataType;

  typedef std::shared_ptr<Marshalled_Data<DataType, mutable_DataType>> serialized_type;
  typedef std::shared_ptr<const Marshalled_Data<DataType, mutable_DataType>> const_serialized_type;

public:
  /// \brief Converts an object into a void* array that HDF5 can natively understand.
  /// \note The shared_ptr takes care of "deallocation" when we no longer need the "buffer".
  const_serialized_type serialize(::gsl::span<const DataType> d) {
    auto res = std::make_shared<Marshalled_Data<DataType, mutable_DataType>>();
    res->DataPointers = std::vector<mutable_DataType>(d.size());
    for (size_t i = 0; i < (size_t)d.size(); ++i) res->DataPointers[i] = d[i];
    // res->DataPointers = std::vector<mutable_value_type>(d.begin(), d.end()); //return (const
    // void*)d.data();
    return res;
  }
  /// \brief Construct an object from an HDF5-provided data stream,
  /// and deallocate any temporary buffer.
  /// \note For trivial (POD) objects, there is no need to do anything.
  serialized_type prep_deserialize(
    size_t numChars)  // Let's make this a span. Don't forget to update Object_Accessor_Array.
  {
    auto res = std::make_shared<Marshalled_Data<char, char>>();
    res->DataPointers = std::vector<char>(numChars);
    return res;
  }
  /// Unpack the data. For POD, nothing special here.
  void deserialize(serialized_type p, gsl::span<DataType> data) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    // HH_Expects(ds == dp);
    for (size_t i = 0; i < (size_t)data.size(); ++i) {
      data[i] = (reinterpret_cast<DataType*>(
        p->DataPointers
          .data()))[i];  // Let's do a char-by-char copy? Don't forget to update Object_Accessor_Array.
    }
  }
};

template <class DataType, class value_type = DataType*>
struct Object_Accessor_Array {
  typedef typename std::remove_const<value_type>::type mutable_value_type;
  typedef std::shared_ptr<const Marshalled_Data<DataType, mutable_value_type, false>> const_serialized_type;
  typedef std::shared_ptr<Marshalled_Data<DataType, mutable_value_type, true>> serialized_type;

public:
  const_serialized_type serialize(::gsl::span<const DataType> d) {
    typedef typename std::remove_const<value_type>::type mutable_value_type;
    auto res = std::make_shared<Marshalled_Data<DataType, mutable_value_type, false>>();
    for (const auto& i : d) {
      res->DataPointers.push_back(const_cast<mutable_value_type>(i.data()));
    }
    return res;

    /*
    for (const auto& s : d) {
            size_t sz = s.size() + 1;
            std::vector<char> sobj(sz);
            _impl::COMPAT_strncpy_s(sobj.data(), sz, s.data(), sz);
            _bufStrPointers.push_back(sobj.data());
            _bufStrs.push_back(std::move(sobj));
    }
    */
    // return (const void*)_bufStrPointers.data();
  }
  serialized_type prep_deserialize(size_t numChars) {
    auto res = std::make_shared<Marshalled_Data<char, char, true>>();
    res->DataPointers = std::vector<char>(numChars);
    return res;
  }
  void deserialize(serialized_type p, gsl::span<DataType> data) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    // HH_Expects(ds == dp);
    for (size_t i = 0; i < (size_t)data.size(); ++i) {
      data[i] = (reinterpret_cast<DataType*>(p->DataPointers.data()))[i];
    }
  }
};

template <typename T>
struct Object_AccessorTypedef {
  typedef Object_Accessor_Regular<T> type;
};
template <>
struct Object_AccessorTypedef<std::string> {
  typedef Object_Accessor_Array<std::string, char*> type;
};
}  // namespace detail
template <typename DataType>
using Object_Accessor = typename detail::Object_AccessorTypedef<DataType>::type;

/*
template<>
struct Object_Accessor<char*>
{
private:
std::unique_ptr<char[]> _buffer;
ssize_t _sz; ///< Size (in bytes) of _buffer.
public:
/// \param sz is the size of the string (+ NULL-termination) to be
/// read. Only needed if reading the object. Should be passed in via the
/// appropriate calls to the HDF5 backend to determine the string's size.
Object_Accessor(ssize_t sz = -1) : _sz(sz), _buffer(new char[sz]) {}
~Object_Accessor() {}
std::shared_ptr<const void> serialize(const char* d)
{
return std::shared_ptr<const void>((const void*)d, [](const void*) {});
}
ssize_t getFromBufferSize() {
return _sz;
}
/// \note For this class specialization, _buffer is marshaled on object construction.
void marshalBuffer(char** objStart) { }
void deserialize(char **objStart) {
std::memcpy(*objStart, _buffer.get(), _sz);
}
void freeBuffer() {} ///< No need in this impl., as the unique-ptr frees itself.
};
*/

}  // namespace Types
}  // namespace HH
