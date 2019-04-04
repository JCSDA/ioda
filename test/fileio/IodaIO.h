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
#include "eckit/testing/Test.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "test/TestEnvironment.h"

#include "fileio/IodaIO.h"
#include "fileio/IodaIOfactory.h"

namespace ioda {
namespace test {

const util::DateTime bgn(1972, 3, 8, 0, 0, 0);
const util::DateTime end(2092, 3, 8, 0, 0, 0);

// -----------------------------------------------------------------------------

void testConstructor() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> obstypes;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;

  std::size_t Nlocs;
  std::size_t Nrecs;
  std::size_t Nvars;
  std::size_t ExpectedNlocs;
  std::size_t ExpectedNrecs;
  std::size_t ExpectedNvars;
  // Walk through the different ObsTypes and try constructing with the files.
  conf.get("ObsTypes", obstypes);
  for (std::size_t i = 0; i < obstypes.size(); ++i) {
    oops::Log::debug() << "IodaIO::ObsTypes: conf" << obstypes[i] << std::endl;

    TestObsType = obstypes[i].getString("ObsType");
    oops::Log::debug() << "IodaIO::ObsType: " << TestObsType << std::endl;

    FileName = obstypes[i].getString("Input.filename");
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, oops::mpi::comm()));
    EXPECT(TestIO.get());

    // Constructor in read mode is also responsible for setting nobs and nlocs
    ExpectedNlocs = obstypes[i].getInt("Input.metadata.nlocs");
    ExpectedNrecs = obstypes[i].getInt("Input.metadata.nrecs");
    ExpectedNvars = obstypes[i].getInt("Input.metadata.nvars");
    Nlocs = TestIO->nlocs();
    Nrecs = TestIO->nrecs();
    Nvars = TestIO->nvars();

    EXPECT(ExpectedNlocs == Nlocs);
    EXPECT(ExpectedNrecs == Nrecs);
    EXPECT(ExpectedNvars == Nvars);

    if (obstypes[i].has("Output.filename")) {
      FileName = obstypes[i].getString("Output.filename");
      ExpectedNlocs = obstypes[i].getInt("Output.metadata.nlocs");
      ExpectedNrecs = obstypes[i].getInt("Output.metadata.nrecs");
      ExpectedNvars = obstypes[i].getInt("Output.metadata.nvars");

      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", bgn, end, oops::mpi::comm(),
                                               ExpectedNlocs, ExpectedNrecs, ExpectedNvars));
      EXPECT(TestIO.get());

      Nlocs = TestIO->nlocs();
      Nrecs = TestIO->nrecs();
      Nvars = TestIO->nvars();

      EXPECT(ExpectedNlocs == Nlocs);
      EXPECT(ExpectedNrecs == Nrecs);
      EXPECT(ExpectedNvars == Nvars);
      }
    }
  }

// -----------------------------------------------------------------------------

void testReadVar() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::vector<eckit::LocalConfiguration> obstypes;
  std::vector<std::string> varnames;
  std::vector<float> ExpectedVnorms;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;
  std::size_t Vsize;
  std::unique_ptr<float[]> TestVarData;
  float Vnorm;
  float Tol;

  // Walk through the different ObsTypes and try constructing with the files.
  conf.get("ObsTypes", obstypes);
  for (std::size_t i = 0; i < obstypes.size(); ++i) {
    oops::Log::debug() << "IodaIO::ObsTypes: conf" << obstypes[i] << std::endl;

    TestObsType = obstypes[i].getString("ObsType");
    oops::Log::debug() << "IodaIO::ObsType: " << TestObsType << std::endl;

    FileName = obstypes[i].getString("Input.filename");
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, oops::mpi::comm()));

    // Read in data from the file and check values.
    varnames = obstypes[i].getStringVector("Input.variables");
    ExpectedVnorms = obstypes[i].getFloatVector("Input.metadata.norms");
    Tol = obstypes[i].getFloat("Input.metadata.tolerance");
    Vsize = TestIO->nlocs();
    TestVarData.reset(new float[Vsize]);
    for(std::size_t j = 0; j < varnames.size(); ++j) {
      TestIO->ReadVar(varnames[j], TestVarData.get());

      // Compute the vector length TestVarData and compare with config values
      Vnorm = 0.0;
      for(std::size_t k = 0; k < Vsize; ++k) {
        Vnorm += pow(TestVarData.get()[k], 2.0);
        }
      Vnorm = sqrt(Vnorm);

      EXPECT(oops::is_close(Vnorm, ExpectedVnorms[j], Tol));
      }
    }
  }

// -----------------------------------------------------------------------------

void testWriteVar() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> obstypes;
  std::vector<std::string> varnames;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;
  std::size_t Nlocs;
  std::size_t Nrecs;
  std::size_t Nvars;
  std::unique_ptr<float[]> TestVarData;

  std::size_t TestNlocs;
  std::size_t TestNrecs;
  std::size_t TestNvars;

  int VarSum;
  int ExpectedSum;

  // Walk through the different ObsTypes and try constructing with the files.
  conf.get("ObsTypes", obstypes);
  for (std::size_t i = 0; i < obstypes.size(); ++i) {
    oops::Log::debug() << "IodaIO::ObsTypes: conf" << obstypes[i] << std::endl;

    TestObsType = obstypes[i].getString("ObsType");
    oops::Log::debug() << "IodaIO::ObsType: " << TestObsType << std::endl;

    // Not all of the tests have output files
    if (obstypes[i].has("Output.filename")) {
      FileName = obstypes[i].getString("Output.filename");
      Nlocs = obstypes[i].getInt("Output.metadata.nlocs");
      Nrecs = obstypes[i].getInt("Output.metadata.nrecs");
      Nvars = obstypes[i].getInt("Output.metadata.nvars");
      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", bgn, end, oops::mpi::comm(),
                                               Nlocs, Nrecs, Nvars));

      // Try writing contrived data into the output file
      varnames = obstypes[i].getStringVector("Output.variables");
      TestVarData.reset(new float[Nlocs]);
      ExpectedSum = 0;
      for (std::size_t j = 0; j < Nlocs; ++j) {
        TestVarData.get()[j] = float(j);
        ExpectedSum += j;
        }
      ExpectedSum *= varnames.size();

      for(std::size_t j = 0; j < varnames.size(); ++j) {
        TestIO->WriteVar(varnames[j], TestVarData.get());
        }

      // open the file we just created and see if it contains what we just wrote into it
      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, oops::mpi::comm()));

      TestNlocs = TestIO->nlocs();
      TestNrecs = TestIO->nrecs();
      TestNvars = TestIO->nvars();

      EXPECT(TestNlocs == Nlocs);
      EXPECT(TestNrecs == Nrecs);
      EXPECT(TestNvars == Nvars);

      VarSum = 0;
      for(std::size_t j = 0; j < varnames.size(); ++j) {
        TestIO->ReadVar(varnames[j], TestVarData.get());
        for(std::size_t k = 0; k < Nlocs; ++k) {
          VarSum += int(TestVarData.get()[k]);
          }
        }

      EXPECT(VarSum == ExpectedSum);
      }
    }
  }

// -----------------------------------------------------------------------------

void testReadDateTime() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::vector<eckit::LocalConfiguration> obstypes;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;
  std::size_t Vsize;
  std::unique_ptr<uint64_t[]> TestVarDate;
  std::unique_ptr<int[]> TestVarTime;
  float Dnorm;
  float Tnorm;
  float ExpectedDnorm;
  float ExpectedTnorm;
  float Tol;

  // Walk through the different ObsTypes and try constructing with the files.
  conf.get("ObsTypes", obstypes);
  for (std::size_t i = 0; i < obstypes.size(); ++i) {
    oops::Log::debug() << "IodaIO::ObsTypes: conf" << obstypes[i] << std::endl;

    TestObsType = obstypes[i].getString("ObsType");
    oops::Log::debug() << "IodaIO::ObsType: " << TestObsType << std::endl;

    FileName = obstypes[i].getString("Input.filename");
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, oops::mpi::comm()));

    // Read in data from the file and check values.
    Vsize = TestIO->nlocs();
    TestVarDate.reset(new uint64_t[Vsize]);
    TestVarTime.reset(new int[Vsize]);
    TestIO->ReadDateTime(TestVarDate.get(), TestVarTime.get());

    // Compute the vector length TestVarData and compare with config values
    Dnorm = 0.0;
    Tnorm = 0.0;
    for(std::size_t k = 0; k < Vsize; ++k) {
      Dnorm += pow(float(TestVarDate.get()[k]), 2.0);
      Tnorm += pow(float(TestVarTime.get()[k]), 2.0);
      }
    Dnorm = sqrt(Dnorm);
    Tnorm = sqrt(Tnorm);

    ExpectedDnorm = obstypes[i].getFloat("Input.datetime.dnorm");
    ExpectedTnorm = obstypes[i].getFloat("Input.datetime.tnorm");
    Tol = obstypes[i].getFloat("Input.datetime.tolerance");

    EXPECT(oops::is_close(Dnorm, ExpectedDnorm, Tol));
    EXPECT(oops::is_close(Tnorm, ExpectedTnorm, Tol));
    }
  }

// -----------------------------------------------------------------------------

class IodaIO : public oops::Test {
 public:
  IodaIO() {}
  virtual ~IodaIO() {}
 private:
  std::string testid() const {return "test::IodaIO";}

  void register_tests() const {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

    ts.emplace_back(CASE("fileio/IodaIO/testConstructor")
      { testConstructor(); });
    ts.emplace_back(CASE("fileio/IodaIO/testReadVar")
      { testReadVar(); });
    ts.emplace_back(CASE("fileio/IodaIO/testWriteVar")
      { testWriteVar(); });
    ts.emplace_back(CASE("fileio/IodaIO/testReadDateTime")
      { testReadDateTime(); });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
