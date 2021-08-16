/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSIOWRITE_H_
#define TEST_IO_OBSIOWRITE_H_

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
        oops::Log::trace() << "ObsIo testWrite obs space config: " << i << ": "
                           << obsConfig << std::endl;
        oops::Log::trace() << "ObsIo testWrite test data config: " << i << ": "
                           << obsConfig << std::endl;

        ioda::ObsTopLevelParameters topParams;
        topParams.validateAndDeserialize(obsConfig);
        ioda::ObsSpaceParameters obsParams(topParams, bgn, end,
                                           oops::mpi::world(), oops::mpi::myself());

        if (obsParams.top_level_.obsOutFile.value() != boost::none) {
            std::vector<eckit::LocalConfiguration> writeVarConfigs =
                testConfig.getSubConfigurations("write variables");
            std::vector<eckit::LocalConfiguration> writeDimConfigs =
                testConfig.getSubConfigurations("write dimensions");

            // Output constructor
            setOfileParamsFromTestConfig(testConfig, obsParams);
            std::shared_ptr<ObsIo> obsIo =
                ObsIoFactory::create(ObsIoModes::WRITE, obsParams);

            // Write the test variables
            for (std::size_t j = 0; j < writeVarConfigs.size(); ++j) {
                std::string varName = writeVarConfigs[j].getString("name");
                std::string varType = writeVarConfigs[j].getString("type");
                std::vector<std::string> varDimNames =
                    writeVarConfigs[j].getStringVector("dims");

                // Create the variable
                ioda::Variable var;
                std::vector<ioda::Variable> varDims;
                for (std::size_t idim = 0; idim < varDimNames.size(); ++idim) {
                    varDims.push_back(obsIo->vars().open(varDimNames[idim]));
                }

                ioda::VariableCreationParameters params;
                params.chunk = true;
                params.compressWithGZIP();
                if (varType == "int") {
                    params.setFillValue<int>(-999);
                    var = obsIo->vars()
                        .createWithScales<int>(varName, varDims, params);
                } else if (varType == "float") {
                    params.setFillValue<float>(-999);
                    var = obsIo->vars()
                        .createWithScales<float>(varName, varDims, params);
                } else if (varType == "string") {
                    params.setFillValue<std::string>("fill");
                    var = obsIo->vars()
                        .createWithScales<std::string>(varName, varDims, params);
                }

                // Write the variable data
                if (varType == "int") {
                    std::vector<int> varValues =
                        writeVarConfigs[j].getIntVector("values");
                    var.write<int>(varValues);
                } else if (varType == "float") {
                    std::vector<float> varValues =
                        writeVarConfigs[j].getFloatVector("values");
                    var.write<float>(varValues);
                } else if (varType == "string") {
                    std::vector<std::string> varValues =
                        writeVarConfigs[j].getStringVector("values");
                    var.write<std::string>(varValues);
                }
            }

            // Update the variable lists in the ObsIo object
            obsIo->updateVarDimInfo();

            // Check if all the variables got written into the file
            // Dimension scale variables
            std::vector<std::string> expectedDimList;
            for (size_t i = 0; i < writeDimConfigs.size(); ++i) {
                expectedDimList.push_back(writeDimConfigs[i].getString("name"));
            }
            std::sort(expectedDimList.begin(), expectedDimList.end());
            VarNameObjectList dimList = obsIo->dimVarList();
            std::sort(dimList.begin(), dimList.end(), [](auto & p1, auto & p2) {
                return (p1.first < p2.first); });
            for (size_t i = 0; i < dimList.size(); ++i) {
                EXPECT_EQUAL(dimList[i].first, expectedDimList[i]);
            }

            // Regular variables
            std::vector<std::string> expectedVarList;
            for (size_t i = 0; i < writeVarConfigs.size(); ++i) {
                expectedVarList.push_back(writeVarConfigs[i].getString("name"));
            }
            std::sort(expectedVarList.begin(), expectedVarList.end());
            VarNameObjectList varList = obsIo->varList();
            std::sort(varList.begin(), varList.end(), [](auto & p1, auto & p2) {
                return (p1.first < p2.first); });
            for (size_t i = 0; i < dimList.size(); ++i) {
                EXPECT_EQUAL(varList[i].first, expectedVarList[i]);
            }

            // Check if the values of the variables got written correctly
            for (std::size_t j = 0; j < writeVarConfigs.size(); ++j) {
                std::string varName = writeVarConfigs[j].getString("name");
                std::string varType = writeVarConfigs[j].getString("type");

                // Read the variable data from the file and compare with
                // the values from the YAML configuration
                Variable var = obsIo->vars().open(varName);
                if (varType == "int") {
                    std::vector<int> expectedVarValues =
                        writeVarConfigs[j].getIntVector("values");
                    std::vector<int> varValues;
                    var.read<int>(varValues);
                    EXPECT_EQUAL(varValues, expectedVarValues);
                } else if (varType == "float") {
                    std::vector<float> expectedVarValues =
                        writeVarConfigs[j].getFloatVector("values");
                    std::vector<float> varValues;
                    var.read<float>(varValues);
                    EXPECT_EQUAL(varValues, expectedVarValues);
                } else if (varType == "string") {
                    std::vector<std::string> expectedVarValues =
                        writeVarConfigs[j].getStringVector("values");
                    std::vector<std::string> varValues;
                    var.read<std::string>(varValues);
                    EXPECT_EQUAL(varValues, expectedVarValues);
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------

class ObsIoWrite : public oops::Test {
 public:
    ObsIoWrite() {}
    virtual ~ObsIoWrite() {}
 private:
    std::string testid() const override {return "test::ObsIoWrite";}

    void register_tests() const override {
        std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

        ts.emplace_back(CASE("ioda/ObsIoWrite/testWrite")
            { testWrite(); });
    }

    void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSIOWRITE_H_
