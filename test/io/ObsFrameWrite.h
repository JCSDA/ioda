/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSFRAMEWRITE_H_
#define TEST_IO_OBSFRAMEWRITE_H_

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
#include "ioda/Engines/HH.h"
#include "ioda/io/ObsFrameWrite.h"
#include "ioda/Layout.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
ObsGroup buildTestObsGroup(const std::vector<eckit::LocalConfiguration> & dimConfigs,
                           const std::vector<eckit::LocalConfiguration> & varConfigs) {
    // create an ObsGroup with an in-memory backend
    Engines::BackendNames backendName;
    Engines::BackendCreationParameters backendParams;
    backendParams.action = Engines::BackendFileActions::Create;
    backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    backendParams.fileName = ioda::Engines::HH::genUniqueName();
    backendParams.allocBytes = 1024*1024*50;
    backendParams.flush = false;

    backendName = Engines::BackendNames::ObsStore;  // Hdf5Mem;  ObsStore;
    Group backend = constructBackend(backendName, backendParams);

    // Add the dimension scales
    NewDimensionScales_t newDims;
    for (std::size_t i = 0; i < dimConfigs.size(); ++i) {
        std::string dimName = dimConfigs[i].getString("name");
        Dimensions_t dimSize = dimConfigs[i].getInt("size");
        bool isUnlimited = dimConfigs[i].getBool("unlimited", false);

        if (isUnlimited) {
            newDims.push_back(
                ioda::NewDimensionScale<int>(
                    dimName, dimSize, ioda::Unlimited, dimSize));
        } else {
            newDims.push_back(
                ioda::NewDimensionScale<int>(
                    dimName, dimSize, dimSize, dimSize));
        }
    }
    ObsGroup obsGroup = ObsGroup::generate(backend, newDims);

    // Create variables
    for (std::size_t j = 0; j < varConfigs.size(); ++j) {
        std::string varName = varConfigs[j].getString("name");
        std::string varType = varConfigs[j].getString("type");
        std::vector<std::string> varDimNames =
            varConfigs[j].getStringVector("dims");

        std::vector<ioda::Variable> varDims;
        for (std::size_t idim = 0; idim < varDimNames.size(); ++idim) {
            varDims.push_back(obsGroup.vars.open(varDimNames[idim]));
        }

        ioda::VariableCreationParameters params;
        params.chunk = true;
        params.compressWithGZIP();
        if (varType == "int") {
            params.setFillValue<int>(-999);
            obsGroup.vars.createWithScales<int>(varName, varDims, params)
                .write<int>(varConfigs[j].getIntVector("values"));
        } else if (varType == "float") {
            params.setFillValue<float>(-999);
            obsGroup.vars.createWithScales<float>(varName, varDims, params)
                .write<float>(varConfigs[j].getFloatVector("values"));
        } else if (varType == "string") {
            params.setFillValue<std::string>("fill");
            obsGroup.vars.createWithScales<std::string>(varName, varDims, params)
                .write<std::string>(varConfigs[j].getStringVector("values"));
        }
    }
    return obsGroup;
}

// -----------------------------------------------------------------------------
void frameWrite(ObsFrameWrite & obsFrame, eckit::LocalConfiguration & testConfig,
                const Has_Variables & sourceVars, const VarNameObjectList & varList,
                const VarNameObjectList & dimVarList,
                const VarDimMap & varDimMap, const Dimensions_t maxVarSize) {
    std::vector<eckit::LocalConfiguration> writeVarConfigs =
        testConfig.getSubConfigurations("write variables");

    int iframe = 0;
    for (obsFrame.frameInit(varList, dimVarList, varDimMap, maxVarSize);
         obsFrame.frameAvailable(); obsFrame.frameNext(varList)) {
        Dimensions_t frameStart = obsFrame.frameStart();
        oops::Log::debug() << "testWrite: Frame number: " << iframe << std::endl
            << "    frameStart: " << frameStart << std::endl;

        // Write the test variables
        for (std::size_t j = 0; j < writeVarConfigs.size(); ++j) {
            std::string varName = writeVarConfigs[j].getString("name");
            std::string varType = writeVarConfigs[j].getString("type");
            std::vector<std::string> varDimNames =
                writeVarConfigs[j].getStringVector("dims");

            ioda::Variable var = obsFrame.vars().open(varName);

            Dimensions_t frameCount = obsFrame.frameCount(varName);
            if (frameCount > 0) {
                oops::Log::debug() << "    Variable: " << varName
                    << ", frameCount: " << frameCount << std::endl;

                if (varType == "int") {
                    std::vector<int> values =
                        writeVarConfigs[j].getIntVector("values");
                    std::vector<int> varValues(values.begin() + frameStart,
                        values.begin() + frameStart + frameCount);
                    obsFrame.writeFrameVar(varName, varValues);
                } else if (varType == "float") {
                    std::vector<float> values =
                        writeVarConfigs[j].getFloatVector("values");
                    std::vector<float> varValues(values.begin() + frameStart,
                        values.begin() + frameStart + frameCount);
                    obsFrame.writeFrameVar(varName, varValues);
                } else if (varType == "string") {
                    std::vector<std::string> values =
                        writeVarConfigs[j].getStringVector("values");
                    std::vector<std::string> varValues(values.begin() + frameStart,
                        values.begin() + frameStart + frameCount);
                    obsFrame.writeFrameVar(varName, varValues);
                }
            }
        }
        iframe++;
    }
}

// -----------------------------------------------------------------------------
// Test Functions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
void testWrite() {
    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        eckit::LocalConfiguration testConfig;
        confOspaces[i].get("obs space", obsConfig);
        confOspaces[i].get("test data", testConfig);
        oops::Log::trace() << "ObsFrame testWrite obs space config: " << i << ": "
                           << obsConfig << std::endl;
        oops::Log::trace() << "ObsFrame testWrite test data config: " << i << ": "
                           << testConfig << std::endl;

        ioda::ObsSpaceParameters obsParams(bgn, end, oops::mpi::world(), oops::mpi::myself());
        obsParams.deserialize(obsConfig);

        if (obsParams.top_level_.obsOutFile.value() != boost::none) {
            // Get dimensions and variables sub configurations
            std::vector<eckit::LocalConfiguration> writeDimConfigs =
                testConfig.getSubConfigurations("write dimensions");
            std::vector<eckit::LocalConfiguration> writeVarConfigs =
                testConfig.getSubConfigurations("write variables");

            // Create an in-memory ObsGroup containing the test dimensions and variables
            ObsGroup testObsGroup = buildTestObsGroup(writeDimConfigs, writeVarConfigs);

            // Form lists of regular and dimension scale variables
            VarNameObjectList varList;
            VarNameObjectList dimVarList;
            VarDimMap dimsAttachedToVars;
            Dimensions_t maxVarSize;
            collectVarDimInfo(testObsGroup, varList, dimVarList,
                              dimsAttachedToVars, maxVarSize);

            // Record dimension scale variables for the output file creation.
            for (auto & dimNameObject : dimVarList) {
                std::string dimName = dimNameObject.first;
                Dimensions_t dimSize = dimNameObject.second.getDimensions().dimsCur[0];
                Dimensions_t dimMaxSize = dimSize;
                if (dimName == "nlocs") {
                    dimMaxSize = Unlimited;
                }
                obsParams.setDimScale(dimName, dimSize, dimMaxSize, dimSize);
            }

            // Record the maximum variable size
            obsParams.setMaxVarSize(maxVarSize);

            // Output constructor
            ObsFrameWrite obsFrame(obsParams);

            // Write contents of file
            frameWrite(obsFrame, testConfig, testObsGroup.vars, varList, dimVarList,
                       dimsAttachedToVars, maxVarSize);
            obsFrame.ioUpdateVarDimInfo();

            // Check if all the variables got written into the file
            // Dimension scale variables
            std::vector<std::string> expectedDimList;
            for (size_t i = 0; i < writeDimConfigs.size(); ++i) {
                expectedDimList.push_back(writeDimConfigs[i].getString("name"));
            }
            std::sort(expectedDimList.begin(), expectedDimList.end());
            std::vector<std::string> dimList;
            for (auto & dimVarNameObject : obsFrame.ioDimVarList()) {
                dimList.push_back(dimVarNameObject.first);
            }
            std::sort(dimList.begin(), dimList.end());
            for (size_t i = 0; i < expectedDimList.size(); ++i) {
                EXPECT_EQUAL(dimList[i], expectedDimList[i]);
            }

            // Regular variables
            std::vector<std::string> expectedVariableList;
            for (size_t i = 0; i < writeVarConfigs.size(); ++i) {
                expectedVariableList.push_back(writeVarConfigs[i].getString("name"));
            }
            std::sort(expectedVariableList.begin(), expectedVariableList.end());
            std::vector<std::string> variableList;
            for (auto & varNameObject : obsFrame.ioVarList()) {
                variableList.push_back(varNameObject.first);
            }
            std::sort(variableList.begin(), variableList.end());
            EXPECT_EQUAL(variableList, expectedVariableList);
        }
    }
}

// -----------------------------------------------------------------------------

class ObsFrameWrite : public oops::Test {
 public:
    ObsFrameWrite() {}
    virtual ~ObsFrameWrite() {}
 private:
    std::string testid() const override {return "test::ObsFrameWrite";}

    void register_tests() const override {
        std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

        ts.emplace_back(CASE("ioda/ObsFrameWrite/testWrite")
            { testWrite(); });
    }

    void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSFRAMEWRITE_H_
