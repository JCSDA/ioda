#pragma once
/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/ContainerVariableType.h"

#include <memory>
#include <optional>  // NOLINT(build/include_order): linter mis-identifies C++ header as C
#include <string>
#include <vector>

namespace ioda {

namespace detail {
class DataLayoutPolicy;
}

namespace Engines {

/// \brief Memory layout of a 2D array with variable values.
enum class ExplicitMemoryLayout {
  /// \brief Values ordered first by channel and then by location.
  RowMajor,
  /// \brief Values ordered first by location and then by channel.
  ColumnMajor
};

/// \brief Memory layout of a 2D array with variable values. Supports a special `Native` mode.
enum class MemoryLayout {
  /// \brief Values ordered first by channel and then by location.
  RowMajor,
  /// \brief Values ordered first by location and then by channel.
  ColumnMajor,
  /// \brief Either row- or column-major depending on which of these layouts is used natively by the
  /// container (or at least its public interface).
  ///
  /// \see ContainerFacade::nativeMemoryLayout().
  Native
};

struct ContainerOptions {
  /// \brief The epoch to use for DateTime variables.
  std::string epoch = "seconds since 1970-01-01T00:00:00Z";
  /// \brief List of variables of dateTime type, used to add epoch to units.
  std::vector<std::string> dateTimeVariables = {};
};

/// \brief A common interface to a wrapped container such as a ioda::ObsGroup or an osdf::IFrame.
///
/// \note Unless specified otherwise, all variable names passed and returned by this interface
/// are "native" names used in the container, not
class ContainerFacade {
 public:
  virtual ~ContainerFacade() {}

  /// \brief Initialize the wrapped container.
  ///
  /// \param numLocations
  ///   Number of locations to be stored in the container.
  /// \param channelIndices
  ///   Channel indices, or std::nullopt if channels will not be used.
  /// \param dataLayoutPolicy
  ///   An object mapping user-specified variable names to canonical names used elsewhere in ioda.
  /// \param options
  ///   Additional options influencing the container's behavior.
  ///
  /// \note Unless noted otherwise, all variable names taken by ContainerFacade member functions
  /// will be transformed by the dataLayoutPolicy to canonical ioda names before being passed to the
  /// container, or the container will be instructed to perform this name mapping on its own.
  /// In other words, data stored in the container will be labelled with the canonical ioda names.
  virtual void initialize(size_t numLocations,
                          const std::optional<std::vector<int>> &channelIndices,
                          std::shared_ptr<const detail::DataLayoutPolicy> dataLayoutPolicy,
                          const ContainerOptions &options) = 0;

  /// \brief Add a variable to the list of dateTime variables stored in options, for use in adding units.
  ///
  /// \param dateTimeVariableName
  ///   Variable of dateTime type to be added to options.datetimevariables
  virtual void addDateTimeVariableToOptions(std::string dateTimeVariableName) = 0;

  /// \brief The number of locations stored in the container (equal to the number of rows for the osdf)
  virtual int numberOfLocations() const = 0;

  /// \brief The number of channels specified during container initialization, or 0 if no 
  /// indices were specified.
  virtual int numberOfChannels() const = 0;

  /// \brief Returns a vector of the channels specified during container initialization
  virtual std::vector<int> channelNumbers() const = 0;

  /// \brief Returns the units of the specified variable
  virtual std::string variableUnits(const std::string &name) const = 0;

  /// \brief Add a variable to the wrapped container.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  /// \param values
  ///   Initial values of the new variable.
  /// \param hasChannelAxis
  ///   Whether to equip the new variable with a channel axis (in addition to a location axis).
  /// \param layout
  ///   The memory layout of `values`. Matters only for variables with a channel axis.
  /// \param missingValue
  ///   The value to be interpreted as a missing value indicator, or `std::nullopt` if no value
  ///   should have such an interpretation.
  virtual void addVariable(const std::string &name, const std::vector<int> &values,
                           bool hasChannelAxis,
                           MemoryLayout layout = MemoryLayout::RowMajor,
                           const std::optional<int> &missingValue = std::nullopt) = 0;
  /// \overload
  virtual void addVariable(const std::string &name, const std::vector<int64_t> &values,
                           bool hasChannelAxis,
                           MemoryLayout layout = MemoryLayout::RowMajor,
                           const std::optional<int64_t> &missingValue = std::nullopt) = 0;
  /// \overload
  virtual void addVariable(const std::string &name, const std::vector<float> &values,
                           bool hasChannelAxis,
                           MemoryLayout layout = MemoryLayout::RowMajor,
                           const std::optional<float> &missingValue = std::nullopt) = 0;
  /// \overload
  virtual void addVariable(const std::string &name, const std::vector<std::string> &values,
                           bool hasChannelAxis,
                           MemoryLayout layout = MemoryLayout::RowMajor,
                           const std::optional<std::string> &missingValue = std::nullopt) = 0;
  /// \overload
  virtual void addVariable(const std::string &name, const std::vector<char> &values,
                           bool hasChannelAxis,
                           MemoryLayout layout = MemoryLayout::RowMajor,
                           const std::optional<char> &missingValue = std::nullopt) = 0;

  /// \brief Remove a variable from the wrapped container.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  virtual void removeVariable(const std::string &name) = 0;

  /// \brief Return whether the wrapped container has a variable with a given name.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  virtual bool hasVariable(const std::string &name) const = 0;

  /// \brief Return the type of a given variable.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  virtual ContainerVariableType variableType(const std::string &name) const = 0;

  /// \brief Whether a given variable has a channel axis.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  virtual bool hasChannelAxis(const std::string &name) const = 0;

  /// \brief Set the unit of values stored in a given variable.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  /// \param unit
  ///   Unit.
  virtual void setVariableUnit(const std::string &name, const std::string &unit) = 0;

  /// \brief Return the values of a variable.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  /// \param layout
  ///   Determines how the returned values will be arranged in memory.
  template <typename T>
  std::vector<T> variableValues(const std::string &name,
                                MemoryLayout layout = MemoryLayout::RowMajor) const {
    std::vector<T> values;
    getVariableValues(name, values, layout);
    return values;
  }

  /// \brief Return the value serving as a missing value indicator in a given variable, or
  /// std::nullopt if there is none.
  template <typename T>
  std::optional<T> missingValue(const std::string &name) const {
    std::optional<T> mv;
    getMissingValue(name, mv);
    return mv;
  }

  /// \brief Set the values of a variable.
  ///
  /// \param name
  ///   Variable name. (See note in initialize().)
  /// \param values
  ///   Values to be set.
  /// \param layout
  ///   Memory layout of `values`.
  virtual void setVariableValues(const std::string &name, const std::vector<int> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) = 0;
  virtual void setVariableValues(const std::string &name, const std::vector<int64_t> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) = 0;
  virtual void setVariableValues(const std::string &name, const std::vector<float> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) = 0;
  virtual void setVariableValues(const std::string &name, const std::vector<std::string> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) = 0;
  virtual void setVariableValues(const std::string &name, const std::vector<char> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) = 0;

  /// \brief Return the list of *canonical* names of variables stored in the container.
  ///
  /// These are names produced by the mapping associated with the data layout policy passed to
  /// initialize().
  virtual std::vector<std::string> iodaVariableNames() const = 0;

  /// \brief Return the explicit memory layout corresponding to the MemoryLayout::Native.
  virtual ExplicitMemoryLayout nativeMemoryLayout() const = 0;

  /// \brief Convert a MemoryLayout to an ExplicitMemoryLayout.
  ///
  /// MemoryLayout::Native is converted to the value returned by nativeMemoryLayout(); other values
  /// are left unchanged.
  ExplicitMemoryLayout explicitMemoryLayout(MemoryLayout layout) const {
    switch (layout) {
      case MemoryLayout::RowMajor:
        return ExplicitMemoryLayout::RowMajor;
      case MemoryLayout::ColumnMajor:
        return ExplicitMemoryLayout::ColumnMajor;
      case MemoryLayout::Native:
      default:
        return nativeMemoryLayout();
    }
  }

  /// \brief Whether the wrapped container needs a sourceLocationIndices column.
  virtual bool needsSourceLocationIndices() const = 0;

 private:
  virtual void getVariableValues(const std::string &name, std::vector<int> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) const = 0;
  virtual void getVariableValues(const std::string &name, std::vector<int64_t> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) const = 0;
  virtual void getVariableValues(const std::string &name, std::vector<float> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) const = 0;
  virtual void getVariableValues(const std::string &name, std::vector<std::string> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) const = 0;
  virtual void getVariableValues(const std::string &name, std::vector<char> &values,
                                 MemoryLayout layout = MemoryLayout::RowMajor) const = 0;

  virtual void getMissingValue(const std::string &name,
                               std::optional<int> &missingValue) const = 0;
  virtual void getMissingValue(const std::string &name,
                               std::optional<int64_t> &missingValue) const = 0;
  virtual void getMissingValue(const std::string &name,
                               std::optional<float> &missingValue) const = 0;
  virtual void getMissingValue(const std::string &name,
                               std::optional<std::string> &missingValue) const = 0;
  virtual void getMissingValue(const std::string &name,
                               std::optional<char> &missingValue) const = 0;
};

}  // namespace Engines
}  // namespace ioda
