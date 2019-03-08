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
#include <typeinfo>

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/noncopyable.hpp>

#include <boost/any.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/util/Logger.h"
#include "test/TestEnvironment.h"

#include "fileio/IodaIO.h"
#include "fileio/IodaIOfactory.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

void ExtractGrpVarName(const std::string & GrpVarName, std::string & GroupName,
                       std::string & VarName) {
  std::size_t Spos = GrpVarName.find("@");
  if (Spos != GrpVarName.npos) {
    GroupName = GrpVarName.substr(Spos+1);
    VarName = GrpVarName.substr(0, Spos);
  } else {
    GroupName = "GroupUndefined";
    VarName = GrpVarName;
  }
}

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
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));
    BOOST_CHECK(TestIO.get());

    // Constructor in read mode is also responsible for setting nobs and nlocs
    ExpectedNlocs = obstypes[i].getInt("Input.metadata.nlocs");
    ExpectedNrecs = obstypes[i].getInt("Input.metadata.nrecs");
    ExpectedNvars = obstypes[i].getInt("Input.metadata.nvars");
    Nlocs = TestIO->nlocs();
    Nrecs = TestIO->nrecs();
    Nvars = TestIO->nvars();

    BOOST_CHECK_EQUAL(ExpectedNlocs, Nlocs);
    BOOST_CHECK_EQUAL(ExpectedNrecs, Nrecs);
    BOOST_CHECK_EQUAL(ExpectedNvars, Nvars);

    if (obstypes[i].has("Output.filename")) {
      FileName = obstypes[i].getString("Output.filename");
      ExpectedNlocs = obstypes[i].getInt("Output.metadata.nlocs");
      ExpectedNrecs = obstypes[i].getInt("Output.metadata.nrecs");
      ExpectedNvars = obstypes[i].getInt("Output.metadata.nvars");

      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", ExpectedNlocs,
                                               ExpectedNrecs, ExpectedNvars));
      BOOST_CHECK(TestIO.get());

      Nlocs = TestIO->nlocs();
      Nrecs = TestIO->nrecs();
      Nvars = TestIO->nvars();

      BOOST_CHECK_EQUAL(ExpectedNlocs, Nlocs);
      BOOST_CHECK_EQUAL(ExpectedNrecs, Nrecs);
      BOOST_CHECK_EQUAL(ExpectedNvars, Nvars);
      }
    }
  }

// -----------------------------------------------------------------------------

void testGrpVarIter() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> obstypes;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;

  // Walk through the different ObsTypes and try out the group variable iterators.
  conf.get("ObsTypes", obstypes);
  for (std::size_t i = 0; i < obstypes.size(); ++i) {
    oops::Log::debug() << "IodaIO::ObsTypes: conf" << obstypes[i] << std::endl;

    TestObsType = obstypes[i].getString("ObsType");
    oops::Log::debug() << "IodaIO::ObsType: " << TestObsType << std::endl;

    FileName = obstypes[i].getString("Input.filename");
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));
    BOOST_CHECK(TestIO.get());

    // Test the iterators by walking through the entire list of variables
    // and check the count of variables (total number in the file) with the
    // expected count.
    std::size_t VarCount;
    std::size_t ExpectedVarCount = obstypes[i].getInt("Input.nvars_in_file");
    for (IodaIO::GroupIter igrp = TestIO->group_begin();
                           igrp != TestIO->group_end(); igrp++) {
      std::string GroupName = TestIO->group_name(igrp);

      for (IodaIO::VarIter ivar = TestIO->var_begin(igrp);
                           ivar != TestIO->var_end(igrp); ivar++) {
        VarCount++;
      }
    }
    BOOST_CHECK_EQUAL(VarCount, ExpectedVarCount);
  }
}

// -----------------------------------------------------------------------------

void testReadVar() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::vector<eckit::LocalConfiguration> obstypes;
  std::vector<std::string> GrpVarNames;
  std::vector<float> ExpectedVnorms;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;
  std::size_t Vsize;
  std::unique_ptr<boost::any[]> TestVarData;
  float Vnorm;
  float Tol;

  // Walk through the different ObsTypes and try constructing with the files.
  conf.get("ObsTypes", obstypes);
  for (std::size_t i = 0; i < obstypes.size(); ++i) {
    oops::Log::debug() << "IodaIO::ObsTypes: conf" << obstypes[i] << std::endl;

    TestObsType = obstypes[i].getString("ObsType");
    oops::Log::debug() << "IodaIO::ObsType: " << TestObsType << std::endl;

    FileName = obstypes[i].getString("Input.filename");
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));

    // Read in data from the file and check values.
    GrpVarNames = obstypes[i].getStringVector("Input.variables");
    ExpectedVnorms = obstypes[i].getFloatVector("Input.metadata.norms");
    Tol = obstypes[i].getFloat("Input.metadata.tolerance");
    Vsize = TestIO->nlocs();
    IodaIO::VarDimList VarShape(1, Vsize);
    TestVarData.reset(new boost::any[Vsize]);
    for(std::size_t j = 0; j < GrpVarNames.size(); ++j) {
      // Split out variable and group names
      std::string VarName;
      std::string GroupName;
      ExtractGrpVarName(GrpVarNames[j], GroupName, VarName);

      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData.get());

      const std::type_info & TestVarDtype = TestVarData.get()->type();
      // Compute the vector length TestVarData and compare with config values
      Vnorm = 0.0;
      for(std::size_t k = 0; k < Vsize; ++k) {
        if (TestVarDtype == typeid(int)) {
          Vnorm += pow(static_cast<double>(boost::any_cast<int>(TestVarData.get()[k])), 2.0);
        } else if (TestVarDtype == typeid(float)) {
          Vnorm += pow(boost::any_cast<float>(TestVarData.get()[k]), 2.0);
        }
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
  std::vector<std::string> GrpVarNames;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;
  std::size_t Nlocs;
  std::size_t Nrecs;
  std::size_t Nvars;
  std::unique_ptr<boost::any[]> TestVarData;

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
      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", Nlocs, Nrecs, Nvars));

      // Try writing contrived data into the output file
      GrpVarNames = obstypes[i].getStringVector("Output.variables");
      IodaIO::VarDimList VarShape{ Nlocs };
      TestVarData.reset(new boost::any[Nlocs]);
      ExpectedSum = 0;
      for (std::size_t j = 0; j < Nlocs; ++j) {
        TestVarData.get()[j] = float(j);
        ExpectedSum += j;
        }
      ExpectedSum *= GrpVarNames.size();

      for(std::size_t j = 0; j < GrpVarNames.size(); ++j) {
        std::string VarName;
        std::string GroupName;
        ExtractGrpVarName(GrpVarNames[j], GroupName, VarName);

        TestIO->WriteVar(GroupName, VarName, VarShape, TestVarData.get());
        }

      // open the file we just created and see if it contains what we just wrote into it
      TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));

      TestNlocs = TestIO->nlocs();
      TestNrecs = TestIO->nrecs();
      TestNvars = TestIO->nvars();

      BOOST_CHECK_EQUAL(TestNlocs, Nlocs);
      BOOST_CHECK_EQUAL(TestNrecs, Nrecs);
      BOOST_CHECK_EQUAL(TestNvars, Nvars);

      VarSum = 0;
      for(std::size_t j = 0; j < GrpVarNames.size(); ++j) {
        std::string VarName;
        std::string GroupName;
        ExtractGrpVarName(GrpVarNames[j], GroupName, VarName);

        TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData.get());
        for(std::size_t k = 0; k < Nlocs; ++k) {
          VarSum += int(boost::any_cast<float>(TestVarData.get()[k]));
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
    TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));

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
    ts->add(BOOST_TEST_CASE(&testGrpVarIter));
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
