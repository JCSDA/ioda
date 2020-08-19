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

    std::unique_ptr<ioda::ObsIoParameters> obsParams;
    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        confOspaces[i].get("obs space", obsConfig);
        oops::Log::trace() << "ObsIo test config: " << i << ": " << obsConfig << std::endl;

        obsParams.reset(new ioda::ObsIoParameters());   
        obsParams->deserialize(obsConfig);

        int dummy = 1; /// delete this
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
