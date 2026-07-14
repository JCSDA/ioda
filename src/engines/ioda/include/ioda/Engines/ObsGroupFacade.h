#pragma once
/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ioda/Engines/ContainerFacade.h"
#include "ioda/ObsGroup.h"

namespace ioda {
namespace Engines {

/// \brief Provides a ContainerFacade-compatible interface to a wrapped ObsGroup object.
class ObsGroupFacade : public ContainerFacade {
public:
  /// \brief Constructor.
  ///
  /// \param group A Group that should be used to generate the wrapped ObsGroup.
  explicit ObsGroupFacade(Group group);

  void initialize(size_t numLocations, const std::optional<std::vector<int>> &channelIndices,
                  std::shared_ptr<const detail::DataLayoutPolicy> dataLayoutPolicy,
                  const ContainerOptions &options) override;

  void addDateTimeVariableToOptions(std::string dateTimeVariableName) override;

  int numberOfChannels() const override;

  void addVariable(const std::string &name, const std::vector<int> &values,
                   bool hasChannelAxis, MemoryLayout layout = MemoryLayout::RowMajor,
                   const std::optional<int> &missingValue = std::nullopt) override;

  void addVariable(const std::string &name, const std::vector<int64_t> &values,
                   bool hasChannelAxis, MemoryLayout layout = MemoryLayout::RowMajor,
                   const std::optional<int64_t> &missingValue = std::nullopt) override;

  void addVariable(const std::string &name, const std::vector<float> &values,
                   bool hasChannelAxis, MemoryLayout layout = MemoryLayout::RowMajor,
                   const std::optional<float> &missingValue = std::nullopt) override;

  void addVariable(const std::string &name, const std::vector<std::string> &values,
                   bool hasChannelAxis, MemoryLayout layout = MemoryLayout::RowMajor,
                   const std::optional<std::string> &missingValue = std::nullopt) override;

  void addVariable(const std::string &name, const std::vector<char> &values,
                   bool hasChannelAxis, MemoryLayout layout = MemoryLayout::RowMajor,
                   const std::optional<char> &missingValue = std::nullopt) override;

  void removeVariable(const std::string &name) override;

  bool hasVariable(const std::string &name) const override;

  ContainerVariableType variableType(const std::string &name) const override;

  bool hasChannelAxis(const std::string &name) const override;

  void setVariableUnit(const std::string &name, const std::string &unit) override ;

  void setVariableValues(const std::string &name, const std::vector<int> &values,
                         MemoryLayout layout = MemoryLayout::RowMajor) override;

  void setVariableValues(const std::string &name, const std::vector<int64_t> &values,
                         MemoryLayout layout = MemoryLayout::RowMajor) override;

  void setVariableValues(const std::string &name, const std::vector<float> &values,
                         MemoryLayout layout = MemoryLayout::RowMajor) override;

  void setVariableValues(const std::string &name, const std::vector<std::string> &values,
                         MemoryLayout layout = MemoryLayout::RowMajor) override;

  void setVariableValues(const std::string &name, const std::vector<char> &values,
                         MemoryLayout layout = MemoryLayout::RowMajor) override;

  std::vector<std::string> iodaVariableNames() const override;

  ExplicitMemoryLayout nativeMemoryLayout() const override;

  bool needsSourceLocationIndices() const override;

  /// \brief Return a reference to the wrapped ObsGroup.
  ObsGroup & obsGroup();

private:
  template <typename T>
  using RowMajorMatrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
  template <typename T>
  using ColumnMajorMatrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;

  void getVariableValues(const std::string &name, std::vector<int> &values,
                         MemoryLayout layout) const override;
  void getVariableValues(const std::string &name, std::vector<int64_t> &values,
                         MemoryLayout layout) const override;
  void getVariableValues(const std::string &name, std::vector<float> &values,
                         MemoryLayout layout) const override;
  void getVariableValues(const std::string &name, std::vector<std::string> &values,
                         MemoryLayout layout) const override;
  void getVariableValues(const std::string &name, std::vector<char> &values,
                         MemoryLayout layout) const override;

  void getMissingValue(const std::string &name,
                       std::optional<int> &missingValue) const override;
  void getMissingValue(const std::string &name,
                       std::optional<int64_t> &missingValue) const override;
  void getMissingValue(const std::string &name,
                       std::optional<float> &missingValue) const override;
  void getMissingValue(const std::string &name,
                       std::optional<std::string> &missingValue) const override;
  void getMissingValue(const std::string &name,
                       std::optional<char> &missingValue) const override;

  template <typename T>
  void addTypedVariable(const std::string &name, const std::vector<T> &values,
                        bool hasChannelAxis, MemoryLayout layout,
                        const std::optional<T> &missingValue);

  template <typename T>
  void getTypedVariableValues(const std::string &name, std::vector<T> &values,
                              MemoryLayout layout) const ;

  template <typename T>
  void getTypedMissingValue(const std::string &name, std::optional<T> &missingValue) const;

  template <typename T>
  void setTypedVariableValues(const std::string &name, const std::vector<T> &values,
                              MemoryLayout layout);

  template <typename T>
  Eigen::Map<const RowMajorMatrix<T>>
  rowMajorMatrixView(const std::vector<T> &values, bool hasChannelAxis, MemoryLayout layout,
                     RowMajorMatrix<T> &fallbackRowMajorMatrix) const;

private:
  Group group_;
  ContainerOptions options_;
  bool isInitialized_ = false;
  ObsGroup og_;
  int numChannels_ = 1;
};

}  // namespace Engines
}  // namespace ioda
