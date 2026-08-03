/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ObsGroupFacade.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "eckit/exception/Exceptions.h"
#include "ioda/Engines/ContainerFacade.h"
#include "ioda/ObsGroup.h"

#include "oops/util/Logger.h"

namespace ioda {
namespace Engines {

namespace {
std::vector<ioda::Variable> dimensionScales(const ObsGroup &og, bool hasChannelAxis) {
  if (hasChannelAxis)
    return {og.vars["Location"], og.vars["Channel"]};
  else
    return {og.vars["Location"]};
}

}  // namespace

ObsGroupFacade::ObsGroupFacade(Group group, FacadeMode mode)
    : group_(group), FacadeMode_(mode), isInitialized_(false) {
  // If the ObsGroupFacade is in writer mode, we don't want to create a new ObsGroup
  if (FacadeMode_ == FacadeMode::WriterMode) {
    og_ = group_;
    isInitialized_ = true;
    numChannels_ = channelNumbers().size();
  }
}

void ObsGroupFacade::initialize(size_t numLocations,
                                const std::optional<std::vector<int>> &channelIndices,
                                std::shared_ptr<const detail::DataLayoutPolicy> dataLayoutPolicy,
                                const ContainerOptions &options) {
  switch (FacadeMode_) {
    case FacadeMode::ReaderMode: {
      NewDimensionScales_t scales;
      scales.push_back(
        NewDimensionScale<int>("Location", numLocations, numLocations, numLocations));

      if (channelIndices) {
        numChannels_ = channelIndices->size();
        scales.push_back(
          NewDimensionScale<int>("Channel", numChannels_, numChannels_, numChannels_));
      } else {
        numChannels_ = 0;
      }

      options_ = options;

      og_ = ObsGroup::generate(group_, scales, dataLayoutPolicy);

      if (channelIndices && numLocations != 0) {
        ioda::Variable v = og_.vars["Channel"];
        v.write(*channelIndices);
      }

      isInitialized_ = true;
    } break;

    case FacadeMode::WriterMode:
      // Calling the above initialisation script on an ObsGroupFacade in writerMode will overwrite
      // the obsGroup og_ which contains the data to be written out, so instead we abort.
      throw eckit::BadValue(
        "Attempting to call initialise on an ObsGroupFacade in writerMode. Aborting.",
        Here());  
    default:
      throw eckit::BadValue("Unknown facadeMode in ObsGroupFacade initialisation. Aborting.",
                            Here());      
  }
}

void ObsGroupFacade::addDateTimeVariableToOptions(std::string dateTimeVariableName) {
  oops::Log::warning() << "Using ObsGroup - dateTimeVariableName not stored." << std::endl;
};

int ObsGroupFacade::numberOfLocations() const {
  return og_.vars["Location"].getDimensions().dimsCur[0];
}

int ObsGroupFacade::numberOfChannels() const {
  return numChannels_;
}

std::string ObsGroupFacade::variableUnits(const std::string &name) const {
  return og_.vars[name].atts.open("units").read<std::string>();
}

std::vector<int> ObsGroupFacade::channelNumbers() const {
  std::vector<int> Channel;
  if (!og_.vars.exists("Channel")) {
    return Channel;
  }

  const TypeClass type = og_.vars["Channel"].getType().getClass();
  if (type == TypeClass::Integer) {
    og_.vars["Channel"].read<int>(Channel);
  } else {
    std::vector<float> ChannelFloat;
    og_.vars["Channel"].read<float>(ChannelFloat);
    for (size_t i = 0; i < ChannelFloat.size(); ++i)
      Channel.emplace_back(static_cast<int>(ChannelFloat[i]));
  }
  return Channel;
}

void ObsGroupFacade::addVariable(const std::string &name, const std::vector<int> &values,
                                 bool hasChannelAxis, MemoryLayout layout,
                                 const std::optional<int> &missingValue) {
  addTypedVariable<int>(name, values, hasChannelAxis, layout, missingValue);
}

void ObsGroupFacade::addVariable(const std::string &name, const std::vector<int64_t> &values,
                                 bool hasChannelAxis, MemoryLayout layout,
                                 const std::optional<int64_t> &missingValue) {
  addTypedVariable<int64_t>(name, values, hasChannelAxis, layout, missingValue);
}

void ObsGroupFacade::addVariable(const std::string &name, const std::vector<float> &values,
                                 bool hasChannelAxis, MemoryLayout layout,
                                 const std::optional<float> &missingValue) {
  addTypedVariable<float>(name, values, hasChannelAxis, layout, missingValue);
}

void ObsGroupFacade::addVariable(const std::string &name, const std::vector<std::string> &values,
                                 bool hasChannelAxis, MemoryLayout layout,
                                 const std::optional<std::string> &missingValue) {
  addTypedVariable<std::string>(name, values, hasChannelAxis, layout, missingValue);
}

void ObsGroupFacade::addVariable(const std::string &name, const std::vector<char> &values,
                                 bool hasChannelAxis, MemoryLayout layout,
                                 const std::optional<char> &missingValue) {
  addTypedVariable<char>(name, values, hasChannelAxis, layout, missingValue);
}

void ObsGroupFacade::removeVariable(const std::string &name) {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());

  og_.vars.remove(name);
}

bool ObsGroupFacade::hasVariable(const std::string &name) const {
  return isInitialized_ && og_.vars.exists(name);
}

ContainerVariableType ObsGroupFacade::variableType(const std::string &name) const {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());

  const BasicTypes type = og_.vars[name].getBasicType();

  switch (type) {
    case BasicTypes::int32_:
      return ContainerVariableType::Int;
    case BasicTypes::int64_:
      return ContainerVariableType::Int64;
    case BasicTypes::float_:
      return ContainerVariableType::Float;
    case BasicTypes::str_:
      return ContainerVariableType::String;
    case BasicTypes::char_:
      return ContainerVariableType::Char;
    default:
      throw eckit::BadValue("Unrecognized type", Here());
  }
}

bool ObsGroupFacade::hasChannelAxis(const std::string &name) const {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());

  if (!og_.vars.exists("Channel") || !og_.vars.exists(name))
    return false;

  if (name == "Channel")
    return true;

  Variable channelVar = og_.vars["Channel"];
  Variable var = og_.vars[name];
  return var.getDimensions().dimensionality > 1 && var.isDimensionScaleAttached(1, channelVar);
}

void ObsGroupFacade::setVariableUnit(const std::string &name, const std::string &unit) {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());

  og_.vars[name].atts.add<std::string>("units", unit);
}

void ObsGroupFacade::setVariableValues(const std::string &name, const std::vector<int> &values,
                                       MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::setVariableValues(const std::string &name, const std::vector<int64_t> &values,
                                       MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::setVariableValues(const std::string &name, const std::vector<float> &values,
                                       MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::setVariableValues(const std::string &name,
                                       const std::vector<std::string> &values,
                                       MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::setVariableValues(const std::string &name, const std::vector<char> &values,
                                       MemoryLayout layout) {
  setTypedVariableValues(name, values, layout);
}

std::vector<std::string> ObsGroupFacade::iodaVariableNames() const {
  if (!isInitialized_)
    return {};

  std::vector<std::string> names = og_.listObjects<ObjectType::Variable>(true /*recurse*/);
  // Remove the special "Location" and "Channel" variables, which do not exist in the OSDF
  // container.
  auto newEnd = std::remove(names.begin(), names.end(), "Location");
  newEnd = std::remove(names.begin(), newEnd, "Channel");
  names.resize(newEnd - names.begin());
  return names;
}

ExplicitMemoryLayout ObsGroupFacade::nativeMemoryLayout() const {
  return ExplicitMemoryLayout::RowMajor;
}

bool ObsGroupFacade::needsSourceLocationIndices() const { return false; }

ObsGroup & ObsGroupFacade::obsGroup() { return og_; }

void ObsGroupFacade::getVariableValues(const std::string &name, std::vector<int> &values,
                                       MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::getVariableValues(const std::string &name, std::vector<int64_t> &values,
                                       MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::getVariableValues(const std::string &name, std::vector<float> &values,
                                       MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::getVariableValues(const std::string &name, std::vector<std::string> &values,
                                       MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::getVariableValues(const std::string &name, std::vector<char> &values,
                                       MemoryLayout layout) const {
  getTypedVariableValues(name, values, layout);
}

void ObsGroupFacade::getMissingValue(const std::string &name,
                                     std::optional<int> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void ObsGroupFacade::getMissingValue(const std::string &name,
                                     std::optional<int64_t> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void ObsGroupFacade::getMissingValue(const std::string &name,
                                     std::optional<float> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void ObsGroupFacade::getMissingValue(const std::string &name,
                                     std::optional<std::string> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

void ObsGroupFacade::getMissingValue(const std::string &name,
                                     std::optional<char> &missingValue) const {
  getTypedMissingValue(name, missingValue);
}

template <typename T>
void ObsGroupFacade::addTypedVariable(const std::string &name, const std::vector<T> &values,
                                      bool hasChannelAxis, MemoryLayout layout,
                                      const std::optional<T> &missingValue) {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());
  if (hasChannelAxis && numChannels_ == 0)
    throw eckit::UserError("Attempted to add a variable " + name + " with a channel axis "
                           "to an ObsGroup without channels");

  if (hasChannelAxis && numChannels_ == 0)
    throw eckit::UserError("Attempted to add a variable " + name
                           + " with a channel axis to an ObsGroup without channels.");

  using Matrix = RowMajorMatrix<T>;
  Matrix fallbackRowMajorMatrix;
  Eigen::Map<const Matrix> rowMajorValuesView = rowMajorMatrixView(
    values, hasChannelAxis, layout, fallbackRowMajorMatrix);

  const std::vector<ioda::Variable> scales = dimensionScales(og_, hasChannelAxis);
  ioda::VariableCreationParameters params;
  if (missingValue)
    params.setFillValue<T>(*missingValue);
  ioda::Variable variable = og_.vars.createWithScales<T>(name, scales, params);

  variable.write(gsl::span<const T>(rowMajorValuesView.data(), rowMajorValuesView.size()));
}

template <typename T>
void ObsGroupFacade::getTypedVariableValues(const std::string &name, std::vector<T> &values,
                                            MemoryLayout layout) const {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());

  if (variableType(name) != containerVariableType<T>)
    throw eckit::BadValue("Attempted to read variable " + name +
                             " into a vector of incorrect type", Here());

  if (hasChannelAxis(name) && explicitMemoryLayout(layout) == ExplicitMemoryLayout::ColumnMajor) {
    const std::vector<T> rowMajorValues = og_.vars[name].readAsVector<T>();
    const size_t numValues = rowMajorValues.size();
    const size_t numChannels = numberOfChannels();
    const size_t numLocations = numValues / numChannels;
    if (numLocations * numChannels != numValues)
      throw eckit::BadValue("Number of values is not divisible by the number of channels",
                            Here());

    Eigen::Map<const RowMajorMatrix<T>> rowMajorValuesView(rowMajorValues.data(),
                                                           numLocations, numChannels);

    values.resize(numValues);
    Eigen::Map<ColumnMajorMatrix<T>> columnMajorValuesView(values.data(),
                                                           numLocations, numChannels);
    columnMajorValuesView = rowMajorValuesView;
  } else {
    values = og_.vars[name].readAsVector<T>();
  }
}

template <typename T>
void ObsGroupFacade::getTypedMissingValue(const std::string &name,
                                          std::optional<T> &missingValue) const {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());

  if (variableType(name) != containerVariableType<T>)
    throw eckit::BadValue("Incorrect type specified for the missing value of variable " + name,
                          Here());

  auto fillValue = og_.vars[name].getFillValue();
  if (fillValue.set_)
    missingValue = ioda::detail::getFillValue<T>(fillValue);
}

template <typename T>
void ObsGroupFacade::setTypedVariableValues(const std::string &name, const std::vector<T> &values,
                                            MemoryLayout layout) {
  if (!isInitialized_)
    throw eckit::UserError("ObsGroup has not been initialized yet", Here());

  if (variableType(name) != containerVariableType<T>)
    throw eckit::BadValue("Attempted to assign values of an incorrect type to variable " + name,
                          Here());

  using Matrix = RowMajorMatrix<T>;
  Matrix fallbackRowMajorMatrix;
  Eigen::Map<const Matrix> rowMajorValuesView = rowMajorMatrixView(
    values, hasChannelAxis(name), layout, fallbackRowMajorMatrix);

  ioda::Variable variable = og_.vars[name];

  variable.write(gsl::span<const T>(rowMajorValuesView.data(), rowMajorValuesView.size()));
}

template <typename T>
Eigen::Map<const typename ObsGroupFacade::RowMajorMatrix<T>>
ObsGroupFacade::rowMajorMatrixView(const std::vector<T> &values, bool hasChannelAxis,
                                   MemoryLayout layout,
                                   RowMajorMatrix<T> &fallbackRowMajorMatrix) const {
  using Matrix = RowMajorMatrix<T>;

  const size_t numValues = values.size();

  if (hasChannelAxis) {
    const size_t numChannels = hasChannelAxis ? numberOfChannels() : 1;
    const size_t numLocations = numValues / numChannels;
    if (numChannels * numLocations != numValues)
      throw eckit::BadValue("Number of values is not divisible by the number of channels",
                            Here());

    const ExplicitMemoryLayout explicitLayout = explicitMemoryLayout(layout);
    if (explicitLayout == ExplicitMemoryLayout::RowMajor) {
      return Eigen::Map<const RowMajorMatrix<T>>(values.data(), numLocations, numChannels);
    } else {
      Eigen::Map<const ColumnMajorMatrix<T>> columnMajorValuesView(
        values.data(), numLocations, numChannels);
      fallbackRowMajorMatrix = columnMajorValuesView;
      return Eigen::Map<const RowMajorMatrix<T>>(fallbackRowMajorMatrix.data(),
                                                 numLocations, numChannels);
    }
  } else {
    return Eigen::Map<const Matrix>(values.data(), numValues, 1);
  }
}

}  // namespace Engines
}  // namespace ioda
