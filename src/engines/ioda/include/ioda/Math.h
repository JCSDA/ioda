#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Math.h
* @brief Eigen wrappers for unit-aware, type-aware, and missing value-aware math!
*/

#include <Eigen/Core>
#include <Eigen/Dense>
#include <exception>
#include <iostream>
#include <type_traits>

#include "Units.h"

namespace ioda {

/** @brief A unit-aware wrapper for Eigen-encapsulated data.
*   @tparam Derived is the Eigen object (Array, Matrix, Reference, Map, unevaluated operation, etc.)
*   @details
*   
**/
template <typename Derived>
class EigenMath {
public:
  Derived data;          ///< The Eigen object that is being wrapped.
  udunits::Units units;  ///< The units
  typedef typename Derived::Scalar
    ScalarType;  ///< Represents the type of data inside the Eigen object (float, int, double, ...)
  ScalarType missingValue{};  ///< Represents missing data

  EigenMath(const Derived &data, const udunits::Units &units, const ScalarType &missingValue)
      : data(data), units(units), missingValue(missingValue) {}
  EigenMath(Derived &&data, const udunits::Units &units, const ScalarType &missingValue)
      : data(data), units(units), missingValue(missingValue) {}
  ~EigenMath() = default;

  /// @brief Triggers an explicit evaluation. Memory is copied, operations are collapsed, etc.
  auto eval() const {
    Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic> to_vals;
    to_vals.resizeLike(data);
    to_vals = data;
    return EigenMath<decltype(to_vals)>(std::move(to_vals), units, missingValue);
  }

  /// @brief Convert data to the specified units.
  /// @param to represents the desired units. Must be compatible with the current units (e.g. you can convert kilograms to grams, but not to meters).
  /// @throws ioda::Exception if the units are nonconvertible.
  /// @note This function triggers an expression evaluation. It limits performance somewhat, but it is okay for now.
  ///       In the future, we could visit the Galilean transformation in udunits and
  ///       extract the scaling factors ourselves.
  auto asUnits(const udunits::Units &to) const {
    auto converter = units.getConverterTo(to);
    Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic> to_vals;
    to_vals.resizeLike(data);
    converter->template tconvert<ScalarType>(data.data(), (size_t)data.size(), to_vals.data());
    to_vals = (data != missingValue).select(to_vals, missingValue);

    return EigenMath<decltype(to_vals)>(std::move(to_vals), to, missingValue);
  }
  auto asUnits(const std::string &to) const { return asUnits(udunits::Units(to)); }

  /// @brief Convert data to a new data type
  /// @tparam T is the new type (float, double, int, etc.)
  /// @returns The data cast to the new type.
  template <typename T>
  auto cast() const {
    auto castdata = data.template cast<T>();
    return EigenMath<decltype(castdata)>(castdata, units, static_cast<T>(missingValue));
  }

  /// \brief All additive, multiplicative, and comparative operators are implemented here.
  /// \details These function are in a static struct that operates like a "namespace". This way
  ///   the function implementations remain publicly accessable while "hidden" from GUIs.
  ///   These may be made private in the future.
  struct Operators {
    template <typename F, typename WE1>
    static auto UnitlessScalar(F func, const WE1 &lhs, const udunits::Units &units) {
      auto res = ((lhs.data == lhs.missingValue).eval()).select(lhs.missingValue, func()).eval();

      return EigenMath<decltype(res)>(res, units, lhs.missingValue);
    }

    template <typename F, typename WE1, typename WE2>
    static auto AdditiveWE(F func, const WE1 &lhs, const WE2 &rhs) {
      if (lhs.units == rhs.units) {
        auto lhsmissing = (lhs.data == lhs.missingValue);
        auto rhsmissing = (rhs.data == rhs.missingValue);
        auto res = ((lhsmissing || rhsmissing).eval()).select(lhs.missingValue, func()).eval();

        return EigenMath<decltype(res)>(res, lhs.units, lhs.missingValue);
      } else
        throw Exception("Nonequal units are being compared.", ioda_Here());
    }

    template <typename F, typename WE1, typename WE2>
    static auto MultiplicativeWE(F func, const udunits::Units &units, const WE1 &lhs,
                                 const WE2 &rhs) {
      auto lhsmissing = (lhs.data == lhs.missingValue);
      auto rhsmissing = (rhs.data == rhs.missingValue);
      auto res        = ((lhsmissing || rhsmissing).eval()).select(lhs.missingValue, func()).eval();

      return EigenMath<decltype(res)>(res, units, lhs.missingValue);
    }

    template <typename F, typename WE1, typename WE2>
    static auto ComparativeSC(F func, const WE1 &lhs, const WE2 &rhs) {
      auto ones = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>::Ones(lhs.data.rows(),
                                                                           lhs.data.cols());
      auto res  = (((std::is_same<bool, typename WE1::ScalarType>::value)
                      ? ones
                      : (lhs.data != lhs.missingValue).eval()))
                   .select(func(), false)
                   .eval();

      return EigenMath<decltype(res)>(res, udunits::RegularUnits("1"), false);
    }

    template <typename F, typename WE1, typename WE2>
    static auto ComparativeWE(F func, const WE1 &lhs, const WE2 &rhs) {
      if (lhs.units == rhs.units) {
        auto ones = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>::Ones(lhs.data.rows(),
                                                                             lhs.data.cols());
        auto res  = (((std::is_same<bool, typename WE1::ScalarType>::value)
                        ? ones
                        : (lhs.data != lhs.missingValue).eval())
                    && ((std::is_same<bool, typename WE2::ScalarType>::value)
                           ? ones
                           : (rhs.data != rhs.missingValue).eval()))
                     .select(func(), false)
                     .eval();

        return EigenMath<decltype(res)>(res, udunits::RegularUnits("1"), false);
      } else
        throw Exception("Nonequal units are being compared.", ioda_Here());
    }
  };

  // Elementwise additive operators (addition and subtraction of arrays & arrays, and arrays & scalars)
  template <typename Derived2>
  auto operator+(const EigenMath<Derived2> &rhs) const {
    return Operators::AdditiveWE([&]() { return (data + rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator+(EigenMath<Derived2> &&rhs) const {
    return Operators::AdditiveWE([&]() { return (data + rhs.data).eval(); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator+(const Derived2 &val) const {
    return Operators::UnitlessScalar([&]() { return (data + val); }, *this, units);
  }

  template <typename Derived2>
  auto operator-(const EigenMath<Derived2> &rhs) const {
    return Operators::AdditiveWE([&]() { return (data - rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator-(EigenMath<Derived2> &&rhs) const {
    return Operators::AdditiveWE([&]() { return (data - rhs.data).eval(); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator-(const Derived2 &val) const {
    return Operators::UnitlessScalar([&]() { return (data - val); }, *this, units);
  }

  // Elementwise multiplicative operators (multiplication and division of arrays & arrays, and arrays & scalars)
  template <typename Derived2>
  auto operator*(const EigenMath<Derived2> &rhs) const {
    return Operators::MultiplicativeWE([&]() { return (data * rhs.data); }, units * rhs.units,
                                       *this, rhs);
  }
  template <typename Derived2>
  auto operator*(EigenMath<Derived2> &&rhs) const {
    return Operators::MultiplicativeWE([&]() { return (data * rhs.data).eval(); },
                                       units * rhs.units, *this, rhs);
  }
  template <typename Derived2>
  auto operator*(const Derived2 &val) const {
    return Operators::UnitlessScalar([&]() { return (data * val); }, *this, units);
  }
  // C++17 only
  // template <>
  // auto operator*(const udunits::Units &val) const {
  //   return Operators::UnitlessScalar([&]() { return (data * val); }, *this, units * val);
  // }

  template <typename Derived2>
  auto operator/(const EigenMath<Derived2> &rhs) const {
    return Operators::MultiplicativeWE([&]() { return (data / rhs.data); }, units / rhs.units,
                                       *this, rhs);
  }
  template <typename Derived2>
  auto operator/(EigenMath<Derived2> &&rhs) const {
    return Operators::MultiplicativeWE([&]() { return (data / rhs.data).eval(); },
                                       units / rhs.units, *this, rhs);
  }
  template <typename Derived2>
  auto operator/(const Derived2 &val) const {
    return Operators::UnitlessScalar([&]() { return (data / val); }, *this, units);
  }
  // C++17 only
  // template <>
  // auto operator/(const udunits::Units &val) const {
  //   return Operators::UnitlessScalar([&]() { return (data / val); }, *this, units / val);
  // }

  // Other elementwise algebraic operators
  auto raise(int val) const {
    return Operators::UnitlessScalar([&]() { return (data.pow(static_cast<ScalarType>(val))); },
                                     *this, units.raise(val));
  }
  auto root(int val) const {
    return Operators::UnitlessScalar(
      [&]() { return (data.pow(static_cast<ScalarType>(1. / val))); }, *this, units.root(val));
  }
  auto pow(int num, int denom = 1) const { return (raise(num) / root(denom)); }

  // Elementwise comparison operators (all are boolean)
  template <typename Derived2>
  auto operator<(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data < rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator<(const Derived2 &val) const {
    return Operators::ComparativeSC([&]() { return (data < val); }, *this, val);
  }
  template <typename Derived2>
  auto operator>(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data > rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator>(const Derived2 &val) const {
    return Operators::ComparativeSC([&]() { return (data > val); }, *this, val);
  }
  template <typename Derived2>
  auto operator<=(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data <= rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator<=(const Derived2 &val) const {
    return Operators::ComparativeSC([&]() { return (data <= val); }, *this, val);
  }
  template <typename Derived2>
  auto operator>=(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data >= rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator>=(const Derived2 &val) const {
    return Operators::ComparativeSC([&]() { return (data >= val); }, *this, val);
  }
  template <typename Derived2>
  auto operator==(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data == rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator==(const Derived2 &val) const {
    return Operators::ComparativeSC([&]() { return (data == val); }, *this, val);
  }
  template <typename Derived2>
  auto operator!=(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data != rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator!=(const Derived2 &val) const {
    return Operators::ComparativeSC([&]() { return (data != val); }, *this, val);
  }
  // Elementwise logical operators (all are boolean)
  template <typename Derived2>
  auto operator&&(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data && rhs.data); }, *this, rhs);
  }
  template <typename Derived2>
  auto operator||(const EigenMath<Derived2> &rhs) const {
    return Operators::ComparativeWE([&]() { return (data || rhs.data); }, *this, rhs);
  }

  // Selection operations
  template <typename TrueVal, typename FalseVal>
  auto select(const EigenMath<TrueVal> &val_if_true,
              const EigenMath<FalseVal> &val_if_false) const {
    if (val_if_true.units != val_if_false.units)
      throw Exception("Incompatible units for select case.", ioda_Here());

    auto res = data
                 .select(val_if_true.data, (val_if_false.data != val_if_false.missingValue)
                                             .select(val_if_false.data, val_if_true.missingValue))
                 .eval();
    return EigenMath<decltype(res)>(std::move(res), val_if_true.units, val_if_true.missingValue);
  }

  template <typename TrueVal, typename FalseVal>
  auto select(const EigenMath<TrueVal> &val_if_true, const FalseVal &val_if_false) const {
    auto res = data.select(val_if_true.data, val_if_false).eval();
    return EigenMath<decltype(res)>(std::move(res), val_if_true.units, val_if_true.missingValue);
  }

  template <typename TrueVal, typename FalseVal>
  auto select(const TrueVal &val_if_true, const EigenMath<FalseVal> &val_if_false) const {
    auto res = data.select(val_if_true, val_if_false.data).eval();
    return EigenMath<decltype(res)>(std::move(res), val_if_false.units, val_if_false.missingValue);
  }

  /* // Odd fourth selection case. Unimplemented in Eigen.
	template <typename TrueVal, typename FalseVal>
	auto select(const TrueVal &val_if_true, const FalseVal &val_if_false,
			const udunits::Units &res_units, const typename Derived::Scalar &res_missingValue) const {
		// Slightly junky since Eigen lacks this fourth select case.
		auto res_f = (data == 0).select(val_if_false, data);
		auto res = data.select(val_if_true, res_f).eval();
		return EigenMath<decltype(res)>(std::move(res), res_units, res_missingValue);
	}
	*/

  auto whereMissing() const {
    auto res = (data == missingValue).eval();
    return EigenMath<decltype(res)>(std::move(res), udunits::RegularUnits("1"), false);
  }
};

/// @brief Convenience function to wrap Eigen data.
template <typename Derived>
EigenMath<Derived> ToEigenMath(const Derived &data, const udunits::Units &units,
                               const typename Derived::Scalar &missingValue) {
  return EigenMath<Derived>(data, units, missingValue);
}

template <typename Derived>
EigenMath<Derived> ToEigenMath(Derived &&data, const udunits::Units &units,
                               const typename Derived::Scalar &missingValue) {
  return EigenMath<Derived>(data, units, missingValue);
}

}  // end namespace ioda

/// @brief Convenience function to print Eigen data + units.
template <typename Derived>
std::ostream &operator<<(std::ostream &out, const ioda::EigenMath<Derived> &e) {
  (std::is_same<bool, typename Derived::Scalar>::value) ? out << e.data << " no units (boolean), "
                                                        : out << e.data << " units: " << e.units;
  (std::is_same<bool, typename Derived::Scalar>::value)
    ? out << "no missing value"
    : out << "   missing value: " << e.missingValue;
  return out;
}
