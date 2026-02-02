/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/VariableCreator.h"

#include <gsl/gsl-lite.hpp>

#include "ioda/Engines/ODC/DataFromSQL.h"
#include "ioda/Engines/ODC/OdbConstants.h"
#include "ioda/Engines/ODC/VariableReaderFactory.h"
#include "ioda/ObsGroup.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {
std::vector<ioda::Variable> dimensionScales(ObsGroup &og, bool hasChannelAxis) {
  if (hasChannelAxis)
    return {og.vars["Location"], og.vars["Channel"]};
  else
    return {og.vars["Location"]};
}
}  // namespace

VariableCreator::VariableCreator(const std::string &name, const std::string &column,
                                 const std::string &member, bool hasChannelAxis,
                                 const VariableReaderParametersBase &readerParameters)
  : name_(name), column_(column), member_(member),
    readerParameters_(readerParameters.clone()), hasChannelAxis_(hasChannelAxis)
{}

ioda::Variable VariableCreator::createVariable(
    ObsGroup &og, const VariableCreationParameters &params, const RowsByLocation &rowsByLocation,
    const DataFromSQL &sqlData) const {
  if (sqlData.getColumnIndex(column_) < 0) {
    throw eckit::UnexpectedState("Source column " + column_ + " for variable " + name_ +
                                 " not found in ODB query results", Here());
  }

  std::unique_ptr<VariableReaderBase> reader =
      VariableReaderFactory::create(*readerParameters_, column_, member_, sqlData);

  const std::vector<ioda::Variable> scales = dimensionScales(og, hasChannelAxis_);
  const size_t numValuesPerLocation =
      hasChannelAxis_ ? scales.back().getDimensions().numElements : 1;

  const int columnType = sqlData.getColumnTypeByName(column_);
  if (member_.empty()) {
    if (columnType == odb_type_int || columnType == odb_type_bitfield)
      return createTypedVariable<int>(og, params, scales, rowsByLocation, numValuesPerLocation,
                                      *reader);
    else if (columnType == odb_type_real)
      return createTypedVariable<float>(og, params, scales, rowsByLocation, numValuesPerLocation,
                                        *reader);
    else if (columnType == odb_type_string)
      return createTypedVariable<std::string>(og, params, scales, rowsByLocation,
                                              numValuesPerLocation, *reader);
    else
      throw eckit::BadValue("Unrecognized column type " + std::to_string(columnType), Here());
  } else {
    if (columnType == odb_type_bitfield)
      return createTypedVariable<char>(og, params, scales, rowsByLocation, numValuesPerLocation,
                                       *reader);
    else
      throw eckit::BadValue("Column " + column_ + " is not of type 'bitfield'", Here());
  }
}

template <typename T>
ioda::Variable VariableCreator::createTypedVariable(
    ObsGroup &og, VariableCreationParameters params, const std::vector<ioda::Variable> &scales,
    const RowsByLocation &rowsByLocation, size_t numValuesPerLocation,
    const VariableReaderBase& reader) const {
  const size_t numLocations = rowsByLocation.size();
  const size_t numValues = numLocations * numValuesPerLocation;

  std::vector<T> values(numValues);
  if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> ||
                std::is_same_v<T, std::string>)
    std::fill(values.begin(), values.end(), odb_missing<T>());
  else if constexpr (std::is_same_v<T, char>)
    std::fill(values.begin(), values.end(), 0);

  T* firstValueAtLocation = values.data();
  for (size_t location = 0; location < numLocations;
       ++location, firstValueAtLocation += numValuesPerLocation) {
    reader.getVariableValuesAtLocation(
      gsl::make_span(rowsByLocation[location]),
      gsl::make_span(firstValueAtLocation, firstValueAtLocation + numValuesPerLocation));
  }

  if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> ||
                std::is_same_v<T, std::string>)
    params.setFillValue<T>(odb_missing<T>());
  ioda::Variable variable = og.vars.createWithScales<T>(name_, scales, params);
  variable.write(values);
  return variable;
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
