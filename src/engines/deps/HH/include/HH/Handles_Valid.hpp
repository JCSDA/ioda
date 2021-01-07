#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <type_traits>

namespace HH {
namespace Handles {
// From http://en.cppreference.com/w/cpp/experimental/is_detected
// and
// https://stackoverflow.com/questions/257288/is-it-possible-to-write-a-template-to-check-for-a-functions-existence
namespace detail {
struct nonesuch {
  nonesuch() = delete;
  ~nonesuch() = delete;
  nonesuch(nonesuch const&) = delete;
  void operator=(nonesuch const&) = delete;
};

template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
  // Note that std::void_t is a C++17 feature
  using value_t = std::true_type;
  using type = Op<Args...>;
};

}  // namespace detail

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<detail::nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<detail::nonesuch, void, Op, Args...>::type;

template <class Default, template <class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

template <typename T>
using valid_t = decltype(std::declval<T&>().valid());

/// Check if a class has a function matching "valid()".
template <typename T>
constexpr bool has_valid = is_detected_v<valid_t, T>;

/// \brief Ensures that a handle is not invalid.
/// \todo Add implicit construction of a weak handle intermediate, when checking validity.
template <class T>
class not_invalid {
private:
  std::shared_ptr<T> _heldObj;
  T& _ptr;

public:
  static_assert(has_valid<T> == true,
                "To use not_invalid, you must use a class that provides the valid() method.");
  constexpr T& get() const {
    bool isValid = _ptr.valid();
    if (!isValid) {  // Lazy extra check to allow me to easily break into a debug shell.
      Expects(isValid);
    }
    return _ptr;
  }
  constexpr T& operator()() const { return get(); }
  constexpr T* operator->() const { return &(get()); }
  constexpr decltype(auto) operator*() const { return (get()); }

  explicit constexpr not_invalid(T t) : _heldObj{std::make_shared<T>(t)}, _ptr{*_heldObj.get()} {
    const bool isValid = _ptr.valid();
    Expects(isValid);
  }

  /*
  constexpr not_invalid(T&& t) : _ptr(std::forward<T>(t)) {
  const bool isValid = _ptr.valid();
  Expects(isValid);
  }
  */

  /// \note This _cannot_ use a move _reference_, as I can't pass the conversion properly using non-pointer
  /// objects. cannot convert 'HH::Handles::not_invalid<HH::Handles::HH_hid_t>' to 'hid_t' \note Beware of
  /// inserting a ScopedHandle into not_invalid! Check that you do not instead want a WeakHandle!
  template <typename U>  //, typename = std::enable_if_t<std::is_convertible<U, T>::value>,
                         // typename = std::enable_if_t<!std::is_reference_v<U>> >
  explicit constexpr not_invalid(U u)
      : _heldObj{std::make_shared<T>(u)},
        _ptr{*_heldObj.get()} {  // u will be destroyed. Note difference between ScopedHandle (will close) and
                                 // WeakHandle (will not close) as input objects.
    const bool isValid = _ptr.valid();
    Expects(isValid);
  }

  /*
  template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>,
  typename = std::enable_if_t<std::is_reference_v<U>> >
  constexpr not_invalid(U&& u) : _ptr(std::forward<U>(u))
  {
  Expects(_ptr.valid());
  }
  */

  /*
  constexpr not_invalid(T& t) : _ptr(t) {
  Expects(_ptr.valid());
  }
  */

  template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  constexpr not_invalid(const not_invalid<U>& other) : not_invalid(other.get()) {}
};

}  // namespace Handles
}  // namespace HH
