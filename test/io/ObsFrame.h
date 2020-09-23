/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSIO_H_
#define TEST_IO_OBSIO_H_

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/DateTime.h"
#include "oops/util/FloatCompare.h"
#include "oops/util/Logger.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/io/ObsIo.h"
#include "ioda/io/ObsIoFactory.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

/// // -----------------------------------------------------------------------------
/// void testFrameRead(std::shared_ptr<ObsIo> & obsIo, eckit::LocalConfiguration & obsConfig,
///                    ioda::ObsSpaceParameters & obsParams) {
///     float floatTol = obsConfig.getFloat("test data.tolerance", 1.0e-5);
///     std::vector<eckit::LocalConfiguration> readVarConfigs =
///         obsConfig.getSubConfigurations("test data.read variables");
/// 
///     // Test reading from frames
///     std::unique_ptr<DistributionFactory> distFactory;
///     std::shared_ptr<Distribution> dist;
///     dist.reset(distFactory->createDistribution(obsParams.comm(), "RoundRobin"));
///     int iframe = 0;
///     for (obsIo->frameInit(); obsIo->frameAvailable(); obsIo->frameNext()) {
///         oops::Log::debug() << "testRead: Frame number: " << iframe << std::endl
///             << "    frameStart: " << obsIo->frameStart() << std::endl;
/// 
///         // generate the selection indices for variabels dimensioned by nlocs
///         obsIo->genFrameIndexRecNums(dist);
///         
///         // Try reading a couple variables
///         for (std::size_t j = 0; j < readVarConfigs.size(); ++j) {
///            std::string varName = readVarConfigs[j].getString("name");
///            std::string expectedVarType = readVarConfigs[j].getString("type");
///            ioda::Variable var = obsIo->obs_group_.vars.open(varName);
/// 
///            Dimensions_t frameCount = obsIo->frameCount(var);
///            if (frameCount > 0) {
///                oops::Log::debug() << "    Variable: " << varName
///                    << ", frameCount: " << frameCount << std::endl;
/// 
///                // Form the hyperslab selection for this frame
///                ioda::Selection frontendSelect;
///                ioda::Selection backendSelect;
///                obsIo->createFrameSelection(var, frontendSelect, backendSelect);
/// 
///                if (expectedVarType == "int") {
///                    EXPECT(var.isA<int>());
///                    std::vector<int> expectedVarValue0 =
///                        readVarConfigs[j].getIntVector("value0");
///                    std::vector<int> varValues(frameCount, 0);
///                    var.read<int>(varValues, frontendSelect, backendSelect);
///                    EXPECT_EQUAL(varValues[0], expectedVarValue0[iframe]);
///                } else if (expectedVarType == "float") {
///                    EXPECT(var.isA<float>());
///                    std::vector<float> expectedVarValue0 =
///                        readVarConfigs[j].getFloatVector("value0");
///                    std::vector<float> varValues(frameCount, 0.0);
///                    var.read<float>(varValues, frontendSelect, backendSelect);
///                    EXPECT(oops::is_close_relative(varValues[0],
///                                                   expectedVarValue0[iframe], floatTol));
///                } else if (expectedVarType == "string") {
///                    std::vector<std::string> expectedVarValue0 =
///                        readVarConfigs[j].getStringVector("value0");
///                    std::vector<std::string> varValues(frameCount, "");
///                    if (var.isA<std::string>()) {
///                        var.read<std::string>(varValues, frontendSelect, backendSelect);
///                    } else {
///                        ioda::Dimensions varDims = var.getDimensions();
///                        std::vector<std::size_t> varShape;
///                        varShape.assign(varDims.dimsCur.begin(), varDims.dimsCur.end());
/// 
///                        std::vector<char> charData;
///                        var.read<char>(charData);
///                        varValues = CharArrayToStringVector(charData.data(), varShape);
///                    }
///                    EXPECT_EQUAL(varValues[0], expectedVarValue0[iframe]);
///                }
///            }
///         }
///         iframe++;
///     }
/// }
/// 
/// // -----------------------------------------------------------------------------
/// void frameWrite(std::shared_ptr<ObsIo> & obsIo, eckit::LocalConfiguration & obsConfig) {
///     std::vector<eckit::LocalConfiguration> writeVarConfigs =
///         obsConfig.getSubConfigurations("test data.write variables");
/// 
///     int iframe = 0;
///     for (obsIo->frameInit(); obsIo->frameAvailable(); obsIo->frameNext()) {
///         Dimensions_t frameStart = obsIo->frameStart();
///         oops::Log::debug() << "testWrite: Frame number: " << iframe << std::endl
///             << "    frameStart: " << frameStart << std::endl;
/// 
///         // Write the test variables
///         for (std::size_t j = 0; j < writeVarConfigs.size(); ++j) {
///             std::string varName = writeVarConfigs[j].getString("name");
///             std::string varType = writeVarConfigs[j].getString("type");
///             std::vector<std::string> varDimNames =
///                 writeVarConfigs[j].getStringVector("dims");
/// 
///             ioda::Variable var;
///             // if on the first frame, create the variable
///             if (iframe == 0) {
///                 std::vector<ioda::Variable> varDims;
///                 for (std::size_t idim = 0; idim < varDimNames.size(); ++idim) {
///                     varDims.push_back(obsIo->obs_group_.vars.open(varDimNames[idim]));
///                 }
/// 
///                 ioda::VariableCreationParameters params;
///                 params.chunk = true;
///                 params.compressWithGZIP();
///                 if (varType == "int") {
///                     params.setFillValue<int>(-999);
///                     var = obsIo->obs_group_.vars
///                         .createWithScales<int>(varName, varDims, params);
///                 } else if (varType == "float") {
///                     params.setFillValue<float>(-999);
///                     var = obsIo->obs_group_.vars
///                         .createWithScales<float>(varName, varDims, params);
///                 } else if (varType == "string") {
///                     params.setFillValue<std::string>("fill");
///                     var = obsIo->obs_group_.vars
///                         .createWithScales<std::string>(varName, varDims, params);
///                 }
///             } else {
///                 var = obsIo->obs_group_.vars.open(varName);
///             }
/// 
///             Dimensions_t frameCount = obsIo->frameCount(var);
///             if (frameCount > 0) {
///                 oops::Log::debug() << "    Variable: " << varName
///                     << ", frameCount: " << frameCount << std::endl;
///                 // Form the hyperslab selection for this frame
///                 ioda::Selection frontendSelect;
///                 ioda::Selection backendSelect;
///                 obsIo->createFrameSelection(var, frontendSelect, backendSelect);
/// 
///                 if (varType == "int") {
///                     std::vector<int> values =
///                         writeVarConfigs[j].getIntVector("values");
///                     std::vector<int> varValues(values.begin() + frameStart,
///                         values.begin() + frameStart + frameCount);
///                     var.write<int>(varValues, frontendSelect, backendSelect);
///                 } else if (varType == "float") {
///                     std::vector<float> values =
///                         writeVarConfigs[j].getFloatVector("values");
///                     std::vector<float> varValues(values.begin() + frameStart,
///                         values.begin() + frameStart + frameCount);
///                     var.write<float>(varValues, frontendSelect, backendSelect);
///                 } else if (varType == "string") {
///                     std::vector<std::string> values =
///                         writeVarConfigs[j].getStringVector("values");
///                     std::vector<std::string> varValues(values.begin() + frameStart,
///                         values.begin() + frameStart + frameCount);
///                     var.write<std::string>(varValues, frontendSelect, backendSelect);
///                 }
///             }
///         }
/// 
///         iframe++;
///     }
/// }

// -----------------------------------------------------------------------------
// Test Functions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
void testConstructor() {
    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        confOspaces[i].get("obs space", obsConfig);
        oops::Log::trace() << "ObsIo test config: " << i << ": " << obsConfig << std::endl;

///         ioda::ObsSpaceParameters obsParams(bgn, end, oops::mpi::world());
///         obsParams.deserialize(obsConfig);
/// 
///         // Try the input constructor first - should have one to try if we got here
///         std::shared_ptr<ObsIo> obsIo;
///         if (obsParams.in_type() == ObsIoTypes::OBS_FILE) {
///             obsIo = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, obsParams);
///         } else if ((obsParams.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
///                    (obsParams.in_type() == ObsIoTypes::GENERATOR_LIST)) {
///             obsIo = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR,
///                                          ObsIoModes::READ_ONLY, obsParams);
///         }
/// 
///         // See if we get expected number of locations
///         ioda::Dimensions_t expectedNumLocs = obsConfig.getInt("test data.nlocs", 0);
///         ioda::Dimensions nlocsDims = obsIo->obs_group_.vars.open("nlocs").getDimensions();
///         ioda::Dimensions_t numLocs = nlocsDims.dimsCur[0];
///         EXPECT_EQUAL(numLocs, expectedNumLocs);
/// 
///         // Try the output constructor, if one was specified
///         if (obsParams.out_type() == ObsIoTypes::OBS_FILE) {
///             obsIo = ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obsParams);
/// 
///             // Should have an empty top level group at this point
///             std::vector<std::string> childGroups = obsIo->obs_group_.list();
///             EXPECT_EQUAL(childGroups.size(), 0);
///         }
    }
}

/// // -----------------------------------------------------------------------------
/// void testRead() {
///     const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
///     std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
///     util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
///     util::DateTime end(::test::TestEnvironment::config().getString("window end"));
/// 
///     for (std::size_t i = 0; i < confOspaces.size(); ++i) {
///         eckit::LocalConfiguration obsConfig;
///         confOspaces[i].get("obs space", obsConfig);
///         oops::Log::trace() << "ObsIo testRead config: " << i << ": " << obsConfig << std::endl;
/// 
///         ioda::ObsSpaceParameters obsParams(bgn, end, oops::mpi::world());
///         obsParams.deserialize(obsConfig);
/// 
///         // Input constructor
///         std::shared_ptr<ObsIo> obsIo;
///         if (obsParams.in_type() == ObsIoTypes::OBS_FILE) {
///             obsIo = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, obsParams);
///         } else if ((obsParams.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
///                    (obsParams.in_type() == ObsIoTypes::GENERATOR_LIST)) {
///             obsIo = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR,
///                                          ObsIoModes::READ_ONLY, obsParams);
///         }
/// 
///         // See if we get expected number of locations and variables
///         ioda::Dimensions_t expectedNumLocs = obsConfig.getInt("test data.nlocs", 0);
///         ioda::Dimensions nlocsDims = obsIo->obs_group_.vars.open("nlocs").getDimensions();
///         ioda::Dimensions_t numLocs = nlocsDims.dimsCur[0];
///         EXPECT_EQUAL(numLocs, expectedNumLocs);
/// 
///         int expectedNumVars = obsConfig.getInt("test data.nvars", 0);
///         std::vector<std::string> varList = listVars(obsIo->obs_group_);
///         int numVars = varList.size();
///         EXPECT_EQUAL(numVars, expectedNumVars);
/// 
///         // Test reading frames
///         testFrameRead(obsIo, obsConfig, obsParams);
///     }
/// }
/// 
/// // -----------------------------------------------------------------------------
/// void testWrite() {
///     const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
///     std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
///     util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
///     util::DateTime end(::test::TestEnvironment::config().getString("window end"));
/// 
///     for (std::size_t i = 0; i < confOspaces.size(); ++i) {
///         eckit::LocalConfiguration obsConfig;
///         confOspaces[i].get("obs space", obsConfig);
///         oops::Log::trace() << "ObsIo testWrite config: " << i << ": " << obsConfig << std::endl;
/// 
///         ioda::ObsSpaceParameters obsParams(bgn, end, oops::mpi::world());
///         obsParams.deserialize(obsConfig);
/// 
///         if (obsParams.out_type() == ObsIoTypes::OBS_FILE) {
///             // Get dimensions and variables sub configurations
///             std::vector<eckit::LocalConfiguration> writeDimConfigs =
///                 obsConfig.getSubConfigurations("test data.write dimensions");
///             std::vector<eckit::LocalConfiguration> writeVarConfigs =
///                 obsConfig.getSubConfigurations("test data.write variables");
/// 
///             // Add the dimensions scales to the ObsIo parameters
///             std::map<std::string, Dimensions_t> dimSizes;
///             for (std::size_t i = 0; i < writeDimConfigs.size(); ++i) {
///                 std::string dimName = writeDimConfigs[i].getString("name");
///                 Dimensions_t dimSize = writeDimConfigs[i].getInt("size");
///                 bool isUnlimited = writeDimConfigs[i].getBool("unlimited", false);
/// 
///                 if (isUnlimited) {
///                     obsParams.setDimScale(dimName, dimSize, Unlimited, dimSize);
///                 } else {
///                     obsParams.setDimScale(dimName, dimSize, dimSize, dimSize);
///                 }
///                 dimSizes.insert(std::pair<std::string, Dimensions_t>(dimName, dimSize));
///             }
/// 
///             // Add the maximum variable size to the ObsIo parmeters
///             Dimensions_t maxVarSize = 0;
///             for (std::size_t i = 0; i < writeVarConfigs.size(); ++i) {
///                 std::vector<std::string> dimNames = writeVarConfigs[i].getStringVector("dims");
///                 Dimensions_t varSize0 = dimSizes.at(dimNames[0]);
///                 if (varSize0 > maxVarSize) {
///                     maxVarSize = varSize0;
///                 }
///             }
///             obsParams.setMaxVarSize(maxVarSize);
/// 
///             // Output constructor
///             std::shared_ptr<ObsIo> obsIo =
///                 ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obsParams);
/// 
///             // Write contents of file
///             frameWrite(obsIo, obsConfig);
/// 
///             // Check if all the variables got written into the file
///             // Dimension scale variables
///             std::vector<std::string> expectedDimList;
///             for (size_t i = 0; i < writeDimConfigs.size(); ++i) {
///                 expectedDimList.push_back(writeDimConfigs[i].getString("name"));
///             }
///             std::sort(expectedDimList.begin(), expectedDimList.end());
///             std::vector<std::string> dimList = listDimVars(obsIo->obs_group_);
///             for (size_t i = 0; i < dimList.size(); ++i) {
///                 EXPECT_EQUAL(dimList[i], expectedDimList[i]);
///             }
/// 
///             // Regular variables
///             std::vector<std::string> expectedVarList;
///             for (size_t i = 0; i < writeVarConfigs.size(); ++i) {
///                 expectedVarList.push_back(writeVarConfigs[i].getString("name"));
///             }
///             std::sort(expectedVarList.begin(), expectedVarList.end());
///             std::vector<std::string> varList = listVars(obsIo->obs_group_);
///             for (size_t i = 0; i < varList.size(); ++i) {
///                 EXPECT_EQUAL(varList[i], expectedVarList[i]);
///             }
///         }
///     }
/// }

// -----------------------------------------------------------------------------

class ObsFrame : public oops::Test {
 public:
    ObsFrame() {}
    virtual ~ObsFrame() {}
 private:
    std::string testid() const override {return "test::ObsIo";}

    void register_tests() const override {
        std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

        ts.emplace_back(CASE("ioda/ObsIo/testConstructor")
            { testConstructor(); });
//        ts.emplace_back(CASE("ioda/ObsIo/testRead")
//            { testRead(); });
//        ts.emplace_back(CASE("ioda/ObsIo/testWrite")
//            { testWrite(); });
    }

    void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSIO_H_
