/*
 * (C) Copyright 2018-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACEPUTDBCHANNELS_H_
#define TEST_IODA_OBSSPACEPUTDBCHANNELS_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/make_unique.hpp>
#include "Eigen/Core"

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

#include "ioda/Engines/HH.h"
#include "ioda/ObsSpace.h"

namespace ioda {
namespace test {

CASE("ioda/ObsSpace/testPutDb") {
  const auto &topLevelConf = ::test::TestEnvironment::config();

  util::DateTime bgn(topLevelConf.getString("window begin"));
  util::DateTime end(topLevelConf.getString("window end"));

  std::vector<eckit::LocalConfiguration> confs;
  topLevelConf.get("observations", confs);

  for (const eckit::LocalConfiguration & conf : confs) {
    eckit::LocalConfiguration obsconf(conf, "obs space");

    std::unique_ptr<ObsSpace> obsspace = boost::make_unique<ObsSpace>(
          obsconf, oops::mpi::world(), bgn, end, oops::mpi::myself());

    const Dimensions_t nlocs = obsspace->nlocs();
    const Dimensions_t nchans = obsspace->nchans();
    const bool hasChannels = nchans != 0;

    std::vector<float> testVec1(nlocs), testVec2(nlocs);
    std::iota(testVec1.begin(), testVec1.end(), 1.0f);
    std::iota(testVec2.begin(), testVec2.end(), 2.0f);
    obsspace->put_db("DummyGroup", "multi_dimensional_var_2", testVec1);
    obsspace->put_db("DummyGroup", "multi_dimensional_var_4", testVec2);
    obsspace->put_db("MetaData", "single_dimensional_var_2", testVec1);
    obsspace->put_db("DummyGroup", "single_dimensional_var", testVec1);
    if (hasChannels) {
      // Channel 1000000 does not exist
      EXPECT_THROWS(obsspace->put_db("DummyGroup", "multi_dimensional_var_1000000", testVec1));
      // The variable single_dimensional_var already exists, but is not associated with the
      // nchans dimension
      EXPECT_THROWS(obsspace->put_db("DummyGroup", "single_dimensional_var_2", testVec1));
    }

    // Destroy the ObsSpace to force an output file to be written
    obsspace = nullptr;

    // Read the output file and check its contents are correct

    const std::string fileName = uniquifyFileName(obsconf.getString("obsdataout.obsfile"), 0, -1);
    const ioda::Group group = ioda::Engines::HH::openFile(
          fileName, ioda::Engines::BackendOpenModes::Read_Only);

    if (hasChannels) {
      const Variable nchansVar = group.vars.open("nchans");
      const std::vector<int> channels = nchansVar.readAsVector<int>();
      const std::size_t channel2Index =
          std::find(channels.begin(), channels.end(), 2) - channels.begin();
      const std::size_t channel4Index =
          std::find(channels.begin(), channels.end(), 4) - channels.begin();

      {
        const Variable var = group.vars.open("DummyGroup/multi_dimensional_var");
        const Dimensions dims = var.getDimensions();
        EXPECT_EQUAL(dims.dimensionality, 2);
        const std::vector<Dimensions_t> expectedDimsCur{nlocs, nchans};
        EXPECT_EQUAL(dims.dimsCur, expectedDimsCur);

        Eigen::ArrayXXf values;
        var.readWithEigenRegular(values);
        std::vector<float> channelValues(nlocs);
        for (std::size_t loc = 0; loc < nlocs; ++loc)
          channelValues[loc] = values(loc, channel2Index);
        EXPECT_EQUAL(channelValues, testVec1);

        for (std::size_t loc = 0; loc < nlocs; ++loc)
          channelValues[loc] = values(loc, channel4Index);
        EXPECT_EQUAL(channelValues, testVec2);
      }
    } else {  // has no channels
      {
        const Variable var = group.vars.open("DummyGroup/multi_dimensional_var_2");
        const Dimensions dims = var.getDimensions();
        EXPECT_EQUAL(dims.dimensionality, 1);
        const std::vector<Dimensions_t> expectedDimsCur{nlocs};
        EXPECT_EQUAL(dims.dimsCur, expectedDimsCur);

        const std::vector<float> values = var.readAsVector<float>();
        EXPECT_EQUAL(values, testVec1);
      }

      {
        const Variable var = group.vars.open("DummyGroup/multi_dimensional_var_4");
        const Dimensions dims = var.getDimensions();
        EXPECT_EQUAL(dims.dimensionality, 1);
        const std::vector<Dimensions_t> expectedDimsCur{nlocs};
        EXPECT_EQUAL(dims.dimsCur, expectedDimsCur);

        const std::vector<float> values = var.readAsVector<float>();
        EXPECT_EQUAL(values, testVec2);
      }
    }

    {
      const Variable var = group.vars.open("DummyGroup/single_dimensional_var");
      const Dimensions dims = var.getDimensions();
      EXPECT_EQUAL(dims.dimensionality, 1);
      const std::vector<Dimensions_t> expectedDimsCur{nlocs};
      EXPECT_EQUAL(dims.dimsCur, expectedDimsCur);

      const std::vector<float> values = var.readAsVector<float>();
      EXPECT_EQUAL(values, testVec1);
    }

    {
      const Variable var = group.vars.open("MetaData/single_dimensional_var_2");
      const Dimensions dims = var.getDimensions();
      EXPECT_EQUAL(dims.dimensionality, 1);
      const std::vector<Dimensions_t> expectedDimsCur{nlocs};
      EXPECT_EQUAL(dims.dimsCur, expectedDimsCur);

      const std::vector<float> values = var.readAsVector<float>();
      EXPECT_EQUAL(values, testVec1);
    }
  }
}

class ObsSpacePutDbChannels : public oops::Test {
 private:
  std::string testid() const override {return "test::ObsSpacePutDbChannels";}

  void register_tests() const override {}

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACEPUTDBCHANNELS_H_
