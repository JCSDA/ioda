/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameUtils.h"
#include <string>

#include "ioda/core/IodaUtils.h"
#include "ioda/containers/ColumnMetadatum.h"
#include "ioda/containers/Constants.h"

namespace osdf {
namespace FrameUtils {

using consts::columnSeparatorChar;
using consts::columnSeparatorString;
using consts::metadatumSeparatorString;
using consts::metadatumSeparatorChar;

//----------------------------------------------------------------------------
std::string serializeColumnMetadata(
                const std::vector<ColumnMetadatum> & columnMetadata) {
  std::string tokens = "columns" + metadatumSeparatorString
                       + std::to_string(columnMetadata.size());
  for (const ColumnMetadatum& columnMetadatum : columnMetadata) {
    tokens += columnSeparatorString + columnMetadatum.getName();
    tokens += metadatumSeparatorString + std::to_string(columnMetadatum.getType());
    tokens += metadatumSeparatorString + std::to_string(columnMetadatum.getPermission());
    tokens += metadatumSeparatorString + std::to_string(columnMetadatum.getWidth());
    tokens += metadatumSeparatorString + columnMetadatum.getUnit();
    tokens += metadatumSeparatorString;
  }
  return tokens;
}

//----------------------------------------------------------------------------
std::vector<ColumnMetadatum> deserializeColumnMetadataTokens(
                                 const std::string & columnMetadataTokens) {
  // Do some basic checks on the input string and extract the tokens
  const std::vector<std::string> tokens
    = ioda::splitString(columnMetadataTokens, columnSeparatorChar);
  const std::vector<std::string> firstTokenParts
    = ioda::splitString(tokens[0], metadatumSeparatorChar);
  if (firstTokenParts.size() != 2 || firstTokenParts[0] != "columns") {
    const std::string errMsg = std::string("ERROR: Column metadata string is not valid.");
    throw eckit::BadValue(errMsg, Here());
  }
  const int nCols = std::stoi(firstTokenParts[1]);
  if (nCols != static_cast<int>(tokens.size()) - 1) {
    const std::string errMsg = std::string("ERROR: Column metadata string has inconsistent ") +
          std::string("number of columns.");
    throw eckit::BadValue(errMsg, Here());
  }

  std::vector<ColumnMetadatum> columnMetadata;
  for (int i = 1; i <= nCols; ++i) {
    std::vector<std::string> tokenParts = ioda::splitString(tokens[i], metadatumSeparatorChar);
    if (tokenParts.size() != 5) {
      const std::string errMsg = std::string("ERROR: Column metadata string has invalid ") +
            std::string("column information: ") + tokens[i];
      throw eckit::BadValue(errMsg, Here());
    }

    // Extract the column information and create the ColumnMetadatum
    const std::string name = tokenParts[0];
    const std::int8_t type = static_cast<std::int8_t>(std::stoi(tokenParts[1]));
    const std::int8_t permission = static_cast<std::int8_t>(std::stoi(tokenParts[2]));
    const std::int16_t width     = static_cast<std::int16_t>(std::stoi(tokenParts[3]));
    const std::string unit = tokenParts[4];
    ColumnMetadatum columnMetadatum(name, unit, type, permission);
    columnMetadatum.setWidth(width);
    columnMetadata.push_back(columnMetadatum);
  }
  return columnMetadata;
}

}  // end namespace FrameUtils
}  // end namespace osdf
