#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <vector>

namespace ioda {
namespace Engines {
namespace ODC {

/// \brief A container whose ith element is the set of indices of ODB rows associated with the ith
/// location.
using RowsByLocation = std::vector<std::vector<std::size_t>>;

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
