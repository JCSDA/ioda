/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSSPACESCHEMACONSISTENCY_H_
#define TEST_IODA_OBSSPACESCHEMACONSISTENCY_H_

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/TimeWindow.h"

#include "ioda/ObsSpace.h"
#include "IodaTestUtils.h"

namespace ioda {
namespace test {

// The cases below are only meaningful if some ranks hold locations and others do not. If they
// do not, the test is configured incorrectly, so throw a configuration error rather than fail
// as though ObsSpace were at fault.
void requirePopulatedAndEmptyRanks(const ioda::ObsSpace & obsSpace) {
  std::vector<int> nlocs{static_cast<int>(obsSpace.nlocs())};
  oops::mpi::allGatherv(obsSpace.comm(), nlocs);
  if (std::any_of(nlocs.begin(), nlocs.end(), [](int n) { return n == 0; }) &&
      std::any_of(nlocs.begin(), nlocs.end(), [](int n) { return n > 0; })) {
    return;
  }
  std::string nlocsList;
  for (std::size_t i = 0; i < nlocs.size(); ++i) {
    nlocsList += (i == 0 ? "" : ", ") + std::to_string(nlocs[i]);
  }
  throw eckit::UserError("ObsSpaceSchemaConsistency: obs space '" + obsSpace.obsname() +
                         "' needs some MPI tasks with locations and some without.", Here());
}

// Each case constructs its own ObsSpaces rather than sharing them through a fixture: a case
// that breaks the schema leaves an ObsSpace whose save() would throw.
std::vector<std::unique_ptr<ioda::ObsSpace>> createSchemaTestObsSpaces() {
  const util::TimeWindow timeWindow
    (::test::TestEnvironment::config().getSubConfiguration("time window"));
  std::vector<eckit::LocalConfiguration> configs;
  ::test::TestEnvironment::config().get("observations", configs);

  std::vector<std::unique_ptr<ioda::ObsSpace>> obsSpaces;
  for (const auto & config : configs) {
    eckit::LocalConfiguration obsConf(config, "obs space");
    applyContainerDefault(::test::TestEnvironment::config(), obsConf);
    obsSpaces.push_back(std::make_unique<ioda::ObsSpace>(obsConf, oops::mpi::world(),
                                                         timeWindow, oops::mpi::myself()));
    requirePopulatedAndEmptyRanks(*obsSpaces.back());
  }
  return obsSpaces;
}

// -----------------------------------------------------------------------------
// Creating a new variable on every rank, including ranks holding no locations, keeps the
// schema consistent.
void testPutDbOnAllRanks() {
  for (auto & obsSpace : createSchemaTestObsSpaces()) {
    obsSpace->checkSchemaConsistency("after construction");

    obsSpace->put_db("DerivedMetaData", "schemaTestFloat",
                     std::vector<float>(obsSpace->nlocs(), 1.0f));
    obsSpace->put_db("DerivedMetaData", "schemaTestInt",
                     std::vector<int>(obsSpace->nlocs(), 1));
    obsSpace->checkSchemaConsistency("after put_db on all ranks");
    obsSpace->save();
  }
}

// -----------------------------------------------------------------------------
// Creating a new variable only on ranks holding locations breaks the schema. Creating it on
// the remaining ranks repairs it.
void testPutDbOnPopulatedRanksOnly() {
  for (auto & obsSpace : createSchemaTestObsSpaces()) {
    const std::vector<float> values(obsSpace->nlocs(), 1.0f);
    if (obsSpace->nlocs() > 0) {
      obsSpace->put_db("DerivedMetaData", "schemaTestFloat", values);
    }
    EXPECT_THROWS_AS(obsSpace->checkSchemaConsistency("after put_db on populated ranks"),
                     eckit::BadValue);
    // save() checks the schema before doing anything else.
    EXPECT_THROWS_AS(obsSpace->save(), eckit::BadValue);

    if (obsSpace->nlocs() == 0) {
      obsSpace->put_db("DerivedMetaData", "schemaTestFloat", values);
    }
    obsSpace->checkSchemaConsistency("after put_db on the remaining ranks");
    obsSpace->save();
  }
}

// -----------------------------------------------------------------------------
// Creating the same variables in a different order on different ranks breaks the schema.
void testPutDbInDifferentOrder() {
  for (auto & obsSpace : createSchemaTestObsSpaces()) {
    const std::vector<float> floatValues(obsSpace->nlocs(), 1.0f);
    const std::vector<int> intValues(obsSpace->nlocs(), 1);
    if (obsSpace->nlocs() > 0) {
      obsSpace->put_db("DerivedMetaData", "schemaTestFloat", floatValues);
      obsSpace->put_db("DerivedMetaData", "schemaTestInt", intValues);
    } else {
      obsSpace->put_db("DerivedMetaData", "schemaTestInt", intValues);
      obsSpace->put_db("DerivedMetaData", "schemaTestFloat", floatValues);
    }
    EXPECT_THROWS_AS(obsSpace->checkSchemaConsistency("after put_db in different orders"),
                     eckit::BadValue);
  }
}

// -----------------------------------------------------------------------------
class ObsSpaceSchemaConsistency : public oops::Test {
 public:
  ObsSpaceSchemaConsistency() {}
  virtual ~ObsSpaceSchemaConsistency() {}

 private:
  std::string testid() const override {return "test::ObsSpaceSchemaConsistency";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpaceSchemaConsistency/testPutDbOnAllRanks")
      { testPutDbOnAllRanks(); });
    ts.emplace_back(CASE("ioda/ObsSpaceSchemaConsistency/testPutDbOnPopulatedRanksOnly")
      { testPutDbOnPopulatedRanksOnly(); });
    ts.emplace_back(CASE("ioda/ObsSpaceSchemaConsistency/testPutDbInDifferentOrder")
      { testPutDbInDifferentOrder(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSSPACESCHEMACONSISTENCY_H_
