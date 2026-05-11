/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/reader/OsdfFrameFacade.hpp"

#include <memory>
#include <optional>  // NOLINT(build/include_order): linter mis-identifies C++ header as C
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>  // NOLINT(build/include_order): linter mis-identifies C++ header as C
#include <vector>

#include <Eigen/Core>  // NOLINT(build/include_order): linter mis-identifies this as a C header

#include "eckit/exception/Exceptions.h"
#include "ioda/containers/Constants.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/Layout.h"
#include "ioda/Misc/StringFuncs.h"
#include "oops/util/missingValues.h"
#include "oops/util/Logger.h"

namespace ioda {

OsdfFrameFacade::OsdfFrameFacade(osdf::IFrame &frame, osdf::FrameMetadata &metadata)
    : frame_(frame), metadata_(metadata)
{}

void OsdfFrameFacade::initialize(size_t /*numLocations*/,
                                 const std::optional<std::vector<int>> &channelIndices,
                                 std::shared_ptr<const detail::DataLayoutPolicy> dataLayoutPolicy,
                                 const ContainerOptions &options) {
  dataLayoutPolicy_ = dataLayoutPolicy;
  options_ = options;

  if (channelIndices) {
    metadata_.setChanNums(*channelIndices);
  }
  isInitialized_ = true;
}

void OsdfFrameFacade::addDateTimeVariableToOptions(std::string dateTimeVariableName) {
  auto iter = std::find(options_.dateTimeVariables.begin(), options_.dateTimeVariables.end(),
                        dateTimeVariableName);
  // Add to list of dateTimeVariables if not already present.
  if (iter == options_.dateTimeVariables.end()) {
    options_.dateTimeVariables.emplace_back(dateTimeVariableName);
  }
}

int OsdfFrameFacade::numberOfChannels() const {
  return std::max<int>(metadata_.getChanNums().size(), 1);
}

void OsdfFrameFacade::addVariable(const std::string &name, const std::vector<int> &values,
                                  bool hasChannelAxis, MemoryLayout layout,
                                  const std::optional<int> &missingValue) {
  addTypedVariable(name, values, hasChannelAxis, layout, missingValue);
}

void OsdfFrameFacade::addVariable(const std::string &name, const std::vector<int64_t> &values,
                                  bool hasChannelAxis, MemoryLayout layout,
                                  const std::optional<int64_t> &missingValue) {
  addTypedVariable(name, values, hasChannelAxis, layout, missingValue);
}

void OsdfFrameFacade::addVariable(const std::string &name, const std::vector<float> &values,
                                  bool hasChannelAxis, MemoryLayout layout,
                                  const std::optional<float> &missingValue) {
  addTypedVariable(name, values, hasChannelAxis, layout, missingValue);
}

void OsdfFrameFacade::addVariable(const std::string &name, const std::vector<std::string> &values,
                                  bool hasChannelAxis, MemoryLayout layout,
                                  const std::optional<std::string> &missingValue) {
  addTypedVariable(name, values, hasChannelAxis, layout, missingValue);
}

void OsdfFrameFacade::addVariable(const std::string &name, const std::vector<char> &values,
                                  bool hasChannelAxis, MemoryLayout layout,
                                  const std::optional<char> &missingValue) {
  addTypedVariable(name, values, hasChannelAxis, layout, missingValue);
}

void OsdfFrameFacade::removeVariable(const std::string &name) {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  const std::string iodaName = iodaVariableName(name);
  if (iodaVariableHasChannelAxis(iodaName)) {
    for (int c : metadata_.getChanNums())
      frame_.removeColumn(iodaName + '_' + std::to_string(c));

    // FrameMetadata keeps track of the list of variables with a channel axis.
    // Remove the deleted variable from that list.
    std::unordered_set<std::string> varsWithChans = metadata_.getVarsWithChans();
    varsWithChans.erase(iodaName);
    metadata_.setVarsWithChans(varsWithChans);
    metadata_.removeVarDimNames(iodaName);
  } else {
    frame_.removeColumn(iodaName);
  }
  missingValueByIodaName_.erase(iodaName);
}

bool OsdfFrameFacade::hasVariable(const std::string &name) const {
  if (!isInitialized_)
    return false;

  const std::string iodaName = iodaVariableName(name);
  if (iodaVariableHasChannelAxis(iodaName) && !metadata_.getChanNums().empty())
    return frame_.hasColumn(iodaName + '_' + std::to_string(metadata_.getChanNums().front()));
  else
    return frame_.hasColumn(iodaName);
}

Engines::ContainerVariableType OsdfFrameFacade::variableType(const std::string &name) const {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  const std::string iodaName = iodaVariableName(name);
  int8_t typeAsInt;
  if (iodaVariableHasChannelAxis(iodaName) && !metadata_.getChanNums().empty())
    typeAsInt = frame_.getColumnType(iodaName + '_' +
                                     std::to_string(metadata_.getChanNums().front()));
  else
    typeAsInt = frame_.getColumnType(iodaName);

  switch (typeAsInt) {
    case osdf::consts::eDataTypes::eInt:
      return ContainerVariableType::Int;
    case osdf::consts::eDataTypes::eInt64:
      return ContainerVariableType::Int64;
    case osdf::consts::eDataTypes::eFloat:
      return ContainerVariableType::Float;
    case osdf::consts::eDataTypes::eString:
      return ContainerVariableType::String;
    case osdf::consts::eDataTypes::eChar:
      return ContainerVariableType::Char;
    default:
      throw eckit::BadValue("Unrecognized type", Here());
  }
}

bool OsdfFrameFacade::hasChannelAxis(const std::string &name) const {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  return iodaVariableHasChannelAxis(iodaVariableName(name));
}

void OsdfFrameFacade::setVariableUnit(const std::string &/*name*/,
                                      const std::string &/*unit*/) {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  // The OSDF container currently does not store units.
}

void OsdfFrameFacade::setVariableValues(const std::string &name,
                                        const std::vector<int> &values,
                                        MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::setVariableValues(const std::string &name,
                                        const std::vector<int64_t> &values,
                                        MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::setVariableValues(const std::string &name,
                                        const std::vector<float> &values,
                                        MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::setVariableValues(const std::string &name,
                                        const std::vector<std::string> &values,
                                        MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::setVariableValues(const std::string &name, const std::vector<char> &values,
                                        MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

std::vector<std::string> OsdfFrameFacade::iodaVariableNames() const {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  std::vector<std::string> orderedNames;
  std::unordered_set<std::string> unorderedNames;

  const std::unordered_set<std::string> &varsWithChannels = metadata_.getVarsWithChans();
  std::string prefix;
  int channelIndex = -1;

  for (const std::string &name : frame_.columnNames()) {
    if (extractChannelSuffixIfPresent(name, prefix, channelIndex) &&
        varsWithChannels.count(prefix) != 0) {
      // Treat `prefix` as the variable name (regardless of whether the channel index belongs to
      // metadata_.getChanNums() or not, for the time being).
      if (unorderedNames.count(prefix) == 0) {
        unorderedNames.insert(prefix);
        orderedNames.push_back(prefix);
      }
    } else {
      // Treat `name` as the variable name.
      if (unorderedNames.count(name) == 0) {
        unorderedNames.insert(name);
        orderedNames.push_back(name);
      }
    }
  }

  return orderedNames;
}

Engines::ExplicitMemoryLayout OsdfFrameFacade::nativeMemoryLayout() const {
  return ExplicitMemoryLayout::ColumnMajor;
}

bool OsdfFrameFacade::needsSourceLocationIndices() const { return true; }

osdf::IFrame &OsdfFrameFacade::frame() { return frame_; }

osdf::FrameMetadata &OsdfFrameFacade::metadata() { return metadata_; }

void OsdfFrameFacade::getVariableValues(const std::string &name, std::vector<int> &values,
                                        MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::getVariableValues(const std::string &name, std::vector<int64_t> &values,
                                        MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::getVariableValues(const std::string &name, std::vector<float> &values,
                                        MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::getVariableValues(const std::string &name, std::vector<std::string> &values,
                                        MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::getVariableValues(const std::string &name, std::vector<char> &values,
                                        MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void OsdfFrameFacade::getMissingValue(const std::string &name,
                                      std::optional<int> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void OsdfFrameFacade::getMissingValue(const std::string &name,
                                      std::optional<int64_t> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void OsdfFrameFacade::getMissingValue(const std::string &name,
                                      std::optional<float> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void OsdfFrameFacade::getMissingValue(const std::string &name,
                                      std::optional<std::string> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void OsdfFrameFacade::getMissingValue(const std::string &name,
                                      std::optional<char> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

std::string OsdfFrameFacade::iodaVariableName(const std::string &name) const {
  return dataLayoutPolicy_->doMap(name);
}

bool OsdfFrameFacade::iodaVariableHasChannelAxis(const std::string &iodaName) const {
  return metadata_.varHasChannels(iodaName);
}

template <typename T>
void OsdfFrameFacade::addTypedVariable(const std::string &name, const std::vector<T> &values,
                                       bool hasChannelAxis, MemoryLayout layout,
                                       const std::optional<T> &missingValue) {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  if (hasChannelAxis && metadata_.getChanNums().empty())
    throw eckit::UserError("Channel indices have not been defined", Here());

  const std::string iodaName = iodaVariableName(name);
  setTypedIodaVariableValues(iodaName, values, hasChannelAxis, layout,
                             missingValue, true /* createNewColumn? */);
  if (hasChannelAxis) {
    metadata_.addVarToVarsWithChans(iodaName);
    metadata_.addVarDimNames(iodaName, {"Location", "Channel"});
  }

  // Count variable (in numVars) if it is in the ObsValue group
  if (iodaName.find("ObsValue/") != std::string::npos) {
    metadata_.incrNumVars();
  }
}

template <typename T>
void OsdfFrameFacade::getTypedVariableValues(const std::string &name, std::vector<T> &values,
                                             MemoryLayout layout) const {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  if (variableType(name) != Engines::containerVariableType<T>)
    throw eckit::BadValue("Attempted to read variable " + name +
                             " into a vector of incorrect type", Here());

  const std::string iodaName = iodaVariableName(name);
  if (iodaVariableHasChannelAxis(iodaName)) {
    using Matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
    const std::vector<int> &channelIndices = metadata_.getChanNums();
    const size_t numChannels = channelIndices.size();
    const size_t numLocations = frame_.numRows();

    Matrix fallbackColumnMajorMatrix;
    Eigen::Map<Matrix> columnMajorValuesView(nullptr, 0, 0);
    const ExplicitMemoryLayout explicitLayout = explicitMemoryLayout(layout);
    if (explicitLayout == ExplicitMemoryLayout::ColumnMajor) {
      values.resize(numLocations * numChannels);
      new (&columnMajorValuesView) Eigen::Map<Matrix>(values.data(),
                                                      numLocations, numChannels);
    } else {
      fallbackColumnMajorMatrix.resize(numLocations, numChannels);
      new (&columnMajorValuesView) Eigen::Map<Matrix>(fallbackColumnMajorMatrix.data(),
                                                      numLocations, numChannels);
    }

    std::vector<T> column;
    column.reserve(numLocations);
    for (size_t columnIndex = 0; columnIndex < numChannels; ++columnIndex) {
      const int channelIndex = channelIndices[columnIndex];
      frame_.getColumn(iodaName + '_' + std::to_string(channelIndex), column);
      for (size_t locIndex = 0; locIndex < numLocations; ++locIndex)
        columnMajorValuesView(locIndex, columnIndex) = column[locIndex];
    }

    if (explicitLayout != ExplicitMemoryLayout::ColumnMajor) {
      values.resize(numLocations * numChannels);
      Eigen::Map<Matrix> rowMajorValuesView(values.data(), numChannels, numLocations);
      rowMajorValuesView = columnMajorValuesView.transpose();
    }
  } else {
    frame_.getColumn(iodaName, values);
  }

  std::optional<T> missingValue;
  getTypedIodaVariableMissingValue(iodaName, missingValue);
  if (missingValue && (*missingValue != util::missingValue<T>())) {
    // Replace the oops missing value with the missing value specified when filling the variable.
    std::replace(values.begin(), values.end(), util::missingValue<T>(), *missingValue);
  }
}

template <typename T>
void OsdfFrameFacade::getTypedMissingValue(const std::string &name,
                                           std::optional<T> &missingValue) const {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  getTypedIodaVariableMissingValue(iodaVariableName(name), missingValue);
}

template <typename T>
void OsdfFrameFacade::getTypedIodaVariableMissingValue(const std::string &iodaName,
                                                       std::optional<T> &missingValue) const {
  missingValue = std::get<std::optional<T>>(missingValueByIodaName_.at(iodaName));
}

template <typename T>
void OsdfFrameFacade::setTypedVariableValues(const std::string &name, const std::vector<T> &values,
                                             MemoryLayout layout) {
  if (!isInitialized_)
    throw eckit::UserError("Frame has not been initialized yet", Here());

  const std::string iodaName = iodaVariableName(name);
  std::optional<T> missingValue;
  getTypedIodaVariableMissingValue(iodaName, missingValue);
  setTypedIodaVariableValues(iodaName, values, iodaVariableHasChannelAxis(iodaName),
                             layout, missingValue, false /*createNewColumns?*/);
}

template <typename T>
void OsdfFrameFacade::setTypedIodaVariableValues(const std::string &iodaName,
                                                 const std::vector<T> &values,
                                                 bool hasChannelAxis, MemoryLayout layout,
                                                 const std::optional<T> &missingValue,
                                                 bool createNewColumns) {
  if (hasChannelAxis) {
    using Matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
    const std::vector<int> &channelIndices = metadata_.getChanNums();
    const size_t numValues = values.size();
    const size_t numChannels = channelIndices.size();
    const size_t numLocations = numValues / numChannels;
    if (numChannels * numLocations != numValues)
      throw eckit::BadValue("Number of values is not divisible by the number of channels",
                            Here());

    Matrix fallbackColumnMajorMatrix;
    Eigen::Map<const Matrix> valuesView(nullptr, 0, 0);
    const ExplicitMemoryLayout explicitLayout = explicitMemoryLayout(layout);
    if (explicitLayout == ExplicitMemoryLayout::ColumnMajor) {
      new (&valuesView) Eigen::Map<const Matrix>(values.data(), numLocations, numChannels);
    } else {
      Eigen::Map<const Matrix> rowMajorValuesView(values.data(), numChannels, numLocations);
      fallbackColumnMajorMatrix = rowMajorValuesView.transpose();
      new (&valuesView) Eigen::Map<const Matrix>(fallbackColumnMajorMatrix.data(),
                                                 numLocations, numChannels);
    }

    std::vector<T> column;
    column.resize(numLocations);
    for (size_t columnIndex = 0; columnIndex < numChannels; ++columnIndex) {
      const int channelIndex = channelIndices[columnIndex];
      if (missingValue)
        std::replace_copy(valuesView.col(columnIndex).data(),
                          valuesView.col(columnIndex).data() + numLocations,
                          column.begin(),
                          *missingValue, util::missingValue<T>());
      else
        column.assign(valuesView.col(columnIndex).data(),
                      valuesView.col(columnIndex).data() + numLocations);

      const std::string fullName = iodaName + '_' + std::to_string(channelIndex);
      if (createNewColumns) {
        frame_.appendNewColumn(fullName, column);
      } else {
        frame_.setColumn(fullName, column);
      }
    }
  } else {
    std::vector<T> editedValues;
    const std::vector<T> *column = &values;
    if (missingValue) {
      editedValues.resize(values.size());
      std::replace_copy(values.begin(), values.end(),
                        editedValues.begin(),
                        *missingValue, util::missingValue<T>());
      column = &editedValues;
    }
    if (createNewColumns) {
      // If variable is in list of known datetimes, add epoch as units
      auto iter = std::find(options_.dateTimeVariables.begin(),
                            options_.dateTimeVariables.end(), iodaName);
      if (iter == options_.dateTimeVariables.end()) {
        frame_.appendNewColumn(iodaName, *column);
      } else {
        frame_.appendNewColumn(iodaName, *column, options_.epoch);
      }
    } else {
      frame_.setColumn(iodaName, *column);
    }
  }

  missingValueByIodaName_[iodaName] = missingValue;
}

}  // namespace ioda
