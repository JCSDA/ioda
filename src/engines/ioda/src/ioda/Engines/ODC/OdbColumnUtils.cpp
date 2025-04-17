/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/OdbColumnUtils.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/io/FileHandle.h"
#include "ioda/Engines/ODC/OdbTablesRange.h"
#include "odc/api/Odb.h"
#include "odc/core/MetaData.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace Engines {
namespace ODC {

OdbColumnsInfo getOdbColumnsInfo(const std::string &path) {
  OdbColumnsInfo result;
  try {
    for (const odc::core::Table &table :
         OdbTablesRange(std::make_unique<eckit::FileHandle>(path))) {
      for (const odc::core::Column* col : table.columns()) {
        ASSERT(col);
        OdbColumnType type = odb_type_ignore;
        switch (col->type()) {
        case odc::api::INTEGER:
          type = odb_type_int;
          break;
        case odc::api::REAL:
        case odc::api::DOUBLE:
          type = odb_type_real;
          break;
        case odc::api::STRING:
          type = odb_type_string;
          break;
        case odc::api::BITFIELD:
          type = odb_type_bitfield;
          break;
        default:
          type = odb_type_ignore;
        }
        auto it = result.find(col->name());
        if (it == result.end())
          result[col->name()] = type;
        else if (it->second != type)
          throw eckit::BadValue("The type of column '" + col->name() +
                                "' is not the same in all ODB frames containing that column",
                                Here());
      }
    }
  } catch (eckit::CantOpenFile &) {
    oops::Log::warning() << "Cannot open file '" << path << "' for reading" << std::endl;
  }

  return result;
}

OdbColumnsInfo::const_iterator findFirstMatchingColumn(
    const OdbColumnsInfo &odbColumnsInfo, const std::string &possiblyQualifiedColumnName) {
  for (auto it = odbColumnsInfo.begin(); it != odbColumnsInfo.end(); ++it) {
    const std::string &qualifiedColumnName = it->first;
    if (odc::core::columnNameMatches(qualifiedColumnName, possiblyQualifiedColumnName)) {
      return it;
    }
  }
  return odbColumnsInfo.end();
}

OdbColumnsInfo::const_iterator findFirstMatchingColumn(
    const OdbColumnsInfo &odbColumnsInfo,
    const std::string &possiblyQualifiedColumnName, OdbColumnType expectedType) {
  for (auto it = odbColumnsInfo.begin(); it != odbColumnsInfo.end(); ++it) {
    const std::string &qualifiedColumnName = it->first;
    const int &columnType = it->second;
    if (odc::core::columnNameMatches(qualifiedColumnName, possiblyQualifiedColumnName) &&
        columnType == expectedType) {
      return it;
    }
  }
  return odbColumnsInfo.end();
}

OdbColumnsInfo::const_iterator findUniqueMatchingColumn(
    const OdbColumnsInfo &odbColumnsInfo,
    const std::string &possiblyQualifiedColumnName, OdbColumnType expectedType,
    UniqueMatchingColumnSearchErrorCode *errorCode) {
  OdbColumnsInfo::const_iterator match = odbColumnsInfo.end();
  if (errorCode)
    *errorCode = UniqueMatchingColumnSearchErrorCode::ERROR_NO_MATCH;
  for (auto it = odbColumnsInfo.begin(); it != odbColumnsInfo.end(); ++it) {
    const std::string &qualifiedColumnName = it->first;
    const int &columnType = it->second;
    if (odc::core::columnNameMatches(qualifiedColumnName, possiblyQualifiedColumnName) &&
        columnType == expectedType) {
      if (match == odbColumnsInfo.end()) {
        match = it;
        if (errorCode)
          *errorCode = UniqueMatchingColumnSearchErrorCode::SUCCESS;
      } else {
        match = odbColumnsInfo.end();
        if (errorCode)
          *errorCode = UniqueMatchingColumnSearchErrorCode::ERROR_MULTIPLE_MATCHES;
        break;
      }
    }
  }
  return match;
}

void splitIntoColumnAndTableName(const std::string &possiblyQualifiedColumnName,
                                 std::string &columnName, std::string &tableName) {
  const auto pos = possiblyQualifiedColumnName.find('@');
  if (pos == std::string::npos) {
    columnName = possiblyQualifiedColumnName;
    tableName.clear();
  } else {
    columnName = possiblyQualifiedColumnName.substr(0, pos);
    tableName = possiblyQualifiedColumnName.substr(pos + 1);
  }
}

std::string joinColumnAndTableName(const std::string &columnName, const std::string &tableName) {
  if (tableName.empty())
    return columnName;
  else
    return columnName + '@' + tableName;
}

std::string getTableName(const std::string &possiblyQualifiedColumnName) {
  const auto pos = possiblyQualifiedColumnName.find('@');
  if (pos == std::string::npos) {
    return std::string();
  } else {
    return possiblyQualifiedColumnName.substr(pos + 1);
  }
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
