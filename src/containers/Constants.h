/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_CONSTANTS_H_
#define CONTAINERS_CONSTANTS_H_

#include <cstdint>
#include <string>

namespace osdf {
namespace consts {
  enum eDataTypes : std::int8_t {
    eInt8,
    eInt16,
    eInt32,
    eInt64,
    eFloat,
    eDouble,
    eString,
    eNumberOfDataTypes
  };

  enum ePermissions : std::int8_t {
    eReadOnly,
    eReadWrite
  };

  enum eSortOrders : std::int8_t {
    eAscending,
    eDescending
  };

  enum eComparisons : std::int8_t {
    eLessThan,
    eLessThanOrEqualTo,
    eEqualTo,
    eGreaterThanOrEqualTo,
    eGreaterThan
  };

  const std::string kSpace = " ";
  const std::string kBigSpace = "   ";

  const std::string kErrorReturnString = "ERROR: Not found.";
  const std::int32_t kErrorReturnValue = -1;
}  // namespace consts
}  // namespace osdf

#endif  // CONTAINERS_CONSTANTS_H_
