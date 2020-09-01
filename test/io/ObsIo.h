/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSIO_H_
#define TEST_IO_OBSIO_H_

#include <functional>
#include <numeric>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/DateTime.h"
#include "oops/util/FloatCompare.h"
#include "oops/util/Logger.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/io/ObsIo.h"
#include "ioda/io/ObsIoFactory.h"
#include "ioda/io/ObsIoParameters.h"
#include "ioda/ObsGroup.h"
#include "ioda/Variables/Variable.h"

namespace ioda {
namespace test {

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

        ioda::ObsIoParameters obsParams(bgn, end, oops::mpi::comm());
        obsParams.deserialize(obsConfig);

        // Try the input constructor first - should have one to try if we got here
        std::shared_ptr<ObsIo> obsIo;
        if (obsParams.in_type() == ObsIoTypes::OBS_FILE) {
            obsIo = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, obsParams);
        } else if ((obsParams.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
                   (obsParams.in_type() == ObsIoTypes::GENERATOR_LIST)) {
            obsIo = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR, ObsIoModes::READ_ONLY, obsParams);
        }

        // See if we get expected number of locations
        ioda::Dimensions_t expectedNumLocs = obsConfig.getInt("test data.nlocs", 0);
        ioda::Dimensions nlocsDims = obsIo->obs_group_.vars.open("nlocs").getDimensions();
        ioda::Dimensions_t numLocs = nlocsDims.dimsCur[0];
        EXPECT_EQUAL(numLocs, expectedNumLocs);

        // Try the output constructor, if one was specified
        if (obsParams.out_type() == ObsIoTypes::OBS_FILE) {
            obsIo = ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obsParams);

            // Should have an empty top level group at this point
            std::vector<std::string> childGroups = obsIo->obs_group_.list();
            EXPECT_EQUAL(childGroups.size(), 0);
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

        ioda::ObsIoParameters obsParams(bgn, end, oops::mpi::comm());
        obsParams.deserialize(obsConfig);

        // Input constructor
        std::shared_ptr<ObsIo> obsIo;
        if (obsParams.in_type() == ObsIoTypes::OBS_FILE) {
            obsIo = ObsIoFactory::create(ObsIoActions::OPEN_FILE, ObsIoModes::READ_ONLY, obsParams);
        } else if ((obsParams.in_type() == ObsIoTypes::GENERATOR_RANDOM) ||
                   (obsParams.in_type() == ObsIoTypes::GENERATOR_LIST)) {
            obsIo = ObsIoFactory::create(ObsIoActions::CREATE_GENERATOR, ObsIoModes::READ_ONLY, obsParams);
        }

        // See if we get expected number of locations and variables
        ioda::Dimensions_t expectedNumLocs = obsConfig.getInt("test data.nlocs", 0);
        ioda::Dimensions nlocsDims = obsIo->obs_group_.vars.open("nlocs").getDimensions();
        ioda::Dimensions_t numLocs = nlocsDims.dimsCur[0];
        EXPECT_EQUAL(numLocs, expectedNumLocs);

        int expectedNumVars = obsConfig.getInt("test data.nvars", 0);
        std::size_t numVars = obsIo->numVars();
        EXPECT_EQUAL(numVars, expectedNumVars);

        // Get the expected data for variables
        float floatTol = obsConfig.getFloat("test data.tolerance", 1.0e-5);
        std::vector<eckit::LocalConfiguration> readVarConfigs =
            obsConfig.getSubConfigurations("test data.read variables");

        // Try the frame iterator
        int iframe = 0;
        for (obsIo->frameInit(); obsIo->frameAvailable(); obsIo->frameNext()) {
            oops::Log::debug() << "testRead: Frame number: " << iframe << std::endl
                << "    frameStart: " << obsIo->frameStart() << std::endl;
            // Try reading a couple variables
            for (std::size_t j = 0; j < readVarConfigs.size(); ++j) {
               std::string varName = readVarConfigs[j].getString("name");
               std::string expectedVarType = readVarConfigs[j].getString("type");

               int frameCount = obsIo->frameCount(varName);
               if (frameCount > 0) {
                   oops::Log::debug() << "    Variable: " << varName
                       << ", frameCount: " << frameCount << std::endl;
                   ioda::Variable var = obsIo->obs_group_.vars.open(varName);

                   // Form the hyperslab selection for this frame
                   ioda::Selection frontendSelect;
                   ioda::Selection backendSelect;
                   obsIo->createFrameSelection(varName, frontendSelect, backendSelect);

                   if (expectedVarType == "int") {
                       EXPECT(var.isA<int>());
                       std::vector<int> expectedVarValue0 =
                           readVarConfigs[j].getIntVector("value0");
                       std::vector<int> varValues(frameCount, 0);
                       var.read<int>(varValues, frontendSelect, backendSelect);
                       EXPECT_EQUAL(varValues[0], expectedVarValue0[iframe]);
                   } else if (expectedVarType == "float") {
                       EXPECT(var.isA<float>());
                       std::vector<float> expectedVarValue0 =
                           readVarConfigs[j].getFloatVector("value0");
                       std::vector<float> varValues(frameCount, 0.0);
                       var.read<float>(varValues, frontendSelect, backendSelect);
                       EXPECT(oops::is_close_relative(varValues[0],
                                                      expectedVarValue0[iframe], floatTol));
                   } else if (expectedVarType == "string") {
                       std::vector<std::string> expectedVarValue0 =
                           readVarConfigs[j].getStringVector("value0");
                       std::vector<std::string> varValues(frameCount, "");
                       if (var.isA<std::string>()) {
                           var.read<std::string>(varValues, frontendSelect, backendSelect);
                       } else {
                           ioda::Dimensions varDims = var.getDimensions();
                           std::vector<std::size_t> varShape;
                           varShape.assign(varDims.dimsCur.begin(), varDims.dimsCur.end());

                           std::vector<char> charData;
                           var.read<char>(charData);
                           varValues = CharArrayToStringVector(charData.data(), varShape);
                       }
                       EXPECT_EQUAL(varValues[0], expectedVarValue0[iframe]);
                   }
               }
            }
            iframe++;
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

        ioda::ObsIoParameters obsParams(bgn, end, oops::mpi::comm());
        obsParams.deserialize(obsConfig);

        // Output constructor
        if (obsParams.out_type() == ObsIoTypes::OBS_FILE) {
            // Get dimensions and variables sub configurations
            std::vector<eckit::LocalConfiguration> writeDimConfigs =
                obsConfig.getSubConfigurations("test data.write dimensions");
            std::vector<eckit::LocalConfiguration> writeVarConfigs =
                obsConfig.getSubConfigurations("test data.write variables");

            // Add the dimensions scales to the ObsIo parameters
            for (std::size_t i = 0; i < writeDimConfigs.size(); ++i) {
                std::string dimName = writeDimConfigs[i].getString("name");
                Dimensions_t dimSize = writeDimConfigs[i].getInt("size");
                bool isUnlimited = writeDimConfigs[i].getBool("unlimited", false);

                if (isUnlimited) {
                    obsParams.setDimScale(dimName, dimSize, Unlimited, dimSize);
                } else {
                    obsParams.setDimScale(dimName, dimSize, dimSize, dimSize);
                }
            }

            std::shared_ptr<ObsIo> obsIo =
                ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obsParams);
        }
    }
}

// -----------------------------------------------------------------------------

class ObsIo : public oops::Test {
    public:
        ObsIo() {}
        virtual ~ObsIo() {}
    private:
        std::string testid() const {return "test::ObsIo";}

        void register_tests() const {
            std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

            ts.emplace_back(CASE("ioda/ObsIo/testConstructor")
                { testConstructor(); });
            ts.emplace_back(CASE("ioda/ObsIo/testRead")
                { testRead(); });
            ts.emplace_back(CASE("ioda/ObsIo/testWrite")
                { testWrite(); });
    }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSIO_H_