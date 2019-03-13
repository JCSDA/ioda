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

std::vector<std::string> CharArrayToStringVector(const char * VarData,
                                                 const std::vector<std::size_t> VarShape) {
  // VarShape[0] is the number of strings
  // VarShape[1] is the length of each string
  std::size_t Nstrings = VarShape[0];
  std::size_t StrLength = VarShape[1];

  std::vector<std::string> StringVector(Nstrings,"");
  for (std::size_t i = 0; i < Nstrings; i++) {
    // Copy characters for i-th string into a char vector
    std::vector<char> CharVector(StrLength, ' ');
    for (std::size_t j = 0; j < StrLength; j++) {
      CharVector[j] = VarData[(i*StrLength) + j];
    }

    // Convert the char vector to a single string. Any trailing white space will be
    // included in the string, so strip off the trailing white space.
    std::string String(CharVector.begin(), CharVector.end());
    String.erase(String.find_last_not_of(" \t\n\r\f\v") + 1);
    StringVector[i] = String;
  }

  return StringVector;
}

// -----------------------------------------------------------------------------

void testConstructor() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::string FileName;
  std::unique_ptr<ioda::IodaIO> TestIO;

  std::size_t Nlocs;
  std::size_t Nrecs;
  std::size_t Nvars;
  std::size_t ExpectedNlocs;
  std::size_t ExpectedNrecs;
  std::size_t ExpectedNvars;

  // Contructor in read mode
  FileName = conf.getString("TestInput.filename");
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));
  BOOST_CHECK(TestIO.get());

  // Constructor in read mode is also responsible for setting nobs and nlocs
  ExpectedNlocs = conf.getInt("TestInput.nlocs");
  ExpectedNrecs = conf.getInt("TestInput.nrecs");
  ExpectedNvars = conf.getInt("TestInput.nvars");

  Nlocs = TestIO->nlocs();
  Nrecs = TestIO->nrecs();
  Nvars = TestIO->nvars();

  BOOST_CHECK_EQUAL(ExpectedNlocs, Nlocs);
  BOOST_CHECK_EQUAL(ExpectedNrecs, Nrecs);
  BOOST_CHECK_EQUAL(ExpectedNvars, Nvars);

  // Constructor in write mode
  FileName = conf.getString("TestOutput.filename");

  ExpectedNlocs = conf.getInt("TestOutput.nlocs");
  ExpectedNrecs = conf.getInt("TestOutput.nrecs");
  ExpectedNvars = conf.getInt("TestOutput.nvars");

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

// -----------------------------------------------------------------------------

void testGrpVarIter() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> obstypes;

  std::string FileName;
  std::unique_ptr<ioda::IodaIO> TestIO;

  // Constructor in read mode will generate a group variable container.
  FileName = conf.getString("TestInput.filename");
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));
  BOOST_CHECK(TestIO.get());

  // Test the iterators by walking through the entire list of variables
  // and check the count of variables (total number in the file) with the
  // expected count.
  std::size_t VarCount = 0;
  std::size_t ExpectedVarCount = conf.getInt("TestInput.nvars_in_file");
  for (IodaIO::GroupIter igrp = TestIO->group_begin(); igrp != TestIO->group_end(); igrp++) {
    std::string GroupName = TestIO->group_name(igrp);

    for (IodaIO::VarIter ivar = TestIO->var_begin(igrp); ivar != TestIO->var_end(igrp); ivar++) {
      VarCount++;
    }
  }
  BOOST_CHECK_EQUAL(VarCount, ExpectedVarCount);
}

// -----------------------------------------------------------------------------

void testReadVar() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::vector<eckit::LocalConfiguration> obstypes;
  std::vector<std::string> GrpVarNames;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;

  FileName = conf.getString("TestInput.filename");
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));

  // Read in data from the file and check values.
  GrpVarNames = conf.getStringVector("TestInput.variables");
  for(std::size_t i = 0; i < GrpVarNames.size(); ++i) {
    // Split out variable and group names
    std::string VarName;
    std::string GroupName;
    ExtractGrpVarName(GrpVarNames[i], GroupName, VarName);

    // Get decriptions of variable
    std::string VarType = TestIO->var_dtype(GroupName, VarName);
    std::vector<std::size_t> VarShape = TestIO->var_shape(GroupName, VarName);
    std::size_t VarSize = 1;
    for (std::size_t j = 0; j < VarShape.size(); j++) {
      VarSize *= VarShape[j];
    }

    // Read and check the variable contents
    std::string ExpectedVarDataName = "TestInput.var" + std::to_string(i);
    float Tolerance = conf.getFloat("TestInput.tolerance");
    if (VarType.compare("int") == 0) {
      std::vector<int> TestVarData(VarSize, 0);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData.data());
      std::vector<int> ExpectedVarData = conf.getIntVector(ExpectedVarDataName);
      BOOST_CHECK_EQUAL_COLLECTIONS(TestVarData.begin(), TestVarData.end(),
                                    ExpectedVarData.begin(), ExpectedVarData.end());
    } else if ((VarType.compare("float") == 0) or (VarType.compare("double") == 0)) {
      std::vector<float> TestVarData(VarSize, 0.0);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData.data());
      std::vector<float> ExpectedVarData = conf.getFloatVector(ExpectedVarDataName);
      for (std::size_t j = 0; j < TestVarData.size(); j++) {
        BOOST_CHECK_CLOSE(TestVarData[j], ExpectedVarData[j], Tolerance);
      }
    } else if (VarType.compare("char") == 0) {
      std::unique_ptr<char[]> TestVarData(new char[VarSize]);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData.get());
      std::vector<std::string> TestStrings =
                               CharArrayToStringVector(TestVarData.get(), VarShape);
      std::vector<std::string> ExpectedVarData = conf.getStringVector(ExpectedVarDataName);
      BOOST_CHECK_EQUAL_COLLECTIONS(TestStrings.begin(), TestStrings.end(),
                                    ExpectedVarData.begin(), ExpectedVarData.end());
    }
  }
}

// -----------------------------------------------------------------------------

void testWriteVar() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::unique_ptr<ioda::IodaIO> TestIO;

  // Try writing contrived data into the output file. One of each data type, and size.
  std::string FileName = conf.getString("TestOutput.filename");
  std::size_t ExpectedNlocs = conf.getInt("TestOutput.nlocs");
  std::size_t ExpectedNrecs = conf.getInt("TestOutput.nrecs");
  std::size_t ExpectedNvars = conf.getInt("TestOutput.nvars");
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", ExpectedNlocs,
                                           ExpectedNrecs, ExpectedNvars));

  // Float data
  std::vector<float> ExpectedFloatData(ExpectedNlocs, 0.0);
  for (std::size_t i = 0; i < ExpectedNlocs; ++i) {
    ExpectedFloatData[i] = float(i) + 0.5;
  }
  std::vector<std::size_t> FloatVarShape{ ExpectedNlocs };
  std::string FloatGrpName = "MetaData";
  std::string FloatVarName = "test_float";
  TestIO->WriteVar(FloatGrpName, FloatVarName, FloatVarShape, ExpectedFloatData.data());

  // Int data
  std::vector<int> ExpectedIntData(ExpectedNvars, 0);
  for (std::size_t i = 0; i < ExpectedNvars; ++i) {
    ExpectedIntData[i] = i * 2;
  }
  std::vector<std::size_t> IntVarShape{ ExpectedNvars };
  std::string IntGrpName = "VarMetaData";
  std::string IntVarName = "test_int";
  TestIO->WriteVar(IntGrpName, IntVarName, IntVarShape, ExpectedIntData.data());

  // Char data
  std::string TempString;
  std::unique_ptr<char[]> ExpectedCharData(new char[ExpectedNrecs * 5]);
  TempString = "HelloWorld";
  for (std::size_t i = 0; i < TempString.size(); i++) {
    ExpectedCharData.get()[i] = TempString[i];
  }
  std::vector<std::size_t> CharVarShape{ ExpectedNrecs, 5 };
  std::string CharGrpName = "RecMetaData";
  std::string CharVarName = "test_char";
  TestIO->WriteVar(CharGrpName, CharVarName, CharVarShape, ExpectedCharData.get());

  // Try char data with same string size
  std::unique_ptr<char[]> ExpectedChar2Data(new char[ExpectedNvars * 5]);
  TempString = "12345aaaaa67890bbbbbABCDE";
  for (std::size_t i = 0; i < TempString.size(); i++) {
    ExpectedChar2Data.get()[i] = TempString[i];
  }
  std::vector<std::size_t> Char2VarShape{ ExpectedNvars, 5 };
  std::string Char2GrpName = "VarMetaData";
  std::string Char2VarName = "test_char2";
  TestIO->WriteVar(Char2GrpName, Char2VarName, Char2VarShape, ExpectedChar2Data.get());

  // Try char data with different shape
  std::unique_ptr<char[]> ExpectedChar3Data(new char[ExpectedNrecs * 10]);
  TempString = "abcdefghij1234567890";
  for (std::size_t i = 0; i < TempString.size(); i++) {
    ExpectedChar3Data.get()[i] = TempString[i];
  }
  std::vector<std::size_t> Char3VarShape{ ExpectedNrecs, 10 };
  std::string Char3GrpName = "RecMetaData";
  std::string Char3VarName = "test_char3";
  TestIO->WriteVar(Char3GrpName, Char3VarName, Char3VarShape, ExpectedChar3Data.get());

  // open the file we just created and see if it contains what we just wrote into it
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));

  std::size_t TestNlocs = TestIO->nlocs();
  std::size_t TestNrecs = TestIO->nrecs();
  std::size_t TestNvars = TestIO->nvars();

  BOOST_CHECK_EQUAL(TestNlocs, ExpectedNlocs);
  BOOST_CHECK_EQUAL(TestNrecs, ExpectedNrecs);
  BOOST_CHECK_EQUAL(TestNvars, ExpectedNvars);

  float Tolerance = conf.getFloat("TestInput.tolerance");
  std::vector<float> TestFloatData(ExpectedNlocs, 0.0);
  TestIO->ReadVar(FloatGrpName, FloatVarName, FloatVarShape, TestFloatData.data());
  for (std::size_t i = 0; i < ExpectedNlocs; i++) {
    BOOST_CHECK_CLOSE(TestFloatData[i], ExpectedFloatData[i], Tolerance);
  }

  std::vector<int> TestIntData(ExpectedNvars, 0);
  TestIO->ReadVar(IntGrpName, IntVarName, IntVarShape, TestIntData.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(TestIntData.begin(), TestIntData.end(),
                                ExpectedIntData.begin(), ExpectedIntData.end());

  std::unique_ptr<char[]> TestCharData(new char[ExpectedNrecs * 5]);
  TestIO->ReadVar(CharGrpName, CharVarName, CharVarShape, TestCharData.get());
  std::vector<std::string> TestStrings =
          CharArrayToStringVector(TestCharData.get(), CharVarShape);
  std::vector<std::string> ExpectedStrings =
          CharArrayToStringVector(ExpectedCharData.get(), CharVarShape);
  BOOST_CHECK_EQUAL_COLLECTIONS(TestStrings.begin(), TestStrings.end(),
                                ExpectedStrings.begin(), ExpectedStrings.end());

  std::unique_ptr<char[]> TestChar2Data(new char[ExpectedNvars * 5]);
  TestIO->ReadVar(Char2GrpName, Char2VarName, Char2VarShape, TestChar2Data.get());
  std::vector<std::string> TestStrings2 =
          CharArrayToStringVector(TestChar2Data.get(), Char2VarShape);
  std::vector<std::string> ExpectedStrings2 =
          CharArrayToStringVector(ExpectedChar2Data.get(), Char2VarShape);
  BOOST_CHECK_EQUAL_COLLECTIONS(TestStrings2.begin(), TestStrings2.end(),
                                ExpectedStrings2.begin(), ExpectedStrings2.end());

  std::unique_ptr<char[]> TestChar3Data(new char[ExpectedNrecs * 10]);
  TestIO->ReadVar(Char3GrpName, Char3VarName, Char3VarShape, TestChar3Data.get());
  std::vector<std::string> TestStrings3 =
          CharArrayToStringVector(TestChar3Data.get(), Char3VarShape);
  std::vector<std::string> ExpectedStrings3 =
          CharArrayToStringVector(ExpectedChar3Data.get(), Char3VarShape);
  BOOST_CHECK_EQUAL_COLLECTIONS(TestStrings3.begin(), TestStrings3.end(),
                                ExpectedStrings3.begin(), ExpectedStrings3.end());
}

// -----------------------------------------------------------------------------

void testReadDateTime() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::vector<eckit::LocalConfiguration> obstypes;

  std::string FileName;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;
  std::size_t VarSize;
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
    VarSize = TestIO->nlocs();
    TestVarDate.reset(new uint64_t[VarSize]);
    TestVarTime.reset(new int[VarSize]);
    TestIO->ReadDateTime(TestVarDate.get(), TestVarTime.get());

    // Compute the vector length TestVarData and compare with config values
    Dnorm = 0.0;
    Tnorm = 0.0;
    for(std::size_t k = 0; k < VarSize; ++k) {
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
    //ts->add(BOOST_TEST_CASE(&testReadDateTime));

    boost::unit_test::framework::master_test_suite().add(ts);
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
