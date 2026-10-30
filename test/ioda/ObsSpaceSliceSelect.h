/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACESLICESELECT_H_
#define TEST_IODA_OBSSPACESLICESELECT_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/make_unique.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/HH.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpace.h"
#include "IodaTestUtils.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
/// \brief pull the 1-based slices in sliceNums out of a Location x slice-dim array
std::vector<float> selectSlices(const std::vector<float> & allValues, const std::size_t numSlices,
                                const std::vector<int> & sliceNums) {
  const std::size_t numLocs = allValues.size() / numSlices;
  std::vector<float> values;
  values.reserve(numLocs * sliceNums.size());
  for (std::size_t loc = 0; loc < numLocs; ++loc) {
    for (const int sliceNum : sliceNums) {
      values.push_back(allValues[loc * numSlices + (sliceNum - 1)]);
    }
  }
  return values;
}

// -----------------------------------------------------------------------------
// Slice selection on a non-Channel slice dimension. A numeric suffix
// ("var_<k>") or a get_db sliceSelect value is a 1-based position along the dimension,
// whatever the dimension's coordinate values are, and both containers must agree on that.
// The input files cover coordinate values that are not 1..N (e.g. N..1) and coordinate
// values that are not usable as keys (e.g. all zeros).
CASE("ioda/ObsSpace/testSliceSelect") {
  const auto & topLevelConf = ::test::TestEnvironment::config();
  const util::TimeWindow timeWindow(topLevelConf.getSubConfiguration("time window"));

  std::vector<eckit::LocalConfiguration> confs;
  topLevelConf.get("observations", confs);

  for (const eckit::LocalConfiguration & conf : confs) {
    eckit::LocalConfiguration obsconf(conf, "obs space");
    applyContainerDefault(topLevelConf, obsconf);
    const eckit::LocalConfiguration testconf(conf, "test data");
    const std::string group = testconf.getString("group");
    const std::string name = testconf.getString("name");
    const std::string dimName = testconf.getString("dimension");
    const std::vector<float> expectedValues = testconf.getFloatVector("expected values");
    const std::vector<double> expectedCoords =
        testconf.getDoubleVector("expected coordinate values");

    std::unique_ptr<ObsSpace> obsspace = boost::make_unique<ObsSpace>(
          obsconf, oops::mpi::world(), timeWindow, oops::mpi::myself());
    oops::Log::info() << "Obs space: " << obsspace->obsname() << ", container: "
                      << (obsspace->useDataframe() ? "OSDF" : "ObsGroup") << std::endl;

    // Whole variable, in file order
    std::vector<float> allValues;
    obsspace->get_db(group, name, allValues);
    EXPECT_EQUAL(allValues, expectedValues);
    const std::size_t numLocs = obsspace->nlocs();
    ASSERT(numLocs > 0);
    const std::size_t numSlices = allValues.size() / numLocs;
    ASSERT(numSlices >= 3);
    const int lastSlice = static_cast<int>(numSlices);

    // Numeric suffix on each slice, and has() on the first and last
    for (int k = 1; k <= lastSlice; ++k) {
      std::vector<float> values;
      obsspace->get_db(group, name + "_" + std::to_string(k), values);
      EXPECT_EQUAL(values, selectSlices(allValues, numSlices, {k}));
    }
    EXPECT(obsspace->has(group, name + "_1"));
    EXPECT(obsspace->has(group, name + "_" + std::to_string(lastSlice)));

    // sliceSelect
    {
      std::vector<float> values;
      obsspace->get_db(group, name, values, {1, 3});
      EXPECT_EQUAL(values, selectSlices(allValues, numSlices, {1, 3}));
    }

    // Positions outside 1..N
    {
      std::vector<float> values;
      const std::string outOfRange = std::to_string(lastSlice + 1);
      EXPECT_THROWS(obsspace->get_db(group, name + "_" + outOfRange, values));
      EXPECT_THROWS(obsspace->get_db(group, name, values, {lastSlice + 1}));
      EXPECT_THROWS(obsspace->get_db(group, name, values, {0}));
    }

    // Single slice write. The ObsGroup container only supports this on the Channel
    // dimension, so it is checked on OSDF only.
    if (obsspace->useDataframe()) {
      const std::vector<std::string> dimList{"Location", dimName};
      std::vector<float> newSlice(numLocs);
      for (std::size_t loc = 0; loc < numLocs; ++loc) newSlice[loc] = -1.0f - loc;
      obsspace->put_db(group, name + "_2", newSlice, dimList);

      std::vector<float> values;
      obsspace->get_db(group, name + "_2", values);
      EXPECT_EQUAL(values, newSlice);

      // Only slice 2 changed
      std::vector<float> expectedAll = allValues;
      for (std::size_t loc = 0; loc < numLocs; ++loc) {
        expectedAll[loc * numSlices + 1] = newSlice[loc];
      }
      obsspace->get_db(group, name, values);
      EXPECT_EQUAL(values, expectedAll);

      EXPECT_THROWS(obsspace->put_db(group, name + "_" + std::to_string(lastSlice + 1),
                                     newSlice, dimList));
    }

    // The output file keeps the input file's coordinate values for the slice dimension
    obsspace->save();
    obsspace.reset();
    const std::string fileName = obsconf.getString("obsdataout.engine.obsfile");
    const ioda::Group outGroup = ioda::Engines::HH::openFile(
          fileName, ioda::Engines::BackendOpenModes::Read_Only);
    const Variable coordVar = outGroup.vars.open(dimName);
    std::vector<double> coords;
    if (coordVar.isA<int>()) {
      const std::vector<int> intCoords = coordVar.readAsVector<int>();
      coords.assign(intCoords.begin(), intCoords.end());
    } else if (coordVar.isA<int64_t>()) {
      const std::vector<int64_t> int64Coords = coordVar.readAsVector<int64_t>();
      coords.assign(int64Coords.begin(), int64Coords.end());
    } else if (coordVar.isA<float>()) {
      const std::vector<float> floatCoords = coordVar.readAsVector<float>();
      coords.assign(floatCoords.begin(), floatCoords.end());
    } else {
      coords = coordVar.readAsVector<double>();
    }
    EXPECT_EQUAL(coords, expectedCoords);
  }
}

// -----------------------------------------------------------------------------

class ObsSpaceSliceSelect : public oops::Test {
 private:
  std::string testid() const override {return "test::ObsSpaceSliceSelect";}

  void register_tests() const override {}

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACESLICESELECT_H_
