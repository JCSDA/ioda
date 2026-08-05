#pragma once
/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <type_traits>

#include "eckit/exception/Exceptions.h"
#include "ioda/containers/Constants.h"
#include "oops/util/missingValues.h"

namespace osdf {
  class ColumnMetadatum;

namespace FrameUtils {

/// \brief Perform an action dependent on the type of an IFrame column variable \p dtype.
///
/// \param dtype
///   One of the allowed values of the osdf::consts::eDataTypes enum defined in Constants.h.
/// \param action
///   A function object callable with a single argument of any type corresponding to a value in the
///   eDataTypes enum
///
/// Example of use:
///
///     osdf::consts::eDataTypes dtype = ...;  // dtype belonging to the eDataTypes enum
///     osdf::FrameUtils::callWithSupportedType(
///       dtype,
///       [&](auto typeDiscriminator) {  // This lambda is the "action" param in the code below
///         using T = decltype(typeDiscriminator);  //  T is discerned type of typeDiscriminator
///         doSomething1<T>(param1, param2);  // This line (or lines) is what you want done
///         doSomething2<T>(param3);
///       });
template <typename Action>
auto callWithSupportedType(const consts::eDataTypes dtype, const Action &action) {
  switch (dtype) {
    case consts::eInt:
      return action(int());
    case consts::eInt64:
      return action(int64_t());
    case consts::eFloat:
      return action(float());
    case consts::eChar:
      return action(char());
    case consts::eString:
      return action(std::string());
    default:
      std::string msg = "ERROR: Data type misconfiguration...";
      throw eckit::Exception(msg, Here());
  }
}

/// \brief Convert a single value from a stored numeric frame type \p S to a requested numeric
/// frame type \p T, remapping the type-specific missing-value marker.
///
/// A plain static_cast would turn util::missingValue<S>() into an unrelated finite value of type
/// T; instead the stored missing marker is mapped to util::missingValue<T>() so that a "missing"
/// entry survives the conversion. String is not a numeric frame type and is rejected at compile
/// time (callers must guard string columns before instantiating this, e.g. via withCoercionType).
///
/// \param value
///   The stored value to convert.
template <typename T, typename S>
T coerce(const S value) {
  static_assert((std::is_arithmetic_v<S> && std::is_arithmetic_v<T>),
                "osdf::FrameUtils::coerce supports numeric frame types only");
  return value == util::missingValue<S>() ? util::missingValue<T>()
                                          : static_cast<T>(value);
}

/// \brief Dispatch \p action over the stored column type \p storedType while enforcing the OSDF
/// read/write type-coercion policy for a caller-requested type \p T.
///
/// \p action is a generic callable taking a single "type discriminator" argument whose type is the
/// stored column type S (mirroring callWithSupportedType). It is only invoked when both S and the
/// requested type T are numeric, so coerce<T, S> / coerce<S, T> are always well formed inside it.
/// Any attempt to convert between a string column and a numeric request (or vice versa) throws,
/// matching the historical strict behaviour for genuinely incompatible "text vs numbers" cases.
///
/// This is the piece shared by FrameCols and FrameRows: the missing-value-aware conversion rule
/// (coerce) and the string-vs-numeric policy live here once, while each container supplies only its
/// own storage-access loop.
///
/// This is the authoritative gate for the string/numeric policy. The check is enforced here,
/// keyed off the runtime storedType via callWithSupportedType, rather than relying on the
/// static_assert in coerce(): \p action is an arbitrary caller-supplied callable that may not call
/// coerce() at all, so coerce()'s static_assert only protects paths that actually instantiate it.
/// The if constexpr branch is resolved at compile time per (T, S) pair, so the string-paired
/// instantiation compiles down to just the throw and never invokes \p action.
///
/// \param storedType
///   The eDataTypes value of the column actually held in the container.
/// \param name
///   The column name, used only for the error message.
/// \param action
///   The numeric conversion action to perform, callable with a single type-discriminator argument.
template <typename T, typename Action>
void withCoercionType(const consts::eDataTypes storedType, const std::string& name,
                      const Action& action) {
  callWithSupportedType(storedType, [&](auto typeDiscriminator) {
    using S = decltype(typeDiscriminator);
    if constexpr (!(std::is_arithmetic_v<S> && std::is_arithmetic_v<T>)) {
      const std::string errMsg = std::string("ERROR: cannot convert column ") + name +
                                 std::string(" between string and numeric data types.");
      throw eckit::BadParameter(errMsg, Here());
    } else {
      action(typeDiscriminator);
    }
  });
}

}  // end namespace FrameUtils
}  // end namespace osdf
