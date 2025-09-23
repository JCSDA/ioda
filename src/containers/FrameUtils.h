#pragma once
/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/Constants.h"

#include "ioda/Exception.h"

namespace osdf {

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
///     int8_t dtype = ...;  // dtype belonging to the eDataTypes enum
///     osdf::FrameUtils::callWithSupportedType(
///       dtype,
///       [&](auto typeDiscriminator) {  // This lambda is the "action" param in the code below
///         using T = decltype(typeDiscriminator);  //  T is discerned type of typeDiscriminator
///         doSomething1<T>(param1, param2);  // This line (or lines) is what you want done
///         doSomething2<T>(param3);
///       });
template <typename Action>
auto callWithSupportedType(const int8_t dtype, const Action &action) {
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
      throw ioda::Exception("ERROR: Data type misconfiguration...", ioda_Here());
  }
}

}  // end namespace FrameUtils
}  // end namespace osdf
