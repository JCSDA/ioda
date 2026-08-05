#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <map>
#include <string>

#include "OdbConstants.h"  // for OdbColumnType

namespace ioda {
namespace Engines {
namespace ODC {

/// \brief Maps qualified column names to column types.
///
/// A _qualified column name_ has the form `column_name@table_name`.
using OdbColumnsInfo = std::map<std::string, OdbColumnType>;

/// \brief Read frame headers from the ODB file located at `path` and return a map mapping the
/// qualified name of each column present in this file to the type of that column.
///
/// If the ODB file does not exist or cannot be opened, an empty map is returned (for consistency
/// with the behavior of the ODB reader itself).
OdbColumnsInfo getOdbColumnsInfo(const std::string &path);

/// \brief Return an iterator to the first entry in `odbColumnsInfo` corresponding to a column
/// whose qualified name matches `possiblyQualifiedColumnName`, or the end iterator if no such
/// entry is found.
///
/// `possiblyQualifiedColumnName` may be qualified or unqualified, i.e. may or may not contain a
/// `@table_name` suffix. If it is unqualified, matching is performed based solely on the column
/// name, disregarding the table name.
OdbColumnsInfo::const_iterator findFirstMatchingColumn(
    const OdbColumnsInfo &odbColumnsInfo, const std::string &possiblyQualifiedColumnName);

/// \brief Return an iterator to the first entry in `odbColumnsInfo` corresponding to a column
/// whose qualified name matches `possiblyQualifiedColumnName` and whose type is `expectedType`,
/// or the end iterator if no such entry is found.
OdbColumnsInfo::const_iterator findFirstMatchingColumn(
    const OdbColumnsInfo &odbColumnsInfo,
    const std::string &possiblyQualifiedColumnName, OdbColumnType expectedType);

/// \brief Success/error code returned by findUniqueMatchingColumn.
enum class UniqueMatchingColumnSearchErrorCode {
  /// Success: a unique match was found.
  SUCCESS,
  /// Failure: no match was found.
  ERROR_NO_MATCH,
  /// Failure: more than one match was found (so the match was not unique).
  ERROR_MULTIPLE_MATCHES
};

/// \brief Return an iterator to the unique entry in `odbColumnsInfo` corresponding to a column
/// whose qualified name matches `possiblyQualifiedColumnName` and whose type is `expectedType`,
/// or the end iterator if there is no such entry exists or there is more than one.
///
/// \param[out] errorCode
///   If not null, on output it is set to SUCCESS if a valid iterator is returned or to an error
///   code indicating whether the failure was due to no match or multiple matches having been found.
///
/// TODO: In C++23, this function could be rewritten to return a std::expected object.
OdbColumnsInfo::const_iterator findUniqueMatchingColumn(
    const OdbColumnsInfo &odbColumnsInfo,
    const std::string &possiblyQualifiedColumnName, OdbColumnType expectedType,
    UniqueMatchingColumnSearchErrorCode *errorCode);

/// \brief If `possiblyQualifiedColumnName` is a qualified column name, split it into a column and
/// table name. If it is an unqualified column name, copy it to `columnName` and set `tableName to
/// an empty string.
void splitIntoColumnAndTableName(const std::string &possiblyQualifiedColumnName,
                                 std::string &columnName, std::string &tableName);

/// \brief If `tableName` is non-empty, join `columnName` and `tableName` with an `@` character and
/// return the resulting string. Otherwise, return `columnName`.
std::string joinColumnAndTableName(const std::string &columnName, const std::string &tableName);

/// \brief If `possiblyQualifiedColumnName` is a qualified column name, return its constituent table
/// name; otherwise, return a copy of the input argument.
std::string getTableName(const std::string &possiblyQualifiedColumnName);

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
