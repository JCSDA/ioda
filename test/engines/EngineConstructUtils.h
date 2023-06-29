/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_ENGINES_ENGINECONSTRUCTUTILS_H_
#define TEST_ENGINES_ENGINECONSTRUCTUTILS_H_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/ReaderFactory.h"
#include "ioda/Engines/WriterFactory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

namespace ioda {
namespace test {

CASE("ioda/engine/constructBackendConfig") {
  // This case checks to see if a proper eckit config can be created
  // with the constructBackendConfig utility.
  std::vector<eckit::LocalConfiguration> configs;
  ::test::TestEnvironment::config().get("construct config tests", configs);

  for (std::size_t jj = 0; jj < configs.size(); ++jj) {
    eckit::LocalConfiguration testCaseConfig = configs[jj].getSubConfiguration("case");
    oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

    std::string fileName = testCaseConfig.getString("file name");
    std::string fileType = testCaseConfig.getString("file type");
    std::string mapFileName("");
    std::string queryFileName("");
    if (fileType == "odb") {
        mapFileName = testCaseConfig.getString("mapping file name");
        queryFileName = testCaseConfig.getString("query file name");
    }

    eckit::LocalConfiguration engineConfig =
        Engines::constructFileBackendConfig(fileType, fileName, mapFileName, queryFileName);

    if (fileType == "hdf5") {
         std::string engineType = engineConfig.getString("engine.type");
         std::string engineObsFile = engineConfig.getString("engine.obsfile");

         EXPECT_EQUAL(engineType, "H5File");
         EXPECT_EQUAL(engineObsFile, fileName);
    } else if (fileType == "odb") {
         std::string engineType = engineConfig.getString("engine.type");
         std::string engineObsFile = engineConfig.getString("engine.obsfile");
         std::string engineMapFile = engineConfig.getString("engine.mapping file");
         std::string engineQueryFile = engineConfig.getString("engine.query file");

         EXPECT_EQUAL(engineType, "ODB");
         EXPECT_EQUAL(engineObsFile, fileName);
         EXPECT_EQUAL(engineMapFile, mapFileName);
         EXPECT_EQUAL(engineQueryFile, queryFileName);
    }
  }
}

CASE("ioda/engine/constructFileReaderFromConfig") {
  // This case checks to see if a proper file reader backend can be constructed
  // with the constructFileReaderFromConfig utility.
  std::vector<eckit::LocalConfiguration> configs;
  ::test::TestEnvironment::config().get("construct file reader tests", configs);

  for (std::size_t jj = 0; jj < configs.size(); ++jj) {
    eckit::LocalConfiguration testCaseConfig = configs[jj].getSubConfiguration("case");
    oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

    std::string fileName = testCaseConfig.getString("file name");
    std::string fileType = testCaseConfig.getString("file type");
    std::string mapFileName("");
    std::string queryFileName("");
    if (fileType == "odb") {
        mapFileName = testCaseConfig.getString("mapping file name");
        queryFileName = testCaseConfig.getString("query file name");
    }

    eckit::LocalConfiguration engineConfig =
        Engines::constructFileBackendConfig(fileType, fileName, mapFileName, queryFileName);

    util::DateTime winStart("2018-04-14T21:00:00Z");
    util::DateTime winEnd("2018-04-15T03:00:00Z");
    std::vector<std::string> obsVarNames{ "airTemperature", "specificHumidity" };
    bool isParallelIo = false;
    std::unique_ptr<Engines::ReaderBase> readerEngine =
        Engines::constructFileReaderFromConfig(winStart, winEnd, oops::mpi::world(),
                oops::mpi::myself(), obsVarNames, isParallelIo, engineConfig);

    // The engine file backend objects are set up to echo the file name they are
    // associated with via an io stream. Capture that file name in a string stream
    // and compare with the file name given in the test YAML to verify that the
    // backend got constructed properly.
    std::ostringstream ss;
    ss << *readerEngine;
    std::string backendFileName = ss.str();
    EXPECT_EQUAL(fileName, backendFileName);
  }
}

CASE("ioda/engine/constructFileWriterFromConfig") {
  // This case checks to see if a proper file writer backend can be constructed
  // with the constructFileWriterFromConfig utility.
  std::vector<eckit::LocalConfiguration> configs;
  ::test::TestEnvironment::config().get("construct file writer tests", configs);

  for (std::size_t jj = 0; jj < configs.size(); ++jj) {
    eckit::LocalConfiguration testCaseConfig = configs[jj].getSubConfiguration("case");
    oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

    std::string fileName = testCaseConfig.getString("file name");
    std::string fileType = testCaseConfig.getString("file type");
    std::string mapFileName("");
    std::string queryFileName("");
    if (fileType == "odb") {
        mapFileName = testCaseConfig.getString("mapping file name");
        queryFileName = testCaseConfig.getString("query file name");
    }

    eckit::LocalConfiguration engineConfig =
        Engines::constructFileBackendConfig(fileType, fileName, mapFileName, queryFileName);

    bool writeMultipleFiles = true;
    bool isParallelIo = false;
    std::unique_ptr<Engines::WriterBase> writerEngine =
        Engines::constructFileWriterFromConfig(oops::mpi::world(), oops::mpi::myself(),
                writeMultipleFiles, isParallelIo, engineConfig);

    // The engine file backend objects are set up to echo the file name they are
    // associated with via an io stream. Capture that file name in a string stream
    // and compare with the file name given in the test YAML to verify that the
    // backend got constructed properly.
    std::ostringstream ss;
    ss << *writerEngine;
    std::string backendFileName = ss.str();
    EXPECT_EQUAL(fileName, backendFileName);
  }
}
class EngineConstructUtils : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::EngineConstructUtils";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_ENGINES_ENGINECONSTRUCTUTILS_H_
