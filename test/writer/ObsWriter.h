/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/testing/Test.h"

#include "ioda/containers/CreateIFrame.h"
#include "ioda/containers/FrameMetadata.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/ObsSourceStats.h"
#include "ioda/distribution/DistributionParametersBase.h"
#include "ioda/distribution/IdentityDistribution.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ObsDataIoParameters.h"
#include "ioda/obsIoPool/ObsIoPool.hpp"
#include "ioda/test/containers/OsdfTestUtils.h"
#include "ioda/writer/ObsWriter.hpp"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
void testFrames() {
  const eckit::mpi::Comm & commAll = oops::mpi::world();
  // Configuration contains a list of obs types, each with a list
  // of columns. Each column has a name, type, and values. These are
  // organized by MPI size and rank in order to allow for testing different
  // MPI writing scenarios.
  const std::vector<eckit::LocalConfiguration> testConfigs =
      ::test::TestEnvironment::config().getSubConfigurations("obs types");
  for (const auto & testConfig : testConfigs) {
    for (const auto & frameType : std::vector<std::string>{"FrameCols", "FrameRows"}) {
      const std::string testName = testConfig.getString("name");
      oops::Log::info() << "Testing: " << testName << " (" << frameType << ")" << std::endl;

      // Build the per-rank column configuration key once; reused for each preserveInputs case.
      const eckit::LocalConfiguration testDataConfig =
        testConfig.getSubConfiguration("test data");
      const std::string mpiSizeRankKey = "mpi size" + std::to_string(commAll.size()) +
                                         ".rank" + std::to_string(commAll.rank());
      const std::vector<eckit::LocalConfiguration> columnsConfig =
        testDataConfig.getSubConfiguration(mpiSizeRankKey).getSubConfigurations("columns");

      // Total number of observations across all MPI tasks (used for sourceNlocs and
      // for the preserveInputs=false validation).
      const std::size_t totalLocs = testDataConfig.getUnsigned("number of locations");

      for (const bool preserveInputs : std::vector<bool>{true, false}) {
        oops::Log::info() << "  preserveInputs=" << preserveInputs << std::endl;

        // ---- parameters for obsdataout ----
        // Append frame type and preserveInputs tag and MPI size to the output filename
        // so that all combinations produce unique files that can coexist on disk.
        eckit::LocalConfiguration obsDataOutConfig =
          testConfig.getSubConfiguration("obsdataout");
        std::string abbrFtype = (frameType == "FrameRows") ? "_frows" : "_fcols";
        std::string abbrPI    = preserveInputs ? "_pi" : "_npi";
        const std::string testFileTag = abbrFtype + abbrPI +
                                        std::string("_mpi") +
                                        std::to_string(commAll.size());
        std::string outputFileName = obsDataOutConfig.getString("engine.obsfile");
        outputFileName.insert(outputFileName.find_last_of('.'), testFileTag);
        obsDataOutConfig.set("engine.obsfile", outputFileName);
        ioda::ObsDataOutParameters dataOutParams;
        dataOutParams.deserialize(obsDataOutConfig);

        // ---- io pool parameters ----
        const eckit::LocalConfiguration ioPoolConfig =
          testConfig.getSubConfiguration("io pool");
        ioda::IoPool::IoPoolParameters ioPoolParams;
        ioPoolParams.deserialize(ioPoolConfig);

        // ---- source OSDF frame ----
        std::unique_ptr<osdf::IFrame> srcOsdf = osdf::createIFrame(frameType);
        std::vector<std::string> columnNames;
        std::vector<std::string> columnTypes;
        populateFrame(columnsConfig, srcOsdf, columnNames, columnTypes);

        // ---- reference frame for preserveInputs=true validation ----
        // Populate from the same config so it is an identical copy of srcOsdf.
        std::unique_ptr<osdf::IFrame> refOsdf = osdf::createIFrame(frameType);
        std::vector<std::string> refColumnNames;
        std::vector<std::string> refColumnTypes;
        populateFrame(columnsConfig, refOsdf, refColumnNames, refColumnTypes);

        // ---- frame metadata ----
        osdf::FrameMetadata osdfMetadata;
        populateFrameMetadata(
          testDataConfig.getSubConfiguration("frame metadata"), osdfMetadata);

        // ---- distribution: Identity means each rank owns exactly its loaded data ----
        std::shared_ptr<Distribution> ospaceDist =
          std::make_shared<IdentityDistribution>(commAll,
                                                 IdentityDistribution::Parameters_{});
        ospaceDist->setNumberLocations(srcOsdf->numRows());

        // ---- ObsSourceStats: sourceNlocs is the global total across all ranks ----
        ioda::ObsSourceStats obsSourceStats;
        obsSourceStats.sourceNlocs = totalLocs;

        if (preserveInputs) {
          // Call obsWrite. srcOsdf must NOT be modified.
          ioda::writer::obsWrite(dataOutParams, ioPoolParams, commAll, "ObsWriter test",
                                 ospaceDist, srcOsdf, obsSourceStats, osdfMetadata,
                                 true);

          // Validate that the source frame is identical to the reference frame.
          compareFrames(srcOsdf, columnNames, columnTypes,
                        refOsdf, refColumnNames, refColumnTypes, 1e-5, false);
        } else {
          // Determine pool membership before calling obsWrite (obsWrite creates its own
          // internal ObsIoPool with the same params, so pool membership is identical).
          std::unique_ptr<ObsIoPool::ObsIoPool> tmpPool =
            std::make_unique<ObsIoPool::ObsIoPool>(ioPoolParams, commAll);
          const bool inIoPool = tmpPool->inIoPool();
          tmpPool.reset();

          // Call obsWrite. srcOsdf WILL be modified: all data moves to ioPool ranks.
          ioda::writer::obsWrite(dataOutParams, ioPoolParams, commAll, "ObsWriter test",
                                 ospaceDist, srcOsdf, obsSourceStats, osdfMetadata,
                                 false);

          // Validate that all locations were collected onto the ioPool rank and that
          // non-ioPool ranks have no remaining data.
          if (inIoPool) {
            EXPECT(srcOsdf->numRows() == totalLocs);
          } else {
            EXPECT(srcOsdf->numRows() == 0);
          }
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
class ObsWriter : public oops::Test {
 public:
  ObsWriter() {}
  virtual ~ObsWriter() {}

 private:
  std::string testid() const override {return "test::ObsWriter";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("ioda/ObsWriter/testFrames")
      { testFrames(); });
  }

  void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda
