/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_IOPOOLFACTORY_H_
#define TEST_IODA_IOPOOLFACTORY_H_

#include <memory>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "ioda/Exception.h"
#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/ReaderPoolBase.h"
#include "ioda/ioPool/ReaderPoolFactory.h"
#include "ioda/ioPool/WriterPoolBase.h"
#include "ioda/ioPool/WriterPoolFactory.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"

namespace ioda {
namespace test {

class TestEngineParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(TestEngineParameters, oops::Parameters)

 public:
    /// option controlling the creation of a reader backend
    oops::RequiredParameter<Engines::ReaderParametersWrapper> readerEngine{"reader engine", this};

    /// option controlling the creation of a writer backend
    oops::RequiredParameter<Engines::WriterParametersWrapper> writerEngine{"writer engine", this};
};


CASE("ioda/WriterPoolFactoryMakers") {
    // Need dummy engine parameters for the io pool construction
    eckit::LocalConfiguration testEngineConfig;
    ::test::TestEnvironment::config().get("test engine specs", testEngineConfig);
    TestEngineParameters testEngineParams;
    testEngineParams.validateAndDeserialize(testEngineConfig);

    // Walk through the list of cases and test constructing the pools
    std::vector<eckit::LocalConfiguration> configs;
    ::test::TestEnvironment::config().get("writer pool factory tests", configs);
    for (std::size_t jj = 0; jj < configs.size(); ++jj) {
        // Each case has two sub configurations, one for the io pool and the other for
        // holding expected values.
        eckit::LocalConfiguration testCaseConfig = configs[jj].getSubConfiguration("case");
        oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

        // Grab the io pool and test data sub configurations, use the factory methods
        // to construct a pool subclass object and test for expected values.
        eckit::LocalConfiguration ioPoolConfig =
            testCaseConfig.getSubConfiguration("io pool");
        eckit::LocalConfiguration testDataConfig =
            testCaseConfig.getSubConfiguration("test data");

        IoPool::IoPoolParameters configParams;
        configParams.validateAndDeserialize(ioPoolConfig);

        std::vector<bool> expectedPatchObsVec(5, testDataConfig.getBool("patch obs vec"));
        IoPool::WriterPoolCreationParameters createParams(
            oops::mpi::world(), oops::mpi::myself(),
            testEngineParams.writerEngine.value().engineParameters, expectedPatchObsVec);
        std::unique_ptr<IoPool::WriterPoolBase> writerPool =
            IoPool::WriterPoolFactory::create(configParams, createParams);

        // Check if various data members got set properly
        int expectedCommAllRank = testDataConfig.getInt("comm all rank");
        int expectedCommAllSize = testDataConfig.getInt("comm all size");
        EXPECT(writerPool->commAll().rank() == expectedCommAllRank);
        EXPECT(writerPool->commAll().size() == expectedCommAllSize);

        std::vector<bool> patchObsVec = writerPool->patchObsVec();
        EXPECT(patchObsVec == expectedPatchObsVec);
    }
}

CASE("ioda/ReaderPoolFactoryMakers") {
    // Need dummy engine parameters for the io pool construction
    eckit::LocalConfiguration testEngineConfig;
    ::test::TestEnvironment::config().get("test engine specs", testEngineConfig);
    TestEngineParameters testEngineParams;
    testEngineParams.validateAndDeserialize(testEngineConfig);

    // Walk through the list of cases and test constructing the pools
    std::vector<eckit::LocalConfiguration> configs;
    ::test::TestEnvironment::config().get("reader pool factory tests", configs);
    for (std::size_t jj = 0; jj < configs.size(); ++jj) {
        // Each case has two sub configurations, one for the io pool and the other for
        // holding expected values.
        eckit::LocalConfiguration testCaseConfig = configs[jj].getSubConfiguration("case");
        oops::Log::info() << "Testing: " << testCaseConfig.getString("name") << std::endl;

        // Grab the io pool and test data sub configurations, use the factory methods
        // to construct a pool subclass object and test for expected values.
        eckit::LocalConfiguration ioPoolConfig =
            testCaseConfig.getSubConfiguration("io pool");
        eckit::LocalConfiguration testDataConfig =
            testCaseConfig.getSubConfiguration("test data");

        IoPool::IoPoolParameters configParams;
        configParams.validateAndDeserialize(ioPoolConfig);

        util::DateTime expectedWinStart(testDataConfig.getString("win start"));
        util::DateTime expectedWinEnd(testDataConfig.getString("win end"));
        std::vector<std::string> expectedObsVarNames =
            testDataConfig.getStringVector("obs var names");
        std::shared_ptr<Distribution> distribution;
        std::vector<std::string> expectedObsGroupVarList =
            testDataConfig.getStringVector("obs group var list");
        IoPool::ReaderPoolCreationParameters createParams(
            oops::mpi::world(), oops::mpi::myself(),
            testEngineParams.readerEngine.value().engineParameters,
            expectedWinStart, expectedWinEnd, expectedObsVarNames,
            distribution, expectedObsGroupVarList);
        std::unique_ptr<IoPool::ReaderPoolBase> readerPool =
            IoPool::ReaderPoolFactory::create(configParams, createParams);

        // Check if various data members got set properly
        int expectedCommAllRank = testDataConfig.getInt("comm all rank");
        int expectedCommAllSize = testDataConfig.getInt("comm all size");
        EXPECT(readerPool->commAll().rank() == expectedCommAllRank);
        EXPECT(readerPool->commAll().size() == expectedCommAllSize);

        util::DateTime winStart = readerPool->winStart();
        EXPECT(winStart == expectedWinStart);

        util::DateTime winEnd = readerPool->winEnd();
        EXPECT(winEnd == expectedWinEnd);

        std::vector<std::string> obsVarNames = readerPool->obsVarNames();
        EXPECT(obsVarNames == expectedObsVarNames);

        std::vector<std::string> obsGroupVarList = readerPool->obsGroupVarList();
        EXPECT(obsGroupVarList == expectedObsGroupVarList);

        std::string expectedWorkDirBase = testDataConfig.getString("work directory");
        EXPECT(readerPool->workDirBase() == expectedWorkDirBase);
    }
}
class IoPoolFactory : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::IoPoolFactory";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_IOPOOLFACTORY_H_
