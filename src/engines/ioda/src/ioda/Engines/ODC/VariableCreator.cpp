/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ODC/VariableCreator.h"

#include <Eigen/Core>
#include <gsl/gsl-lite.hpp>

#include "ioda/Engines/ContainerFacade.h"
#include "ioda/Engines/ODC/DataFromSQL.h"
#include "ioda/Engines/ODC/OdbConstants.h"
#include "ioda/Engines/ODC/VariableReaderFactory.h"

namespace ioda {
namespace Engines {
namespace ODC {

VariableCreator::VariableCreator(const std::string &name, const std::string &column,
                                 const std::string &member, bool hasChannelAxis,
                                 const VariableReaderParametersBase &readerParameters)
  : name_(name), column_(column), member_(member),
    readerParameters_(readerParameters.clone()), hasChannelAxis_(hasChannelAxis)
{}

void VariableCreator::createVariable(
    ContainerFacade &container, const RowsByLocation &rowsByLocation, const DataFromSQL &sqlData) const {
  if (sqlData.getColumnIndex(column_) < 0) {
    throw eckit::UnexpectedState("Source column " + column_ + " for variable " + name_ +
                                 " not found in ODB query results", Here());
  }

  std::unique_ptr<VariableReaderBase> reader =
      VariableReaderFactory::create(*readerParameters_, column_, member_, sqlData);

  const size_t numValuesPerLocation =
      hasChannelAxis_ ? container.numberOfChannels() : 1;

  const int columnType = sqlData.getColumnTypeByName(column_);
  if (member_.empty()) {
    if (columnType == odb_type_int || columnType == odb_type_bitfield)
      return createTypedVariable<int>(container, rowsByLocation, numValuesPerLocation,
                                      *reader);
    else if (columnType == odb_type_real)
      return createTypedVariable<float>(container, rowsByLocation, numValuesPerLocation,
                                        *reader);
    else if (columnType == odb_type_string)
      return createTypedVariable<std::string>(container, rowsByLocation,
                                              numValuesPerLocation, *reader);
    else
      throw eckit::BadValue("Unrecognized column type " + std::to_string(columnType), Here());
  } else {
    if (columnType == odb_type_bitfield)
      return createTypedVariable<char>(container, rowsByLocation, numValuesPerLocation,
                                       *reader);
    else
      throw eckit::BadValue("Column " + column_ + " is not of type 'bitfield'", Here());
  }
}

template <typename T>
void VariableCreator::createTypedVariable(
    ContainerFacade &container,
    const RowsByLocation &rowsByLocation, size_t numValuesPerLocation,
    const VariableReaderBase& reader) const {
  const size_t numLocations = rowsByLocation.size();
  const size_t numValues = numLocations * numValuesPerLocation;

  // Flattened 2D array of variable values, with numLocations rows and numValuesPerLocations
  // columns, stored in `container`'s native memory layout (row- or column-major).
  std::vector<T> values(numValues);
  if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> ||
                std::is_same_v<T, std::string>)
    std::fill(values.begin(), values.end(), odb_missing<T>());
  else if constexpr (std::is_same_v<T, char>)
    std::fill(values.begin(), values.end(), 0);

  if (!hasChannelAxis_ || container.nativeMemoryLayout() == ExplicitMemoryLayout::RowMajor) {
    // VariableReaderBase::getVariableValuesAtLocation() retrieves successive rows of the `values`
    // array.
    // If there's no channel axis (so row-major and column-major layouts are equivalent)
    // or the container's native layout is row-major, these rows can be written directly into the
    // array.
    T* firstValueAtLocation = values.data();
    for (size_t location = 0; location < numLocations;
         ++location, firstValueAtLocation += numValuesPerLocation) {
      reader.getVariableValuesAtLocation(
        gsl::make_span(rowsByLocation[location]),
        gsl::make_span(firstValueAtLocation, firstValueAtLocation + numValuesPerLocation));
    }
  } else {
    // If there's a channel axis and the container's native layout is column-major, fill the
    // `values` array via a column-major view of its contents.
    Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>> columnMajorValues(
      values.data(), numLocations, numValuesPerLocation);
    Eigen::RowVector<T, Eigen::Dynamic> currentValues(numValuesPerLocation);
    for (size_t location = 0; location < numLocations; ++location) {
      reader.getVariableValuesAtLocation(
        gsl::make_span(rowsByLocation[location]),
        gsl::make_span(currentValues));
      columnMajorValues.row(location) = currentValues;
    }
  }

  std::optional<T> missingValue;
  if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> ||
                std::is_same_v<T, std::string>)
    missingValue = odb_missing<T>();

  container.addVariable(name_, values, hasChannelAxis_, MemoryLayout::Native, missingValue);
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
