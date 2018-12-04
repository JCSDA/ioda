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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/noncopyable.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "test/TestEnvironment.h"

#include "fileio/IodaIO.h"
#include "fileio/IodaIOfactory.h"

namespace ioda {
namespace test {

const static double missingvalue = -9.9999e+299;
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
  std::size_t Nobs;
  std::size_t Nrecs;
  std::size_t Nvars;
  std::size_t ExpectedNobs;
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
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, missingvalue));
    BOOST_CHECK(TestIO.get());

    // Constructor in read mode is also responsible for setting nobs and nlocs
    ExpectedNlocs = obstypes[i].getInt("Input.metadata.nlocs");
    ExpectedNobs = obstypes[i].getInt("Input.metadata.nobs");
    ExpectedNrecs = obstypes[i].getInt("Input.metadata.nrecs");
    ExpectedNvars = obstypes[i].getInt("Input.metadata.nvars");
    Nlocs = TestIO->nlocs();
    Nobs = TestIO->nobs();
    Nrecs = TestIO->nrecs();
    Nvars = TestIO->nvars();

    BOOST_CHECK_EQUAL(ExpectedNlocs, Nlocs);
    BOOST_CHECK_EQUAL(ExpectedNobs, Nobs);
    BOOST_CHECK_EQUAL(ExpectedNrecs, Nrecs);
    BOOST_CHECK_EQUAL(ExpectedNvars, Nvars);

    if (obstypes[i].has("Output.filename")) {
      FileName = obstypes[i].getString("Output.filename");
      ExpectedNlocs = obstypes[i].getInt("Output.metadata.nlocs");
      ExpectedNobs = obstypes[i].getInt("Output.metadata.nobs");
      ExpectedNrecs = obstypes[i].getInt("Output.metadata.nrecs");
      ExpectedNvars = obstypes[i].getInt("Output.metadata.nvars");

      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", bgn, end, missingvalue,
                                               ExpectedNlocs, ExpectedNobs, ExpectedNrecs, ExpectedNvars));
      BOOST_CHECK(TestIO.get());

      Nlocs = TestIO->nlocs();
      Nobs = TestIO->nobs();
      Nrecs = TestIO->nrecs();
      Nvars = TestIO->nvars();

      BOOST_CHECK_EQUAL(ExpectedNlocs, Nlocs);
      BOOST_CHECK_EQUAL(ExpectedNobs, Nobs);
      BOOST_CHECK_EQUAL(ExpectedNrecs, Nrecs);
      BOOST_CHECK_EQUAL(ExpectedNvars, Nvars);
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
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, missingvalue));

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

      BOOST_CHECK_CLOSE(Vnorm, ExpectedVnorms[j], Tol);
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
  std::size_t Nobs;
  std::size_t Nrecs;
  std::size_t Nvars;
  std::unique_ptr<float[]> TestVarData;

  std::size_t TestNlocs;
  std::size_t TestNobs;
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
      Nobs = obstypes[i].getInt("Output.metadata.nobs");
      Nrecs = obstypes[i].getInt("Output.metadata.nrecs");
      Nvars = obstypes[i].getInt("Output.metadata.nvars");
      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", bgn, end, missingvalue, Nlocs, Nobs, Nrecs, Nvars));

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
      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, missingvalue));

      TestNlocs = TestIO->nlocs();
      TestNobs  = TestIO->nobs();
      TestNrecs = TestIO->nrecs();
      TestNvars = TestIO->nvars();

      BOOST_CHECK_EQUAL(TestNlocs, Nlocs);
      BOOST_CHECK_EQUAL(TestNobs, Nobs);
      BOOST_CHECK_EQUAL(TestNrecs, Nrecs);
      BOOST_CHECK_EQUAL(TestNvars, Nvars);

      VarSum = 0;
      for(std::size_t j = 0; j < varnames.size(); ++j) {
        TestIO->ReadVar(varnames[j], TestVarData.get());
        for(std::size_t k = 0; k < Nlocs; ++k) {
          VarSum += int(TestVarData.get()[k]);
          }
        }

      BOOST_CHECK_EQUAL(VarSum, ExpectedSum);
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
  std::unique_ptr<int[]> TestVarDate;
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
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", bgn, end, missingvalue));

    // Read in data from the file and check values.
    Vsize = TestIO->nlocs();
    TestVarDate.reset(new int[Vsize]);
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

    BOOST_CHECK_CLOSE(Dnorm, ExpectedDnorm, Tol);
    BOOST_CHECK_CLOSE(Tnorm, ExpectedTnorm, Tol);
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
    boost::unit_test::test_suite * ts = BOOST_TEST_SUITE("IodaIO");

    ts->add(BOOST_TEST_CASE(&testConstructor));
    ts->add(BOOST_TEST_CASE(&testReadVar));
    ts->add(BOOST_TEST_CASE(&testWriteVar));
    ts->add(BOOST_TEST_CASE(&testReadDateTime));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
