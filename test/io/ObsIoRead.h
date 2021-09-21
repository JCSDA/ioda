/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSIOREAD_H_
#define TEST_IO_OBSIOREAD_H_

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
        oops::Log::trace() << "ObsIo testRead obs space config: " << i << ": "
                           << obsConfig << std::endl;
        oops::Log::trace() << "ObsIo testRead test data config: " << i << ": "
                           << testConfig << std::endl;

        ioda::ObsTopLevelParameters topParams;
        topParams.validateAndDeserialize(obsConfig);
        ioda::ObsSpaceParameters obsParams(topParams, bgn, end,
                                           oops::mpi::world(), oops::mpi::myself());

        // Input constructor
        std::shared_ptr<ObsIo> obsIo;
        obsIo = ObsIoFactory::create(ObsIoModes::READ, obsParams);

        // Try reading a couple variables
        float floatTol = testConfig.getFloat("tolerance", 1.0e-5);
        std::vector<eckit::LocalConfiguration> readVarConfigs =
            testConfig.getSubConfigurations("read variables");

        for (std::size_t ivar = 0; ivar < readVarConfigs.size(); ++ivar) {
            std::string varName = readVarConfigs[ivar].getString("name");
            std::string expectedVarType = readVarConfigs[ivar].getString("type");
            ioda::Variable var = obsIo->vars().open(varName);

            if (expectedVarType == "int") {
                EXPECT(var.isA<int>());
                std::vector<int> expectedVarValues =
                    readVarConfigs[ivar].getIntVector("values");
                std::vector<int> varValues;
                var.read<int>(varValues);
                for (std::size_t ival = 0; ival < expectedVarValues.size(); ++ival) {
                    EXPECT_EQUAL(varValues[ival], expectedVarValues[ival]);
                }
            } else if (expectedVarType == "float") {
                EXPECT(var.isA<float>());
                std::vector<float> expectedVarValues =
                    readVarConfigs[ivar].getFloatVector("values");
                std::vector<float> varValues;
                var.read<float>(varValues);
                for (std::size_t ival = 0; ival < expectedVarValues.size(); ++ival) {
                    EXPECT(oops::is_close_relative(varValues[ival],
                                                   expectedVarValues[ival], floatTol));
                }
            } else if (expectedVarType == "string") {
                EXPECT(var.isA<std::string>());
                std::vector<std::string> expectedVarValues =
                    readVarConfigs[ivar].getStringVector("values");
                std::vector<std::string> varValues;
                Dimensions varDims = var.getDimensions();
                if (varDims.dimensionality > 1) {
                    std::vector<Dimensions_t> varShape = var.getDimensions().dimsCur;
                    std::vector<std::string> stringArray = var.readAsVector<std::string>();
                    varValues = StringArrayToStringVector(stringArray, varShape);
                } else {
                    var.read<std::string>(varValues);
                }
                for (std::size_t ival = 0; ival < expectedVarValues.size(); ++ival) {
                    EXPECT_EQUAL(varValues[ival], expectedVarValues[ival]);
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------

class ObsIoRead : public oops::Test {
 public:
    ObsIoRead() {}
    virtual ~ObsIoRead() {}
 private:
    std::string testid() const override {return "test::ObsIoRead";}

    void register_tests() const override {
        std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

        ts.emplace_back(CASE("ioda/ObsIoRead/testRead")
            { testRead(); });
    }

    void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSIOREAD_H_
