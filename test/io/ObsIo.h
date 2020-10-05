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

        ioda::ObsSpaceParameters obsParams(bgn, end, oops::mpi::world(), oops::mpi::myself());
        obsParams.deserialize(obsConfig);

        // Try the input constructor first - should have one to try if we got here
        std::shared_ptr<ObsIo> obsIo;
        if (obsParams.in_type() == ObsIoTypes::OBS_FILE) {
            obsIo = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, obsParams);
        } else if ((obsParams.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
                   (obsParams.in_type() == ObsIoTypes::GENERATOR_LIST)) {
            obsIo = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR,
                                         ObsIoModes::READ_ONLY, obsParams);
        }

        // Test the counts that should be set on construction
        ioda::Dimensions_t expectedMaxVarSize = obsConfig.getInt("test data.max var size", 0);
        ioda::Dimensions_t maxVarSize = obsIo->maxVarSize();
        EXPECT_EQUAL(maxVarSize, expectedMaxVarSize);

        ioda::Dimensions_t expectedNumLocs = obsConfig.getInt("test data.nlocs", 0);
        ioda::Dimensions_t numLocs = obsIo->numLocs();
        EXPECT_EQUAL(numLocs, expectedNumLocs);

        ioda::Dimensions_t expectedNumVars = obsConfig.getInt("test data.nvars", 0);
        ioda::Dimensions_t numVars = obsIo->numVars();
        EXPECT_EQUAL(numVars, expectedNumVars);

        ioda::Dimensions_t expectedNumDimVars = obsConfig.getInt("test data.ndvars", 0);
        ioda::Dimensions_t numDimVars = obsIo->numDimVars();
        EXPECT_EQUAL(numDimVars, expectedNumDimVars);

        // Try the output constructor, if one was specified
        if (obsParams.out_type() == ObsIoTypes::OBS_FILE) {
            setOfileParamsFromTestConfig(obsConfig, obsParams);
            obsIo = ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obsParams);

            // See if we get expected number of locations
            std::vector<eckit::LocalConfiguration> writeDimConfigs =
                obsConfig.getSubConfigurations("test data.write dimensions");
            ioda::Dimensions_t expectedNumLocs = 0;
            for (std::size_t j = 0; j < writeDimConfigs.size(); ++j) {
                std::string dimName = writeDimConfigs[i].getString("name");
                Dimensions_t dimSize = writeDimConfigs[i].getInt("size");
                if (dimName == "nlocs") {
                    expectedNumLocs = dimSize;
                }
            }

            ioda::Dimensions_t numLocs = obsIo->numLocs();
            EXPECT_EQUAL(numLocs, expectedNumLocs);
        }
    }
}

// -----------------------------------------------------------------------------
void testRead() {
    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        confOspaces[i].get("obs space", obsConfig);
        oops::Log::trace() << "ObsIo testRead config: " << i << ": " << obsConfig << std::endl;

        ioda::ObsSpaceParameters obsParams(bgn, end, oops::mpi::world(), oops::mpi::myself());
        obsParams.deserialize(obsConfig);

        // Input constructor
        std::shared_ptr<ObsIo> obsIo;
        if (obsParams.in_type() == ObsIoTypes::OBS_FILE) {
            obsIo = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, obsParams);
        } else if ((obsParams.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
                   (obsParams.in_type() == ObsIoTypes::GENERATOR_LIST)) {
            obsIo = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR,
                                         ObsIoModes::READ_ONLY, obsParams);
        }

        // Try reading a couple variables
        float floatTol = obsConfig.getFloat("test data.tolerance", 1.0e-5);
        std::vector<eckit::LocalConfiguration> readVarConfigs =
            obsConfig.getSubConfigurations("test data.read variables");

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
void testWrite() {
    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        confOspaces[i].get("obs space", obsConfig);
        oops::Log::trace() << "ObsIo testWrite config: " << i << ": " << obsConfig << std::endl;

        ioda::ObsSpaceParameters obsParams(bgn, end, oops::mpi::world(), oops::mpi::myself());
        obsParams.deserialize(obsConfig);

        if (obsParams.out_type() == ObsIoTypes::OBS_FILE) {
            std::vector<eckit::LocalConfiguration> writeVarConfigs =
                obsConfig.getSubConfigurations("test data.write variables");
            std::vector<eckit::LocalConfiguration> writeDimConfigs =
                obsConfig.getSubConfigurations("test data.write dimensions");

            // Output constructor
            setOfileParamsFromTestConfig(obsConfig, obsParams);
            std::shared_ptr<ObsIo> obsIo =
                ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obsParams);

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

            // reset the variable lists in the ObsIo object
            obsIo->resetVarList();
            obsIo->resetDimVarList();
            obsIo->resetVarDimMap();

            // Check if all the variables got written into the file
            // Dimension scale variables
            std::vector<std::string> expectedDimList;
            for (size_t i = 0; i < writeDimConfigs.size(); ++i) {
                expectedDimList.push_back(writeDimConfigs[i].getString("name"));
            }
            std::sort(expectedDimList.begin(), expectedDimList.end());
            std::vector<std::string> dimList = obsIo->dimVarList();
            for (size_t i = 0; i < dimList.size(); ++i) {
                EXPECT_EQUAL(dimList[i], expectedDimList[i]);
            }

            // Regular variables
            std::vector<std::string> expectedVarList;
            for (size_t i = 0; i < writeVarConfigs.size(); ++i) {
                expectedVarList.push_back(writeVarConfigs[i].getString("name"));
            }
            std::sort(expectedVarList.begin(), expectedVarList.end());
            std::vector<std::string> varList = obsIo->varList();
            for (size_t i = 0; i < varList.size(); ++i) {
                EXPECT_EQUAL(varList[i], expectedVarList[i]);
            }
        }
    }
}

// -----------------------------------------------------------------------------

class ObsIo : public oops::Test {
 public:
    ObsIo() {}
    virtual ~ObsIo() {}
 private:
    std::string testid() const override {return "test::ObsIo";}

    void register_tests() const override {
        std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

        ts.emplace_back(CASE("ioda/ObsIo/testConstructor")
            { testConstructor(); });
        ts.emplace_back(CASE("ioda/ObsIo/testRead")
            { testRead(); });
        ts.emplace_back(CASE("ioda/ObsIo/testWrite")
            { testWrite(); });
    }

    void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSIO_H_
