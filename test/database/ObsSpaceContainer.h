/*
 * (C) Copyright 2009-2016 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef TEST_INTERFACE_OBSSPACE_H_
#define TEST_INTERFACE_OBSSPACE_H_

#include <string>
#include <cmath>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/mpi/Comm.h"
#include "eckit/testing/Test.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "test/TestEnvironment.h"

#include "database/MultiIndexContainer.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void testConstructor() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  util::DateTime bgn(conf.getString("window_begin"));
  util::DateTime end(conf.getString("window_end"));
  eckit::mpi::Comm & mpi_comm = eckit::mpi::comm();

  std::vector<eckit::LocalConfiguration> containers;

  std::string TestContainerName;
  std::unique_ptr<ioda::ObsSpaceContainer> TestContainer;

  // Walk through the different distribution types and try constructing.
  conf.get("Containers", containers);
  for (std::size_t i = 0; i < containers.size(); ++i) {
    oops::Log::debug() << "ObsSpaceContainer::DistributionTypes: conf"
                       << containers[i] << std::endl;

    TestContainerName = containers[i].getString("Container");
    oops::Log::debug() << "ObsSpaceContainer::DistType: " << TestContainerName << std::endl;

    // Instantiate a container
    TestContainer.reset(new ioda::ObsSpaceContainer(conf, bgn, end, mpi_comm));
    EXPECT(TestContainer.get());
    }
  }

// -----------------------------------------------------------------------------

void testInsertInquire() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  util::DateTime bgn(conf.getString("window_begin"));
  util::DateTime end(conf.getString("window_end"));
  eckit::mpi::Comm & mpi_comm = eckit::mpi::comm();

  std::vector<eckit::LocalConfiguration> containers;

  std::string TestContainerName;
  std::unique_ptr<ioda::ObsSpaceContainer> TestContainer;

  std::string FileName;
  std::size_t Nlocs;
  std::size_t Nvars;
  std::size_t ExpectedNlocs;
  std::size_t ExpectedNvars;

  // Walk through the different distribution types and try loading (insert) data
  // into the container, and extracting (inquire) data from the container.
  conf.get("Containers", containers);
  for (std::size_t i = 0; i < containers.size(); ++i) {
    oops::Log::debug() << "ObsSpaceContainer::DistributionTypes: conf"
                       << containers[i] << std::endl;

    TestContainerName = containers[i].getString("Container");
    oops::Log::debug() << "ObsSpaceContainer::DistType: " << TestContainerName << std::endl;

    // Instantiate a container
    TestContainer.reset(new ioda::ObsSpaceContainer(conf, bgn, end, mpi_comm));
    EXPECT(TestContainer.get());

    // Read in data and place in the container
    FileName = containers[i].getString("InData.filename");
    ExpectedNlocs = containers[i].getInt("InData.nlocs");
    ExpectedNvars = containers[i].getInt("InData.nvars");

    TestContainer->CreateFromFile(FileName, "r", bgn, end, mpi_comm);
    Nlocs = TestContainer->nlocs();
    Nvars = TestContainer->nvars();
    
    EXPECT(Nlocs == ExpectedNlocs);
    EXPECT(Nvars == ExpectedNvars);

    std::string GroupName = containers[i].getString("InData.group");
    std::string VarName = containers[i].getString("InData.variable");
    std::string VarType = containers[i].getString("InData.type");
    float ExpectedVnorm = containers[i].getFloat("InData.norm");
    float Tolerance = containers[i].getFloat("InData.tolerance");

    float Vnorm;
    if (VarType.compare("float") == 0) {
      std::vector<float> VarData(Nlocs);
      TestContainer->inquire(GroupName, VarName, Nlocs, VarData.data());
      for (std::size_t j = 0; j < Nlocs; j++) {
        Vnorm += pow(VarData[j], 2.0);
      }
    } else if (VarType.compare("integer") == 0) {
      std::vector<int> VarData(Nlocs);
      TestContainer->inquire(GroupName, VarName, Nlocs, VarData.data());
      for (std::size_t j = 0; j < Nlocs; j++) {
        Vnorm += pow(float(VarData[j]), 2.0);
      }
    }
    Vnorm = sqrt(Vnorm);
    EXPECT(oops::is_close(Vnorm, ExpectedVnorm, Tolerance));
  }
}

// -----------------------------------------------------------------------------

class ObsSpaceContainer : public oops::Test {
 public:
  ObsSpaceContainer() {}
  virtual ~ObsSpaceContainer() {}
 private:
  std::string testid() const {return "test::ObsSpaceContainer";}

  void register_tests() const {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("database/ObsSpaceContainer/testConstructor")
      { testConstructor; });
    ts.emplace_back(CASE("database/ObsSpaceContainer/testInsertInquire")
      { testInsertInquire; });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
