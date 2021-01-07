#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <string>
#include <tuple>
#include <vector>

#include "Handles.hpp"
#include "Types.hpp"
#if __has_include(<Eigen/Dense>)
#include <Eigen/Dense>
#endif
namespace HH {
/** \brief Implementation of tagged types, for C++-style variadic functions.
 *
 * This implementation allows for developers to pass parameters to functions by type,
 * instead of by parameter order. See Tags::Example::Example() for an example.
 **/
namespace Tags {
template <typename T, typename Tuple>
struct has_type;

template <typename T>
struct has_type<T, std::tuple<>> : std::false_type {};

template <typename T, typename U, typename... Ts>
struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>> {};

template <typename T, typename... Ts>
struct has_type<T, std::tuple<T, Ts...>> : std::true_type {};

template <class TagName, class DataType>
struct Tag {
  DataType data;
  explicit Tag(DataType d) : data(d) {}
  template <class... Args>
  explicit Tag(Args... args) : data(args...) {}

  constexpr const DataType& get() const { return data; }
  constexpr const DataType& operator()() const { return get(); }
  constexpr const DataType* operator->() const { return &(get()); }
  constexpr decltype(auto) operator*() const { return (get()); }
  Tag<TagName, DataType>& operator=(DataType d) {
    data = d;
    return *this;
  }
  Tag() {}
  typedef DataType value_type;
};

template <typename T, class... Args>
bool getOptionalValue(T& opt, std::tuple<Args...> vals,
                      typename std::enable_if<HH::Tags::has_type<T, std::tuple<Args...>>::value>::type* = 0) {
  opt = std::get<T>(vals);
  return true;
}

template <typename T, class... Args>
bool getOptionalValue(
  T& opt, std::tuple<Args...> vals,
  typename std::enable_if<!HH::Tags::has_type<T, std::tuple<Args...>>::value>::type* = 0) {
  return false;
}

namespace ObjSizes {
namespace _detail {
struct tag_dimensionality {};
struct tag_numpoints {};
struct tag_dimensions_current {};
struct tag_dimensions_max {};
}  // namespace _detail
typedef Tag<_detail::tag_dimensions_current, std::vector<hsize_t>> t_dimensions_current;
typedef Tag<_detail::tag_dimensions_max, std::vector<hsize_t>> t_dimensions_max;
typedef Tag<_detail::tag_dimensionality, hsize_t> t_dimensionality;
typedef Tag<_detail::tag_numpoints, hssize_t> t_numpoints;
}  // namespace ObjSizes

namespace Objects {
namespace _detail {
struct tag_storage_type {};
struct tag_objname_type {};
struct tag_dimensions_type {};
}  // namespace _detail
typedef Tag<_detail::tag_storage_type, HH_hid_t> HH_t_storage_type;
typedef Tag<_detail::tag_objname_type, std::string> t_name;
typedef Tag<_detail::tag_dimensions_type, std::initializer_list<size_t>> t_dimensions;
}  // namespace Objects
namespace Datatypes {
namespace _detail {
struct tag_datatype_id {};
}  // namespace _detail
typedef Tag<_detail::tag_datatype_id, HH_hid_t> t_datatype;
}  // namespace Datatypes
namespace Dataspaces {
namespace _detail {
struct tag_mem_space_id {};
struct tag_file_space_id {};
}  // namespace _detail
typedef Tag<_detail::tag_mem_space_id, HH_hid_t> t_mem_space;
typedef Tag<_detail::tag_file_space_id, HH_hid_t> t_file_space;
}  // namespace Dataspaces
namespace Datasets {
namespace _detail {
template <class T>
struct tag_data_as_span {};
template <class T>
struct tag_data_as_initializer_list {};
template <class T>
struct tag_data_as_eigen {};
template <class T>
struct tag_dset_DatasetParameterPack {};
}  // namespace _detail
template <class T>
using t_data_span = Tag<_detail::tag_data_as_span<T>, gsl::span<T>>;
template <class T>
using t_data_initializer_list = Tag<_detail::tag_data_as_initializer_list<T>, std::initializer_list<T>>;
#if __has_include(<Eigen/Dense>)
template <class T>
using t_data_eigen = Tag<_detail::tag_data_as_eigen<T>, ::Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>;
#endif
/// \note Keeping as a template to avoid awkwardness wrt forward
/// declarations of a struct within a struct.
template <class T>
using t_ParameterPack = Tag<_detail::tag_dset_DatasetParameterPack<T>, T>;
}  // namespace Datasets

// Namespace import
using namespace ObjSizes;
using namespace Objects;
using namespace Datatypes;
using namespace Dataspaces;
using namespace Datasets;
// using namespace PropertyLists;

/*
namespace Example {
// These tags produce parameterizable types!!!
struct tag_string {};
struct tag_int {};
struct tag_float {};
typedef Tag<tag_string, std::string> options_s;
typedef Tag<tag_int, int> options_i;
typedef Tag<tag_float, float> options_f;

template <class ... Args>
void caller(std::tuple<Args...> vals)
{
typedef std::tuple<Args...> vals_t;
static_assert(HH::Tags::has_type<options_i, vals_t >::value, "Must have an options_i type");
constexpr bool has_options_i = HH::Tags::has_type<options_i, vals_t >::value;
//std::cerr << "has_options_i " << has_options_i << std::endl;
auto opts = std::get<options_i>(vals);
constexpr bool has_options_f = HH::Tags::has_type<options_f, vals_t >::value;
//std::cerr << "has_options_f " << has_options_f << std::endl;
auto optional_float = options_f(-1.f);
bool gotVal = getOptionalValue(optional_float, vals);
//std::cerr << "got_options_f " << gotVal << std::endl
//	<< "opts_f " << optional_float.data << std::endl;
}
/// Pack everything into a tuple
template <class ... Args>
void caller(Args... args) {
auto t = std::make_tuple(args...);
caller(t);
}
// Can also allow for ordered parameters via clever function signatures. Trivial to implement.

inline void Example() {
auto a = options_f(1);
auto b = options_i(8);
caller(a, b);
caller(options_i(4), options_f(5));
//caller(a); // This won't work. options_i is a mandatory parameter.
// Optional parameters can be detected using constexpr bool. Add a template to detect and, optionally,
// set a value.
caller(b);
caller(options_i() = 29);

caller(options_i() = 29, options_f(5));
}

}
*/
}  // namespace Tags
}  // namespace HH
