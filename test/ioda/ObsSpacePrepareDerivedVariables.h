/*
 * (C) Crown copyright 2026 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"
#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameUtils.h"
#include "IodaTestUtils.h"
#include "ioda/ObsSpace.h"
#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

class ObsSpaceTestFixture : private boost::noncopyable {
 public:
  static ioda::ObsSpace &obspace(const std::size_t ii) { return *getInstance().ospaces_.at(ii); }
  static std::size_t size() { return getInstance().ospaces_.size(); }
  static void cleanup() {
    auto &spaces = getInstance().ospaces_;
    for (auto &space : spaces) {
      space->save();
      space.reset();
    }
  }

 private:
  static ObsSpaceTestFixture &getInstance() {
    static ObsSpaceTestFixture theObsSpaceTestFixture;
    return theObsSpaceTestFixture;
  }

  ObsSpaceTestFixture() : ospaces_() {
    const util::TimeWindow timeWindow(
      ::test::TestEnvironment::config().getSubConfiguration("time window"));
    std::vector<eckit::LocalConfiguration> conf;
    ::test::TestEnvironment::config().get("observations", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      eckit::LocalConfiguration obsconf(conf[jj], "obs space");
      ioda::ObsTopLevelParameters obsparams;
      obsparams.validateAndDeserialize(obsconf);
      boost::shared_ptr<ioda::ObsSpace> tmp(
        new ioda::ObsSpace(obsconf, oops::mpi::world(), timeWindow, oops::mpi::myself()));
      ospaces_.push_back(tmp);
    }
  }

  ~ObsSpaceTestFixture() {}

  std::vector<boost::shared_ptr<ioda::ObsSpace> > ospaces_;
};

// -----------------------------------------------------------------------------

void testPrepareDerivedVariables() {
  typedef ObsSpaceTestFixture Test_;

  for (std::size_t jj = 0; jj < Test_::size(); ++jj) {
    ObsSpace &odb = Test_::obspace(jj);
    std::unique_ptr<osdf::IFrame> srcOsdf = osdf::createIFrame("FrameCols");
    std::vector<float> testLat = {0.0};

    /// Test srcOsdf containing exactly one column (not in osdf_)
    srcOsdf->appendNewColumn("fish", testLat, "fishUnit");
    EXPECT_THROWS_AS(odb.prepareSourceOsdfDerivedVariables(srcOsdf), eckit::BadParameter);

    /// Test srcOsdf containing exactly one column (in osdf_)
    srcOsdf->removeColumn("fish");
    srcOsdf->appendNewColumn(odb.listVariables()[0], testLat);
    odb.prepareSourceOsdfDerivedVariables(srcOsdf);

    // check operation results in same columns as in osdf_
    std::vector<std::string> srcColumns = srcOsdf->columnNames();
    std::vector<std::string> odbColumns = odb.listVariables();
    EXPECT(unorderedVectorComparison(srcColumns, odbColumns));

    // check operation doesn't add rows
    std::size_t numRowsAfter = srcOsdf->getData().getSizeRows();
    EXPECT_EQUAL(numRowsAfter, 1);

    // check operation adds correct missing values to completed row(s)
    std::vector<osdf::DataRow> srcOsdfDataRows;
    srcOsdfDataRows.reserve(1);
    srcOsdf->getData().getDataRows(srcOsdfDataRows);
    osdf::DataRow srcOsdfDataRow = srcOsdfDataRows[0];

    for (std::string column : odb.listVariables()) {
      osdf::FrameUtils::callWithSupportedType(
        srcOsdf->getColumnType(column), [&](auto typeDiscriminator) {
          using T = decltype(typeDiscriminator);
          const T missingValue = util::missingValue<T>();
          std::size_t columnIndex = srcOsdf->getData().getColumnMetadata().getIndex(column);
          if (columnIndex > 0) {
            std::shared_ptr<osdf::Datum<T>> datum = std::static_pointer_cast<osdf::Datum<T>>(
              srcOsdfDataRow.getColumn(columnIndex));
            EXPECT_EQUAL(missingValue, datum->getValue());
          }
        });
    }

    /// Test srcOsdf with more columns than osdf_ (should do nothing to srcOsdf)
    srcOsdf->appendNewColumn("fish", testLat, "fishUnit");
    srcColumns = srcOsdf->columnNames();
    odb.prepareSourceOsdfDerivedVariables(srcOsdf);

    std::vector<std::string> updatedSrcColumns = srcOsdf->columnNames();
    EXPECT(unorderedVectorComparison(srcColumns, updatedSrcColumns));
  }
}

// -----------------------------------------------------------------------------

class ObsSpacePrepareDerivedVariables : public oops::Test {
 public:
  ObsSpacePrepareDerivedVariables() {}
  virtual ~ObsSpacePrepareDerivedVariables() {}

 private:
    std::string testid() const override {
      return "test::ObsSpacePrepareDerivedVariables<ioda::IodaTrait>"; }

  void register_tests() const override {
    std::vector<eckit::testing::Test> &ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsSpacePrepareDerivedVariables/testPrepareDerivedVariables")
                    { testPrepareDerivedVariables(); });
  }

  void clear() const override {
    typedef ObsSpaceTestFixture Test_;
    Test_::cleanup();
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
