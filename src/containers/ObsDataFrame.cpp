/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ObsDataFrame.h"

ObsDataFrame::ObsDataFrame(const std::int8_t type) : type_(type) {}

ObsDataFrame::ObsDataFrame(ColumnMetadata columnMetadata, const std::int8_t type) :
  columnMetadata_(columnMetadata), type_(type) {}

const std::int8_t ObsDataFrame::getType() {
  return type_;
}
