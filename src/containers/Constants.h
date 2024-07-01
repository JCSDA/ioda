/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>
#include <string>

namespace osdf {
namespace consts {
  /// \brief supported column data types
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

  /// \brief column permissions
  enum eColumnPermissions : std::int8_t {
    eReadOnly,
    eReadWrite
  };

  /// \brief ObsDataFrame types
  /// \details The *Priority are for "deep" objects (which allocate
  /// the memory for storing the data). The *View are for "shallow"
  /// objects (which don't allocate storage, rather they point to
  /// data in a deep object).
  enum eDataFrameTypes : std::int8_t {
    eRowPriority,
    eColumnPriority,
    eRowView,
    eColumnView
  };

  /// \brief sort directions
  enum eSortOrders : std::int8_t {
    eAscending,
    eDescending
  };

  /// \brief comparison operators
  enum eComparisons : std::int8_t {
    eLessThan,
    eLessThanOrEqualTo,
    eEqualTo,
    eGreaterThanOrEqualTo,
    eGreaterThan
  };

  /// \brief useful for print formatting
  const std::string kSpace = " ";
  const std::string kBigSpace = "   ";

  /// \brief ObsDataFrame error code
  const std::int32_t kErrorValue = -9999;
}  // namespace consts
}  // namespace osdf

#endif  // CONSTANTS_H
