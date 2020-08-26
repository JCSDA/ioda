/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSIO_H_
#define TEST_IO_OBSIO_H_

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "ioda/io/ObsIo.h"
#include "ioda/io/ObsIoFactory.h"
#include "ioda/io/ObsIoParameters.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");

    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        confOspaces[i].get("obs space", obsConfig);
        oops::Log::trace() << "ObsIo test config: " << i << ": " << obsConfig << std::endl;

        util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
        util::DateTime end(::test::TestEnvironment::config().getString("window end"));

        std::vector<std::string> simVarNames =
            obsConfig.getStringVector("simulated variables", { });

        ioda::ObsIoParameters obsParams(bgn, end, oops::mpi::comm(), simVarNames);
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
        EXPECT(numLocs == expectedNumLocs);

        // Try the output constructor, if one was specified
        if (obsParams.out_type() == ObsIoTypes::OBS_FILE) {
            obsIo = ObsIoFactory::create(ObsIoActions::CREATE_FILE, ObsIoModes::CLOBBER, obsParams);

            // Should have an empty top level group at this point
            std::vector<std::string> childGroups = obsIo->obs_group_.list();
            EXPECT(childGroups.size() == 0);
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
    }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSIO_H_
