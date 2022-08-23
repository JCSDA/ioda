/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSFRAMEREAD_H_
#define TEST_IO_OBSFRAMEREAD_H_

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
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ObsStore.h"
#include "ioda/io/ObsFrameRead.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
void testFrameRead(ObsFrameRead & obsFrame, eckit::LocalConfiguration & obsConfig,
                   ioda::ObsSpaceParameters & obsParams, ioda::Has_Attributes & destAttrs) {
    float floatTol = obsConfig.getFloat("tolerance", 1.0e-5);
    std::vector<eckit::LocalConfiguration> readVarConfigs =
        obsConfig.getSubConfigurations("read variables");

    // Test reading from frames
    int iframe = 0;
    for (obsFrame.frameInit(destAttrs); obsFrame.frameAvailable(); obsFrame.frameNext()) {
        Dimensions_t frameStart = obsFrame.frameStart();
        oops::Log::debug() << "testRead: Frame number: " << iframe << std::endl
            << "    frameStart: " << frameStart << std::endl;

        // Try reading a couple variables
        for (std::size_t j = 0; j < readVarConfigs.size(); ++j) {
            std::string varName = readVarConfigs[j].getString("name");
            std::string expectedVarType = readVarConfigs[j].getString("type");
            ioda::Variable var = obsFrame.ioVars().open(varName);

            oops::Log::debug() << "    Variable: " << varName
                << ", frameCount: " << obsFrame.frameCount(varName) << std::endl;

            if (expectedVarType == "int") {
                EXPECT(var.isA<int>());
                std::vector<int> expectedVarValue0 =
                    readVarConfigs[j].getIntVector("value0");
                std::vector<int> varValues;
                if (obsFrame.readFrameVar(varName, varValues)) {
                    EXPECT_EQUAL(varValues[0], expectedVarValue0[iframe]);
                }
            } else if (expectedVarType == "int64") {
                EXPECT(var.isA<int64_t>());
                std::vector<int64_t> expectedVarValue0 =
                    readVarConfigs[j].getInt64Vector("value0");
                std::vector<int64_t> varValues;
                if (obsFrame.readFrameVar(varName, varValues)) {
                    EXPECT_EQUAL(varValues[0], expectedVarValue0[iframe]);
                }
            } else if (expectedVarType == "float") {
                EXPECT(var.isA<float>());
                std::vector<float> expectedVarValue0 =
                    readVarConfigs[j].getFloatVector("value0");
                std::vector<float> varValues;
                if (obsFrame.readFrameVar(varName, varValues)) {
                    EXPECT(oops::is_close_relative(varValues[0],
                           expectedVarValue0[iframe], floatTol));
                }
            } else if (expectedVarType == "string") {
                EXPECT(var.isA<std::string>());
                std::vector<std::string> expectedVarValue0 =
                    readVarConfigs[j].getStringVector("value0");
                std::vector<std::string> varValues;
                if (obsFrame.readFrameVar(varName, varValues)) {
                    EXPECT_EQUAL(varValues[0], expectedVarValue0[iframe]);
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
void testRead() {
    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        eckit::LocalConfiguration testConfig;
        confOspaces[i].get("obs space", obsConfig);
        confOspaces[i].get("test data", testConfig);
        oops::Log::trace() << "ObsFrame testRead obs space config: " << i << ": "
                           << obsConfig << std::endl;
        oops::Log::trace() << "ObsFrame testRead test data config: " << i
                           << ": " << testConfig << std::endl;

        ioda::ObsTopLevelParameters topParams;
        topParams.validateAndDeserialize(obsConfig);
        ioda::ObsSpaceParameters obsParams(topParams, bgn, end,
                                           oops::mpi::world(), oops::mpi::myself());

        // Input constructor
        ObsFrameRead obsFrame(obsParams);

        // Check the counts
        ioda::Dimensions_t expectedNumLocs = testConfig.getInt("nlocs", 0);
        ioda::Dimensions_t numLocs = obsFrame.ioNumLocs();
        EXPECT_EQUAL(numLocs, expectedNumLocs);

        ioda::Dimensions_t expectedNumVars = testConfig.getInt("nvars", 0);
        ioda::Dimensions_t numVars = obsFrame.ioNumVars();
        EXPECT_EQUAL(numVars, expectedNumVars);

        ioda::Dimensions_t expectedNumDimVars = testConfig.getInt("ndvars", 0);
        ioda::Dimensions_t numDimVars = obsFrame.ioNumDimVars();
        EXPECT_EQUAL(numDimVars, expectedNumDimVars);

        ioda::Dimensions_t expectedMaxVarSize = testConfig.getInt("max var size", 0);
        ioda::Dimensions_t maxVarSize = obsFrame.ioMaxVarSize();
        EXPECT_EQUAL(maxVarSize, expectedMaxVarSize);

        // Test reading frames. Create a container for capturing the global attributes.
        Engines::BackendNames backendName;
        Engines::BackendCreationParameters backendParams;
        backendName = ioda::Engines::BackendNames::ObsStore;
        Group backend = constructBackend(backendName, backendParams);
        ObsGroup testObsGroup = ObsGroup::generate(backend, { });
        testFrameRead(obsFrame, testConfig, obsParams, testObsGroup.atts);
    }
}

// -----------------------------------------------------------------------------

class ObsFrameRead : public oops::Test {
 public:
    ObsFrameRead() {}
    virtual ~ObsFrameRead() {}
 private:
    std::string testid() const override {return "test::ObsFrameRead";}

    void register_tests() const override {
        std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

        ts.emplace_back(CASE("ioda/ObsFrameRead/testRead")
            { testRead(); });
    }

    void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSFRAMEREAD_H_
