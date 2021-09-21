/*
 * (C) Crown Copyright 2021 UK Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/OdbQueryParameters.h"

#include <utility>

namespace ioda {
namespace Engines {
namespace ODC {

constexpr char StarParameterTraitsHelper::enumTypeName[];
constexpr util::NamedEnumerator<StarParameter> StarParameterTraitsHelper::namedValues[];

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
