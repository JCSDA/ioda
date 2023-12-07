/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_ENGINES_ENGINEFACTORY_H_
#define TEST_ENGINES_ENGINEFACTORY_H_

#include <memory>
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

class EngineFactoryTestObsDataInParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(EngineFactoryTestObsDataInParameters, Parameters)
 public:
  oops::RequiredParameter<Engines::ReaderParametersWrapper> engine{"engine", this};
};

class EngineFactoryTestObsDataOutParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(EngineFactoryTestObsDataOutParameters, Parameters)
 public:
  oops::RequiredParameter<Engines::WriterParametersWrapper> engine{"engine", this};
};

class EngineFactoryTestCaseCheckParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(EngineFactoryTestCaseCheckParameters, Parameters)
 public:
  oops::RequiredParameter<std::string> attrName{"attr name", this};
  oops::RequiredParameter<std::string> attrValue{"attr value", this};
};

class EngineFactoryTestCaseParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(EngineFactoryTestCaseParameters, Parameters)
 public:
  oops::RequiredParameter<std::string> name{"name", this};
  // Only the generators use the obsVarNames list and only "simulated variables" is a
  // required parameter (ie, "observed variables" is not required). So the logic to
  // create the ObsGroup associated with a generator (GenList, GenRandom) uses the list
  // specified by "simulated variables".
  oops::RequiredParameter<std::vector<std::string>> obsVarNames{"simulated variables", this};
  oops::OptionalParameter<EngineFactoryTestObsDataInParameters> obsDataIn{"obsdatain", this};
  oops::OptionalParameter<EngineFactoryTestObsDataOutParameters> obsDataOut{"obsdataout", this};
  oops::RequiredParameter<EngineFactoryTestCaseCheckParameters>
      check{"global attribute check", this};
};

CASE("ioda/GlobalAttributeCheck") {
  // This test is checking to make sure the Engine parameters and factory are in
  // sync. The parameters have a polymorphic structure (base class plus subclasses)
  // that mirrors that of the Engine base and subclasses.
  std::vector<eckit::LocalConfiguration> configs;
  ::test::TestEnvironment::config().get("engine factory tests", configs);

  for (std::size_t jj = 0; jj < configs.size(); ++jj) {
    // The idea here is to copy the case configuration (which mimics the structure
    // of the obs space config) into a local copy and add in the keywords that are
    // expected in the engine specification.
    eckit::LocalConfiguration testCaseConfig = configs[jj].getSubConfiguration("case");
    oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

    // Build the parameters structure, then use that to create the backend object.
    EngineFactoryTestCaseParameters params;
    params.validateAndDeserialize(testCaseConfig);

    bool isParallelIo = (oops::mpi::world().size() > 1);
    std::unique_ptr<Engines::ReaderBase> testReaderEngine;
    std::unique_ptr<Engines::WriterBase> testWriterEngine;
    if (params.obsDataIn.value() != boost::none) {
      eckit::LocalConfiguration timeWindowConfig;
      timeWindowConfig.set("begin", "2018-04-14T21:00:00Z");
      timeWindowConfig.set("end", "2018-04-15T03:00:00Z");

      Engines::ReaderCreationParameters createParams
        (util::TimeWindow(timeWindowConfig),
         oops::mpi::world(), oops::mpi::myself(), params.obsVarNames, isParallelIo);
      testReaderEngine = Engines::ReaderFactory::create(
          params.obsDataIn.value()->engine.value().engineParameters, createParams);
      oops::Log::info() << "Reader source: " << *testReaderEngine << std::endl;
    } else if (params.obsDataOut.value() != boost::none) {
      bool createMultipleFiles = false;
      Engines::WriterCreationParameters createParams(oops::mpi::world(), oops::mpi::myself(),
                                        createMultipleFiles, isParallelIo);
      testWriterEngine = Engines::WriterFactory::create(
          params.obsDataOut.value()->engine.value().engineParameters, createParams);
      oops::Log::info() << "Writer destination: " << *testWriterEngine << std::endl;
    }

    // Do a quick check on the value of an expected global attribute.
    // It is assumed that the selected attribute is a string.
    if (testReaderEngine.get() != nullptr) {
      oops::Log::debug() << "testReaderEngine atts: "
                         << testReaderEngine->getObsGroup().atts.list() << std::endl;
      std::string expectedVal = params.check.value().attrValue;
      std::string testVal = testReaderEngine->getObsGroup().atts
          .read<std::string>(params.check.value().attrName);
      EXPECT(testVal == expectedVal);
    }

    if (testWriterEngine.get() != nullptr) {
      oops::Log::debug() << "testWriterEngine atts: "
                         << testWriterEngine->getObsGroup().atts.list() << std::endl;
      testWriterEngine->getObsGroup().atts
          .add<std::string>(params.check.value().attrName, params.check.value().attrValue);
      std::string expectedVal = params.check.value().attrValue;
      std::string testVal = testWriterEngine->getObsGroup().atts
          .read<std::string>(params.check.value().attrName);
      EXPECT(testVal == expectedVal);
    }
  }
}

class EngineFactory : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::EngineFactory";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_ENGINES_ENGINEFACTORY_H_
