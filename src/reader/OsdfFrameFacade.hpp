#pragma once
/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <memory>
#include <optional>  // NOLINT(build/include_order): linter mis-identifies C++ header as C
#include <string>
#include <unordered_map>
#include <variant>   // NOLINT(build/include_order): linter mis-identifies C++ header as C
#include <vector>

#include "ioda/Engines/ContainerFacade.h"
#include "ioda/Layout.h"

namespace osdf
{
class FrameMetadata;
class IFrame;
}  // namespace osdf

namespace ioda {
namespace detail {
class DataLayoutPolicy;
}  // namespace detail

/// \brief Provides a ContainerFacade-compatible interface to a wrapped object derived from
/// osdf::IFrame.
class OsdfFrameFacade : public Engines::ContainerFacade {
 public:
  using ContainerOptions = Engines::ContainerOptions;
  using ContainerVariableType = Engines::ContainerVariableType;
  using ExplicitMemoryLayout = Engines::ExplicitMemoryLayout;
  using MemoryLayout = Engines::MemoryLayout;

  /// \brief Constructor.
  ///
  /// \param frame
  ///   The frame to be wrapped.
  /// \param metadata
  ///   Frame metadata (to be kept in sync with the frame's contents).
  OsdfFrameFacade(osdf::IFrame &frame, osdf::FrameMetadata &metadata);

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
                   bool hasChannelAxis, MemoryLayout layout,
                   const std::optional<std::string> &missingValue = std::nullopt) override;
  void addVariable(const std::string &name, const std::vector<char> &values,
                   bool hasChannelAxis, MemoryLayout layout = MemoryLayout::RowMajor,
                   const std::optional<char> &missingValue = std::nullopt) override;

  void removeVariable(const std::string &name) override;

  bool hasVariable(const std::string &name) const override;

  ContainerVariableType variableType(const std::string &name) const override;

  bool hasChannelAxis(const std::string &name) const override;

  void setVariableUnit(const std::string &name, const std::string &unit) override;

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

  /// \brief Return a reference to the wrapped frame.
  osdf::IFrame &frame();

  /// \brief Return a reference to metadata of the wrapped frame.
  osdf::FrameMetadata &metadata();

 private:
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

  std::string iodaVariableName(const std::string &name) const;

  bool iodaVariableHasChannelAxis(const std::string &iodaName) const;

  template <typename T>
  void addTypedVariable(const std::string &name, const std::vector<T> &values,
                        bool hasChannelAxis, MemoryLayout layout,
                        const std::optional<T> &missingValue);

  template <typename T>
  void getTypedVariableValues(const std::string &name, std::vector<T> &values,
                              MemoryLayout layout) const;

  template <typename T>
  void getTypedMissingValue(const std::string &name, std::optional<T> &missingValue) const;

  template <typename T>
  void getTypedIodaVariableMissingValue(const std::string &iodaName,
                                        std::optional<T> &missingValue) const;

  template <typename T>
  void setTypedVariableValues(const std::string &name, const std::vector<T> &values,
                              MemoryLayout layout);

  template <typename T>
  void setTypedIodaVariableValues(const std::string &iodaName, const std::vector<T> &values,
                                  const std::string &secondDimName, MemoryLayout layout,
                                  const std::optional<T> &missingValue, bool createNewColumns);

 private:
  osdf::IFrame &frame_;
  osdf::FrameMetadata &metadata_;
  bool isInitialized_ = false;
  std::shared_ptr<const detail::DataLayoutPolicy> dataLayoutPolicy_;
  ContainerOptions options_;
  std::unordered_map<std::string,
                     std::variant<std::optional<int>,
                                  std::optional<int64_t>,
                                  std::optional<float>,
                                  std::optional<std::string>,
                                  std::optional<char>
                                  >> missingValueByIodaName_;
};

}  // namespace ioda
