/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/runs/Run.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/containers/FrameCols.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ObsGroupFacade.h"
#include "ioda/Layout.h"
#include "ioda/ObsGroup.h"
#include "ioda/reader/OsdfFrameFacade.hpp"

// -----------------------------------------------------------------------------

using ioda::detail::DataLayoutPolicy;
using ioda::Engines::ContainerFacade;
using ioda::Engines::ContainerOptions;
using ioda::Engines::ContainerVariableType;
using ioda::Engines::ExplicitMemoryLayout;
using ioda::Engines::MemoryLayout;
using ioda::Engines::ObsGroupFacade;
using ioda::OsdfFrameFacade;

class WrappedContainer {
 public:
  virtual ~WrappedContainer() {}

  virtual ContainerFacade & facade() = 0;
};

class WrappedObsGroup : public WrappedContainer {
 public:
  WrappedObsGroup()
      : group_(ioda::Engines::constructBackend(ioda::Engines::BackendNames::ObsStore, {})),
        facade_(group_)
  {}

  ContainerFacade & facade() override { return facade_; }

 private:
  ioda::Group group_;
  ObsGroupFacade facade_;
};

class WrappedOsdfFrame : public WrappedContainer {
 public:
  WrappedOsdfFrame() : facade_(frame_, metadata_) {}

  ContainerFacade & facade() override { return facade_; }

 private:
  osdf::FrameCols frame_;
  osdf::FrameMetadata metadata_;
  OsdfFrameFacade facade_;
};

std::shared_ptr<const DataLayoutPolicy> dataLayoutPolicy() {
  static std::shared_ptr<const DataLayoutPolicy> policy = DataLayoutPolicy::generate(
    DataLayoutPolicy::Policies::ObsGroupODB,
    ::test::TestEnvironment::config().getString("mapping file"),
    {"Location", "Channel"});
  return policy;
}

void initializeContainerWithoutChannels(ContainerFacade &facade) {
  const int numLocations = 4;
  facade.initialize(numLocations, std::nullopt /*channelIndices*/,
                    dataLayoutPolicy(), ContainerOptions());
}

void initializeContainerWithChannels(ContainerFacade &facade) {
  const int numLocations = 4;
  const std::vector<int> channelIndices({3, 5, 8});
  facade.initialize(numLocations, channelIndices, dataLayoutPolicy(), ContainerOptions());
}

void testUninitialisedContainer(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  std::unique_ptr<WrappedContainer> container = makeContainer();
  ContainerFacade &facade = container->facade();

  EXPECT_EQUAL(facade.numberOfChannels(), 1);
  EXPECT_THROWS(facade.addVariable("initial_obsvalue/2", std::vector<float>({300.f}),
                                   false /*hasChannelAxis*/));
  EXPECT_THROWS(facade.removeVariable("initial_obsvalue/2"));
  EXPECT_NOT(facade.hasVariable("initial_obsvalue/2"));
  EXPECT_THROWS(facade.variableType("initial_obsvalue/2"));
  EXPECT_THROWS(facade.hasChannelAxis("initial_obsvalue/2"));
  EXPECT_THROWS(facade.setVariableUnit("initial_obsvalue/2", "millikelvin"));
  EXPECT_THROWS(facade.variableValues<float>("initial_obsvalue/2"));
  EXPECT_THROWS(facade.missingValue<float>("initial_obsvalue/2"));
  EXPECT_THROWS(facade.setVariableValues("initial_obsvalue/2", std::vector<float>({300.f})));
  EXPECT(facade.explicitMemoryLayout(MemoryLayout::Native) ==
         facade.nativeMemoryLayout());
  EXPECT(facade.explicitMemoryLayout(MemoryLayout::RowMajor) ==
         ExplicitMemoryLayout::RowMajor);
  EXPECT(facade.explicitMemoryLayout(MemoryLayout::ColumnMajor) ==
         ExplicitMemoryLayout::ColumnMajor);
}

CASE("testUninitialisedContainer_ObsGroup") {
  testUninitialisedContainer(std::make_unique<WrappedObsGroup>);
}

CASE("testUninitialisedContainer_OsdfFrame") {
  testUninitialisedContainer(std::make_unique<WrappedOsdfFrame>);
}

void testNumberOfChannels(const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  // Section 1: Container initialised without channels
  {
    std::unique_ptr<WrappedContainer> container = makeContainer();
    ContainerFacade &facade = container->facade();
    initializeContainerWithoutChannels(facade);
    EXPECT_EQUAL(facade.numberOfChannels(), 1);
  }

  // Section 2: Container initialised with channels
  {
    std::unique_ptr<WrappedContainer> container = makeContainer();
    ContainerFacade &facade = container->facade();
    initializeContainerWithChannels(facade);
    EXPECT_EQUAL(facade.numberOfChannels(), 3);
  }
}

CASE("testNumberOfChannels_ObsGroup") {
  testNumberOfChannels(std::make_unique<WrappedObsGroup>);
}

CASE("testNumberOfChannels_OsdfFrame") {
  testNumberOfChannels(std::make_unique<WrappedOsdfFrame>);
}

void testVariableAdditionAndRemoval(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  // Section 1: Container initialised without channels
  {
    std::unique_ptr<WrappedContainer> container = makeContainer();
    ContainerFacade &facade = container->facade();
    initializeContainerWithoutChannels(facade);
    EXPECT_THROWS(facade.addVariable("initial_obsvalue/2",
                                     std::vector<float>({301.f, 302.f, 303.f, 304.f}),
                                     true /*hasChannelAxis*/));
    EXPECT_NO_THROW(facade.addVariable("initial_obsvalue/2",
                                       std::vector<float>({301.f, 302.f, 303.f, 304.f}),
                                       false /*hasChannelAxis*/));
  }

  // Section 2: Container initialised with channels
  {
    std::unique_ptr<WrappedContainer> container = makeContainer();
    ContainerFacade &facade = container->facade();
    initializeContainerWithChannels(facade);

    facade.addVariable("initial_obsvalue/3",
                       std::vector<float>({0.f, 1.f, 2.f,
                                           3.f, 4.f, 5.f,
                                           6.f, 7.f, 8.f,
                                           9.f, 10.f, 11.f}),
                       true /*hasChannelAxis*/, MemoryLayout::RowMajor);
    facade.addVariable("rain_flag", std::vector<int>({1, 2, 3, 4}), false /*hasChannelAxis*/);
    facade.addVariable("level", std::vector<int64_t>({-1, -2, -3, -4}), false /*hasChannelAxis*/);
    facade.addVariable("lidar_class", std::vector<char>({'a', 'b', 'c', 'd'}),
                       false /*hasChannelAxis*/);
    facade.addVariable("satellite_identifier", {"ABC", "DEF", "GHI", "JKL"},
                       false /*hasChannelAxis*/);
    EXPECT(facade.hasVariable("initial_obsvalue/3"));
    EXPECT(facade.hasChannelAxis("initial_obsvalue/3"));
    EXPECT(facade.hasVariable("rain_flag"));
    EXPECT_NOT(facade.hasChannelAxis("rain_flag"));
    EXPECT(facade.hasVariable("level"));
    EXPECT(facade.hasVariable("lidar_class"));
    EXPECT(facade.hasVariable("satellite_identifier"));
    EXPECT_NOT(facade.hasVariable("lon"));
    EXPECT_NOT(facade.hasVariable("random_variable"));

    facade.removeVariable("level");
    EXPECT(facade.hasVariable("initial_obsvalue/3"));
    EXPECT(facade.hasVariable("rain_flag"));
    EXPECT_NOT(facade.hasVariable("level"));
    EXPECT(facade.hasVariable("lidar_class"));
    EXPECT(facade.hasVariable("satellite_identifier"));
    EXPECT_NOT(facade.hasVariable("lon"));
    EXPECT_NOT(facade.hasVariable("random_variable"));

    facade.removeVariable("initial_obsvalue/3");
    EXPECT_NOT(facade.hasVariable("initial_obsvalue/3"));
    EXPECT_NOT(facade.hasChannelAxis("initial_obsvalue/3"));
    EXPECT(facade.hasVariable("rain_flag"));
    EXPECT(facade.hasVariable("lidar_class"));
    EXPECT(facade.hasVariable("satellite_identifier"));
  }
}

CASE("testVariableAdditionAndRemoval_ObsGroup") {
  testVariableAdditionAndRemoval(std::make_unique<WrappedObsGroup>);
}

CASE("testVariableAdditionAndRemoval_OsdfFrame") {
  testVariableAdditionAndRemoval(std::make_unique<WrappedOsdfFrame>);
}

void testVariableType(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  std::unique_ptr<WrappedContainer> container = makeContainer();
  ContainerFacade &facade = container->facade();
  initializeContainerWithChannels(facade);

  EXPECT_THROWS(facade.variableType("initial_obsvalue/3"));
  facade.addVariable("initial_obsvalue/3",
                     std::vector<float>({0.f, 1.f, 2.f,
                                         3.f, 4.f, 5.f,
                                         6.f, 7.f, 8.f,
                                         9.f, 10.f, 11.f}),
                     true /*hasChannelAxis*/, MemoryLayout::RowMajor);
  facade.addVariable("rain_flag", std::vector<int>({1, 2, 3, 4}), false /*hasChannelAxis*/);
  facade.addVariable("level", std::vector<int64_t>({-1, -2, -3, -4}), false /*hasChannelAxis*/);
  facade.addVariable("lidar_class", std::vector<char>({'a', 'b', 'c', 'd'}),
                     false /*hasChannelAxis*/);
  facade.addVariable("satellite_identifier", {"ABC", "DEF", "GHI", "JKL"},
                     false /*hasChannelAxis*/);
  EXPECT(facade.variableType("initial_obsvalue/3") == ContainerVariableType::Float);
  EXPECT(facade.variableType("rain_flag") == ContainerVariableType::Int);
  EXPECT(facade.variableType("level") == ContainerVariableType::Int64);
  EXPECT(facade.variableType("lidar_class") == ContainerVariableType::Char);
  EXPECT(facade.variableType("satellite_identifier") == ContainerVariableType::String);
}

CASE("testVariableType_ObsGroup") {
  testVariableType(std::make_unique<WrappedObsGroup>);
}

CASE("testVariableType_OsdfFrame") {
  testVariableType(std::make_unique<WrappedOsdfFrame>);
}

void testHasChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  // Section 1: Container initialised without channels
  {
    std::unique_ptr<WrappedContainer> container = makeContainer();
    ContainerFacade &facade = container->facade();
    initializeContainerWithoutChannels(facade);
    EXPECT_NOT(facade.hasChannelAxis("initial_obsvalue/3"));
    facade.addVariable("initial_obsvalue/3",
                       std::vector<float>({0.f, 1.f, 2.f, 3.f}),
                       false /*hasChannelAxis*/);
    EXPECT_NOT(facade.hasChannelAxis("initial_obsvalue/3"));
  }

  // Section 2: Container initialised with channels
  {
    std::unique_ptr<WrappedContainer> container = makeContainer();
    ContainerFacade &facade = container->facade();
    initializeContainerWithChannels(facade);
    EXPECT_NOT(facade.hasChannelAxis("initial_obsvalue/3"));
    facade.addVariable("initial_obsvalue/3",
                       std::vector<float>({0.f, 1.f, 2.f,
                                           3.f, 4.f, 5.f,
                                           6.f, 7.f, 8.f,
                                           9.f, 10.f, 11.f}),
                       true /*hasChannelAxis*/, MemoryLayout::RowMajor);
    facade.addVariable("rain_flag", std::vector<int>({1, 2, 3, 4}), false /*hasChannelAxis*/);
    EXPECT(facade.hasChannelAxis("initial_obsvalue/3"));
    EXPECT_NOT(facade.hasChannelAxis("rain_flag"));
  }
}

CASE("testHasChannelAxis_ObsGroup") {
  testHasChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testHasChannelAxis_OsdfFrame") {
  testHasChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testGetVariableValuesWithoutChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  for (bool initializeWithChannels : {false, true})
    for (MemoryLayout memoryLayoutOnWrite : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                             MemoryLayout::Native})
      for (MemoryLayout memoryLayoutOnRead : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                              MemoryLayout::Native})
      {
        std::unique_ptr<WrappedContainer> container = makeContainer();
        ContainerFacade &facade = container->facade();
        if (initializeWithChannels)
          initializeContainerWithoutChannels(facade);
        else
          initializeContainerWithChannels(facade);

        // Float variable
        {
          const std::string name = "initial_obsvalue/3";
          const std::vector<float> writtenValues({0.f, 1.f, 2.f, 3.f});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          const std::vector<float> readValues = facade.variableValues<float>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<float>(name) == std::nullopt);
          // Try to read values into a vector of an incorrect type.
          EXPECT_THROWS(facade.variableValues<int>(name, memoryLayoutOnRead));
        }

        // Int variable
        {
          const std::string name = "rain_flag";
          const std::vector<int> writtenValues({1, 2, 3, 4});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          const std::vector<int> readValues = facade.variableValues<int>(name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int>(name) == std::nullopt);
          // Try to read values into a vector of an incorrect type.
          EXPECT_THROWS(facade.variableValues<float>(name, memoryLayoutOnRead));
        }

        // Int64_t variable
        {
          const std::string name = "level";
          const std::vector<int64_t> writtenValues({-1, -2, -3, -4});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          const std::vector<int64_t> readValues = facade.variableValues<int64_t>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int64_t>(name) == std::nullopt);
          // Try to read values into a vector of an incorrect type.
          EXPECT_THROWS(facade.variableValues<std::string>(name, memoryLayoutOnRead));
        }

        // String variable
        {
          const std::string name = "satellite_identifier";
          const std::vector<std::string> writtenValues({"ABC", "DEF", "GHI", "JKL"});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          const std::vector<std::string> readValues = facade.variableValues<std::string>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          // Try to read values into a vector of an incorrect type.
          EXPECT_THROWS(facade.variableValues<char>(name, memoryLayoutOnRead));
        }

        // Char variable
        {
          const std::string name = "lidar_class";
          const std::vector<char> writtenValues({'a', 'b', 'c', 'd'});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          const std::vector<char> readValues = facade.variableValues<char>(
            name, memoryLayoutOnRead);
          EXPECT(facade.missingValue<char>(name) == std::nullopt);
          EXPECT_EQUAL(writtenValues, readValues);
          // Try to read values into a vector of an incorrect type.
          EXPECT_THROWS(facade.variableValues<float>(name, memoryLayoutOnRead));
        }
      }
}

CASE("testGetVariableValuesWithoutChannelAxis_ObsGroup") {
  testGetVariableValuesWithoutChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testGetVariableValuesWithoutChannelAxis_OsdfFrame") {
  testGetVariableValuesWithoutChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testGetVariableValuesWithChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  std::unique_ptr<WrappedContainer> container = makeContainer();
  ContainerFacade &facade = container->facade();
  initializeContainerWithChannels(facade);
  // Rows = locations, columns = channels
  const std::vector<float> rowMajorValues({300.f, 301.f, 302.f,
                                           303.f, 304.f, 305.f,
                                           306.f, 307.f, 308.f,
                                           309.f, 310.f, 311.f});
  const std::vector<float> colMajorValues({300.f, 303.f, 306.f, 309.f,
                                           301.f, 304.f, 307.f, 310.f,
                                           302.f, 305.f, 308.f, 311.f});
  facade.addVariable("initial_obsvalue/2", rowMajorValues, true /*hasChannelAxis*/,
                     MemoryLayout::RowMajor);
  facade.addVariable("initial_obsvalue/3", colMajorValues, true /*hasChannelAxis*/,
                     MemoryLayout::ColumnMajor);
  facade.addVariable("initial_obsvalue/4", rowMajorValues, true /*hasChannelAxis*/,
                     MemoryLayout::Native);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/4", MemoryLayout::Native),
               rowMajorValues);
  EXPECT(facade.missingValue<float>("initial_obsvalue/2") == std::nullopt);
  // Try to read values into a vector of an incorrect type.
  EXPECT_THROWS(facade.variableValues<int>("initial_obsvalue/2", MemoryLayout::RowMajor));
}

CASE("testGetVariableValuesWithChannelAxis_ObsGroup") {
  testGetVariableValuesWithChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testGetVariableValuesWithChannelAxis_OsdfFrame") {
  testGetVariableValuesWithChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testGetVariableValuesWithMissingValuesWithoutChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  for (bool initializeWithChannels : {false, true})
    for (MemoryLayout memoryLayoutOnWrite : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                             MemoryLayout::Native})
      for (MemoryLayout memoryLayoutOnRead : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                              MemoryLayout::Native})
      {
        std::unique_ptr<WrappedContainer> container = makeContainer();
        ContainerFacade &facade = container->facade();
        if (initializeWithChannels)
          initializeContainerWithoutChannels(facade);
        else
          initializeContainerWithChannels(facade);

        // Float variable
        {
          const std::string name = "initial_obsvalue/3";
          const float missingValue = -10;
          const std::vector<float> writtenValues({0.f, 1.f, missingValue, 3.f});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          const std::vector<float> readValues = facade.variableValues<float>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<float>(name) == missingValue);
          // Try to invoke missingValue() for an incorrect type.
          EXPECT_THROWS(facade.missingValue<int>(name));
        }

        // Int variable
        {
          const std::string name = "rain_flag";
          const int missingValue = -10;
          const std::vector<int> writtenValues({1, 2, missingValue, 4});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          const std::vector<int> readValues = facade.variableValues<int>(name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int>(name) == missingValue);
          // Try to invoke missingValue() for an incorrect type.
          EXPECT_THROWS(facade.missingValue<float>(name));
        }

        // Int64_t variable
        {
          const std::string name = "level";
          const int64_t missingValue = -10;
          const std::vector<int64_t> writtenValues({-1, -2, missingValue, -4});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          const std::vector<int64_t> readValues = facade.variableValues<int64_t>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int64_t>(name) == missingValue);
          // Try to invoke missingValue() for an incorrect type.
          EXPECT_THROWS(facade.missingValue<int>(name));
        }

        // String variable
        {
          const std::string name = "satellite_identifier";
          const std::string missingValue = "NONE";
          const std::vector<std::string> writtenValues({"ABC", "DEF", missingValue, "JKL"});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          const std::vector<std::string> readValues = facade.variableValues<std::string>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<std::string>(name) == missingValue);
          // Try to invoke missingValue() for an incorrect type.
          EXPECT_THROWS(facade.missingValue<int>(name));
        }

        // Char variable
        {
          const std::string name = "lidar_class";
          const char missingValue = 'z';
          const std::vector<char> writtenValues({'a', 'b', 'c', 'd'});

          facade.addVariable(name, writtenValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          const std::vector<char> readValues = facade.variableValues<char>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<char>(name) == missingValue);
          // Try to invoke missingValue() for an incorrect type.
          EXPECT_THROWS(facade.missingValue<int>(name));
        }
      }
}

CASE("testGetVariableValuesWithMissingValuesWithoutChannelAxis_ObsGroup") {
  testGetVariableValuesWithMissingValuesWithoutChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testGetVariableValuesWithMissingValuesWithoutChannelAxis_OsdfFrame") {
  testGetVariableValuesWithMissingValuesWithoutChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testGetVariableValuesWithMissingValuesWithChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  std::unique_ptr<WrappedContainer> container = makeContainer();
  ContainerFacade &facade = container->facade();
  initializeContainerWithChannels(facade);
  const float missingValue = -10.f;
  // Rows = locations, columns = channels
  const std::vector<float> rowMajorValues({300.f, 301.f, missingValue,
                                           303.f, 304.f, 305.f,
                                           306.f, missingValue, 308.f,
                                           309.f, 310.f, 311.f});
  const std::vector<float> colMajorValues({300.f, 303.f, 306.f, 309.f,
                                           301.f, 304.f, missingValue, 310.f,
                                           missingValue, 305.f, 308.f, 311.f});
  facade.addVariable("initial_obsvalue/2", rowMajorValues, true /*hasChannelAxis*/,
                     MemoryLayout::RowMajor, missingValue);
  facade.addVariable("initial_obsvalue/3", colMajorValues, true /*hasChannelAxis*/,
                     MemoryLayout::ColumnMajor, missingValue);
  facade.addVariable("initial_obsvalue/4", rowMajorValues, true /*hasChannelAxis*/,
                     MemoryLayout::Native, missingValue);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/4", MemoryLayout::Native),
               rowMajorValues);
  EXPECT(facade.missingValue<float>("initial_obsvalue/2") == missingValue);
  EXPECT(facade.missingValue<float>("initial_obsvalue/3") == missingValue);
  EXPECT(facade.missingValue<float>("initial_obsvalue/4") == missingValue);
  // Try to invoke missingValue() for an incorrect type.
  EXPECT_THROWS(facade.missingValue<int>("initial_obsvalue/2"));
}

CASE("testGetVariableValuesWithMissingValuesWithChannelAxis_ObsGroup") {
  testGetVariableValuesWithMissingValuesWithChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testGetVariableValuesWithMissingValuesWithChannelAxis_OsdfFrame") {
  testGetVariableValuesWithMissingValuesWithChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testSetVariableValuesWithoutChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  const size_t numLocations = 4;
  for (bool initializeWithChannels : {false, true})
    for (MemoryLayout memoryLayoutOnWrite : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                             MemoryLayout::Native})
      for (MemoryLayout memoryLayoutOnRead : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                              MemoryLayout::Native})
      {
        std::unique_ptr<WrappedContainer> container = makeContainer();
        ContainerFacade &facade = container->facade();
        if (initializeWithChannels)
          initializeContainerWithoutChannels(facade);
        else
          initializeContainerWithChannels(facade);

        // Float variable
        {
          const std::string name = "initial_obsvalue/3";
          const std::vector<float> nullValues(numLocations);
          const std::vector<float> writtenValues({0.f, 1.f, 2.f, 3.f});

          // Try to assign values to a non-existing variable.
          EXPECT_THROWS(facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite));
          // Create a variable, then set its values.
          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<float> readValues = facade.variableValues<float>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<float>(name) == std::nullopt);
          // Try to assign values of an incorrect type.
          EXPECT_THROWS(facade.setVariableValues(name, std::vector<int>(numLocations),
                                                 memoryLayoutOnWrite));
        }

        // Int variable
        {
          const std::string name = "rain_flag";
          const std::vector<int> nullValues(numLocations);
          const std::vector<int> writtenValues({1, 2, 3, 4});

          // Try to assign values to a non-existing variable.
          EXPECT_THROWS(facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite));
          // Create a variable, then set its values.
          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<int> readValues = facade.variableValues<int>(name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int>(name) == std::nullopt);
          // Try to assign values of an incorrect type.
          EXPECT_THROWS(facade.setVariableValues(name, std::vector<float>(numLocations),
                                                 memoryLayoutOnWrite));
        }

        // Int64_t variable
        {
          const std::string name = "level";
          const std::vector<int64_t> nullValues(numLocations);
          const std::vector<int64_t> writtenValues({-1, -2, -3, -4});

          // Try to assign values to a non-existing variable.
          EXPECT_THROWS(facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite));
          // Create a variable, then set its values.
          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<int64_t> readValues = facade.variableValues<int64_t>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int64_t>(name) == std::nullopt);
          // Try to assign values of an incorrect type.
          EXPECT_THROWS(facade.setVariableValues(name, std::vector<std::string>(numLocations),
                                                 memoryLayoutOnWrite));
        }

        // String variable
        {
          const std::string name = "satellite_identifier";
          const std::vector<std::string> nullValues(numLocations);
          const std::vector<std::string> writtenValues({"ABC", "DEF", "GHI", "JKL"});

          // Try to assign values to a non-existing variable.
          EXPECT_THROWS(facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite));
          // Create a variable, then set its values.
          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<std::string> readValues = facade.variableValues<std::string>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<std::string>(name) == std::nullopt);
          // Try to assign values of an incorrect type.
          EXPECT_THROWS(facade.setVariableValues(name, std::vector<int>(numLocations),
                                                 memoryLayoutOnWrite));
        }

        // Char variable
        {
          const std::string name = "lidar_class";
          const std::vector<char> nullValues(numLocations);
          const std::vector<char> writtenValues({'a', 'b', 'c', 'd'});

          // Try to assign values to a non-existing variable.
          EXPECT_THROWS(facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite));
          // Create a variable, then set its values.
          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<char> readValues = facade.variableValues<char>(
            name, memoryLayoutOnRead);
          EXPECT(facade.missingValue<char>(name) == std::nullopt);
          EXPECT_EQUAL(writtenValues, readValues);
          // Try to assign values of an incorrect type.
          EXPECT_THROWS(facade.setVariableValues(name, std::vector<float>(numLocations),
                                                 memoryLayoutOnWrite));
        }
      }
}

CASE("testSetVariableValuesWithoutChannelAxis_ObsGroup") {
  testSetVariableValuesWithoutChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testSetVariableValuesWithoutChannelAxis_OsdfFrame") {
  testSetVariableValuesWithoutChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testSetVariableValuesWithChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  std::unique_ptr<WrappedContainer> container = makeContainer();
  ContainerFacade &facade = container->facade();
  initializeContainerWithChannels(facade);
  // Rows = locations, columns = channels
  const std::vector<float> nullValues(12);
  const std::vector<float> rowMajorValues({300.f, 301.f, 302.f,
                                           303.f, 304.f, 305.f,
                                           306.f, 307.f, 308.f,
                                           309.f, 310.f, 311.f});
  const std::vector<float> colMajorValues({300.f, 303.f, 306.f, 309.f,
                                           301.f, 304.f, 307.f, 310.f,
                                           302.f, 305.f, 308.f, 311.f});
  facade.addVariable("initial_obsvalue/2", nullValues, true /*hasChannelAxis*/,
                     MemoryLayout::RowMajor);
  facade.addVariable("initial_obsvalue/3", nullValues, true /*hasChannelAxis*/,
                     MemoryLayout::ColumnMajor);
  facade.addVariable("initial_obsvalue/4", nullValues, true /*hasChannelAxis*/,
                     MemoryLayout::Native);
  facade.setVariableValues("initial_obsvalue/2", rowMajorValues, MemoryLayout::RowMajor);
  facade.setVariableValues("initial_obsvalue/3", colMajorValues, MemoryLayout::ColumnMajor);
  facade.setVariableValues("initial_obsvalue/4", rowMajorValues, MemoryLayout::Native);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/4", MemoryLayout::Native),
               rowMajorValues);
  EXPECT(facade.missingValue<float>("initial_obsvalue/2") == std::nullopt);
  // Try to assign values into a vector of an incorrect type.
  EXPECT_THROWS(facade.setVariableValues("initial_obsvalue/2", std::vector<int>(12),
                                         MemoryLayout::RowMajor));
}

CASE("testSetVariableValuesWithChannelAxis_ObsGroup") {
  testSetVariableValuesWithChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testSetVariableValuesWithChannelAxis_OsdfFrame") {
  testSetVariableValuesWithChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testSetVariableValuesWithMissingValuesWithoutChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  const size_t numLocations = 4;
  for (bool initializeWithChannels : {false, true})
    for (MemoryLayout memoryLayoutOnWrite : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                             MemoryLayout::Native})
      for (MemoryLayout memoryLayoutOnRead : {MemoryLayout::ColumnMajor, MemoryLayout::RowMajor,
                                              MemoryLayout::Native})
      {
        std::unique_ptr<WrappedContainer> container = makeContainer();
        ContainerFacade &facade = container->facade();
        if (initializeWithChannels)
          initializeContainerWithoutChannels(facade);
        else
          initializeContainerWithChannels(facade);

        // Float variable
        {
          const std::string name = "initial_obsvalue/3";
          const std::vector<float> nullValues(numLocations);
          const float missingValue = -10;
          const std::vector<float> writtenValues({0.f, 1.f, missingValue, 3.f});

          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<float> readValues = facade.variableValues<float>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<float>(name) == missingValue);
        }

        // Int variable
        {
          const std::string name = "rain_flag";
          const std::vector<int> nullValues(numLocations);
          const int missingValue = -10;
          const std::vector<int> writtenValues({1, 2, missingValue, 4});

          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<int> readValues = facade.variableValues<int>(name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int>(name) == missingValue);
        }

        // Int64_t variable
        {
          const std::string name = "level";
          const std::vector<int64_t> nullValues(numLocations);
          const int64_t missingValue = -10;
          const std::vector<int64_t> writtenValues({-1, -2, missingValue, -4});

          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<int64_t> readValues = facade.variableValues<int64_t>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<int64_t>(name) == missingValue);
        }

        // String variable
        {
          const std::string name = "satellite_identifier";
          const std::vector<std::string> nullValues(numLocations);
          const std::string missingValue = "NONE";
          const std::vector<std::string> writtenValues({"ABC", "DEF", missingValue, "JKL"});

          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<std::string> readValues = facade.variableValues<std::string>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<std::string>(name) == missingValue);
        }

        // Char variable
        {
          const std::string name = "lidar_class";
          const std::vector<char> nullValues(numLocations);
          const char missingValue = 'z';
          const std::vector<char> writtenValues({'a', 'b', 'c', 'd'});

          facade.addVariable(name, nullValues, false /*hasChannelAxis*/, memoryLayoutOnWrite,
                             missingValue);
          facade.setVariableValues(name, writtenValues, memoryLayoutOnWrite);
          const std::vector<char> readValues = facade.variableValues<char>(
            name, memoryLayoutOnRead);
          EXPECT_EQUAL(writtenValues, readValues);
          EXPECT(facade.missingValue<char>(name) == missingValue);
        }
      }
}

CASE("testSetVariableValuesWithMissingValuesWithoutChannelAxis_ObsGroup") {
  testSetVariableValuesWithMissingValuesWithoutChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testSetVariableValuesWithMissingValuesWithoutChannelAxis_OsdfFrame") {
  testSetVariableValuesWithMissingValuesWithoutChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testSetVariableValuesWithMissingValuesWithChannelAxis(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  std::unique_ptr<WrappedContainer> container = makeContainer();
  ContainerFacade &facade = container->facade();
  initializeContainerWithChannels(facade);
  const float missingValue = -10.f;
  // Rows = locations, columns = channels
  const std::vector<float> nullValues(12);
  const std::vector<float> rowMajorValues({300.f, 301.f, missingValue,
                                           303.f, 304.f, 305.f,
                                           306.f, missingValue, 308.f,
                                           309.f, 310.f, 311.f});
  const std::vector<float> colMajorValues({300.f, 303.f, 306.f, 309.f,
                                           301.f, 304.f, missingValue, 310.f,
                                           missingValue, 305.f, 308.f, 311.f});
  facade.addVariable("initial_obsvalue/2", nullValues, true /*hasChannelAxis*/,
                     MemoryLayout::RowMajor, missingValue);
  facade.addVariable("initial_obsvalue/3", nullValues, true /*hasChannelAxis*/,
                     MemoryLayout::ColumnMajor, missingValue);
  facade.addVariable("initial_obsvalue/4", nullValues, true /*hasChannelAxis*/,
                     MemoryLayout::Native, missingValue);
  facade.setVariableValues("initial_obsvalue/2", rowMajorValues, MemoryLayout::RowMajor);
  facade.setVariableValues("initial_obsvalue/3", colMajorValues, MemoryLayout::ColumnMajor);
  facade.setVariableValues("initial_obsvalue/4", rowMajorValues, MemoryLayout::Native);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/2", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::RowMajor),
               rowMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/3", MemoryLayout::ColumnMajor),
               colMajorValues);
  EXPECT_EQUAL(facade.variableValues<float>("initial_obsvalue/4", MemoryLayout::Native),
               rowMajorValues);
  EXPECT(facade.missingValue<float>("initial_obsvalue/2") == missingValue);
  EXPECT(facade.missingValue<float>("initial_obsvalue/3") == missingValue);
  EXPECT(facade.missingValue<float>("initial_obsvalue/4") == missingValue);
}

CASE("testSetVariableValuesWithMissingValuesWithChannelAxis_ObsGroup") {
  testSetVariableValuesWithMissingValuesWithChannelAxis(std::make_unique<WrappedObsGroup>);
}

CASE("testSetVariableValuesWithMissingValuesWithChannelAxis_OsdfFrame") {
  testSetVariableValuesWithMissingValuesWithChannelAxis(std::make_unique<WrappedOsdfFrame>);
}

void testIodaVariableNames(
  const std::function<std::unique_ptr<WrappedContainer>()> &makeContainer) {
  std::unique_ptr<WrappedContainer> container = makeContainer();
  ContainerFacade &facade = container->facade();
  initializeContainerWithChannels(facade);
  facade.addVariable("lon", std::vector<float>({1., 2., 3., 4.}), false /*hasChannelAxis*/);
  facade.addVariable("initial_obsvalue/2",
                     std::vector<float>({300.f, 301.f, 302.f,
                                         303.f, 304.f, 305.f,
                                         306.f, 307.f, 308.f,
                                         309.f, 310.f, 311.f}), true /*hasChannelAxis*/,
                     MemoryLayout::RowMajor);
  facade.addVariable("profile_number", std::vector<int>({10, 20, 30, 40}),
                     false /*hasChannelAxis*/);
  facade.addVariable("qc_flags", std::vector<char>({'a', 'b', 'c', 'd'}), false /*hasChannelAxis*/);
  facade.removeVariable("qc_flags");

  std::vector<std::string> iodaVariableNames = facade.iodaVariableNames();
  std::sort(iodaVariableNames.begin(), iodaVariableNames.end());

  const std::vector<std::string> expectedIodaVariableNames({"MetaData/longitude",
                                                            "MetaData/profile_number",
                                                            "ObsValue/airTemperature"});
  EXPECT_EQUAL(iodaVariableNames, expectedIodaVariableNames);
}

CASE("testIodaVariableNames_ObsGroup") {
  testIodaVariableNames(std::make_unique<WrappedObsGroup>);
}

CASE("testIodaVariableNames_OsdfFrame") {
  testIodaVariableNames(std::make_unique<WrappedOsdfFrame>);
}

// -----------------------------------------------------------------------------

class ContainerFacades : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::ContainerFacades";}

  void register_tests() const override {}

  void clear() const override {}
};

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
  oops::Run run(argc, argv);
  ContainerFacades tests;
  return run.execute(tests);
}
