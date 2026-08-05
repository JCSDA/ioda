/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/ComplementarityInfo.h"

#include "../../Layouts/Layout_ObsGroup_ODB_Params.h"
#include "ioda/Engines/ODC/OdbColumnUtils.h"
#include "ioda/Engines/ODC/OdbQueryParameters.h"
#include "ioda/Engines/ODC/ParsedColumnExpression.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {
void splitVariablePathIntoGroupAndName(const std::string &variablePath,
                                       std::string &variableGroup, std::string &variableName) {
  const auto pos = variablePath.find('/');
  if (pos == std::string::npos) {
    variableGroup.clear();
    variableName = variablePath;
  } else {
    variableGroup = variablePath.substr(0, pos);
    variableName = variablePath.substr(pos + 1);
  }
}
}  // namespace

ComplementarityInfo::ComplementarityInfo(
    const detail::ODBLayoutParameters &layoutParams,
    const OdbQueryParameters &queryParams,
    const std::map<std::string, OdbColumnType> &odbColumnsInfo) {
  std::set<ParsedColumnExpression> queryContents;
  for (const auto &column : queryParams.variables.value())
    queryContents.insert(ParsedColumnExpression(column.name));

  for (const detail::VariableParameters &columnParams : layoutParams.variables.value()) {
    // Skip columns meant to be written, but not read.
    if (columnParams.mode.value() == detail::IoMode::WRITE)
      continue;

    // Skip sources absent from the query.
    ParsedColumnExpression parsedSource(columnParams.source);
    if (!isSourceInQuery(parsedSource, queryContents))
      continue;

    const std::string &aggregateColumnName = parsedSource.column;
    const std::string &aggregateVariablePath = columnParams.name;

    if (findFirstMatchingColumn(odbColumnsInfo, aggregateColumnName) != odbColumnsInfo.end()) {
      // A column with this name exists in the ODB file, so it has not been split into components
      // and there is no need to try to detect them.
      continue;
    }

    std::string columnName, tableName;
    splitIntoColumnAndTableName(aggregateColumnName, columnName, tableName);

    // Check if a column `aggregateColumnName` + "_1" exists. If so, it will be treated as the first
    // of possibly multiple components.
    int componentIndex = 1;
    const std::string firstComponentName = makeQualifiedComponentColumnName(
          columnName, tableName, componentIndex);
    UniqueMatchingColumnSearchErrorCode errorCode = UniqueMatchingColumnSearchErrorCode::SUCCESS;
    OdbColumnsInfo::const_iterator firstComponentColumnIt = findUniqueMatchingColumn(
          odbColumnsInfo, firstComponentName, odb_type_string, &errorCode);
    if (errorCode == UniqueMatchingColumnSearchErrorCode::SUCCESS) {
      // Yes, such a column exists.
      const std::string &qualifiedFirstComponentColumnName = firstComponentColumnIt->first;
      tableName = getTableName(qualifiedFirstComponentColumnName);

      std::string variableGroup, variableName;
      splitVariablePathIntoGroupAndName(aggregateVariablePath, variableGroup, variableName);

      complementaryColumns_[aggregateColumnName].push_back(qualifiedFirstComponentColumnName);
      complementaryVariables_[aggregateVariablePath].push_back(
            makeComponentVariablePath(variableGroup, variableName, componentIndex));

      // Check how many component exist.
      std::string qualifiedComponentName = makeQualifiedComponentColumnName(
            columnName, tableName, ++componentIndex);
      auto componentColumnIt =
          findFirstMatchingColumn(odbColumnsInfo, qualifiedComponentName, odb_type_string);
      while (componentColumnIt != odbColumnsInfo.end()) {
        complementaryColumns_[aggregateColumnName].push_back(qualifiedComponentName);
        complementaryVariables_[aggregateVariablePath].push_back(
              makeComponentVariablePath(variableGroup, variableName, componentIndex));

        qualifiedComponentName = makeQualifiedComponentColumnName(
              columnName, tableName, ++componentIndex);
        componentColumnIt =
            findFirstMatchingColumn(odbColumnsInfo, qualifiedComponentName, odb_type_string);
      }
    } else if (errorCode == UniqueMatchingColumnSearchErrorCode::ERROR_MULTIPLE_MATCHES) {
      throw eckit::UserError(
            "Column '" + columnName + "' of type string found in multiple ODB tables. "
                                      "Disambiguate it in the query file by following its name "
                                      "with the '@' character and a table name.", Here());
    }
  }
}

std::string ComplementarityInfo::makeQualifiedComponentColumnName(
    const std::string &columnName, const std::string &tableName, const int componentIndex) {
  return joinColumnAndTableName(columnName + "_" + std::to_string(componentIndex), tableName);
}

std::string ComplementarityInfo::makeComponentVariablePath(
    const std::string &variableGroup, const std::string &variableName, const int componentIndex) {
  return variableGroup + "/__" + variableName + "_" + std::to_string(componentIndex);
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
