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

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include <boost/noncopyable.hpp>

#include <boost/any.hpp>

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
  EXPECT(TestIO.get());

  // Constructor in read mode is also responsible for setting nobs and nlocs
  ExpectedNlocs = conf.getInt("TestInput.nlocs");
  ExpectedNrecs = conf.getInt("TestInput.nrecs");
  ExpectedNvars = conf.getInt("TestInput.nvars");

  Nlocs = TestIO->nlocs();
  Nrecs = TestIO->nrecs();
  Nvars = TestIO->nvars();

  EXPECT(ExpectedNlocs == Nlocs);
  EXPECT(ExpectedNrecs == Nrecs);
  EXPECT(ExpectedNvars == Nvars);

  // Constructor in write mode
  FileName = conf.getString("TestOutput.filename");

  ExpectedNlocs = conf.getInt("TestOutput.nlocs");
  ExpectedNrecs = conf.getInt("TestOutput.nrecs");
  ExpectedNvars = conf.getInt("TestOutput.nvars");

  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", ExpectedNlocs,
                                           ExpectedNrecs, ExpectedNvars));
  EXPECT(TestIO.get());

  Nlocs = TestIO->nlocs();
  Nrecs = TestIO->nrecs();
  Nvars = TestIO->nvars();

  EXPECT(ExpectedNlocs == Nlocs);
  EXPECT(ExpectedNrecs == Nrecs);
  EXPECT(ExpectedNvars == Nvars);
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
  EXPECT(TestIO.get());

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
  EXPECT(VarCount == ExpectedVarCount);
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
      for (std::size_t j = 0; j < TestVarData.size(); j++) {
        EXPECT(TestVarData[j] == ExpectedVarData[j]);
      }
    } else if ((VarType.compare("float") == 0) or (VarType.compare("double") == 0)) {
      std::vector<float> TestVarData(VarSize, 0.0);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData.data());
      std::vector<float> ExpectedVarData = conf.getFloatVector(ExpectedVarDataName);
      for (std::size_t j = 0; j < TestVarData.size(); j++) {
        EXPECT(oops::is_close(TestVarData[j], ExpectedVarData[j], Tolerance));
      }
    } else if (VarType.compare("char") == 0) {
      std::unique_ptr<char[]> TestVarData(new char[VarSize]);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData.get());
      std::vector<std::string> TestStrings =
                               CharArrayToStringVector(TestVarData.get(), VarShape);
      std::vector<std::string> ExpectedVarData = conf.getStringVector(ExpectedVarDataName);
      for (std::size_t j = 0; j < TestStrings.size(); j++) {
        EXPECT(TestStrings[j] == ExpectedVarData[j]);
      }
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
  std::unique_ptr<char[]> ExpectedChar3Data(new char[ExpectedNlocs * 20]);
  std::vector<std::string> Dates = {
    "2018-04-15T00:00:00Z", "2018-04-15T00:00:30Z", "2018-04-15T00:01:00Z",
    "2018-04-15T00:01:30Z", "2018-04-15T00:02:00Z", "2018-04-15T00:02:30Z",
    "2018-04-15T00:03:00Z", "2018-04-15T00:03:30Z" };
  TempString = Dates[0] + Dates[1] + Dates[2] + Dates[3] + Dates[4] + Dates[5] +
               Dates[6] + Dates[7];
  for (std::size_t i = 0; i < TempString.size(); i++) {
    ExpectedChar3Data.get()[i] = TempString[i];
  }
  std::vector<std::size_t> Char3VarShape{ ExpectedNlocs, 20 };
  std::string Char3GrpName = "MetaData";
  std::string Char3VarName = "datetime";
  TestIO->WriteVar(Char3GrpName, Char3VarName, Char3VarShape, ExpectedChar3Data.get());

  // open the file we just created and see if it contains what we just wrote into it
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r"));

  std::size_t TestNlocs = TestIO->nlocs();
  std::size_t TestNrecs = TestIO->nrecs();
  std::size_t TestNvars = TestIO->nvars();

  EXPECT(TestNlocs == ExpectedNlocs);
  EXPECT(TestNrecs == ExpectedNrecs);
  EXPECT(TestNvars == ExpectedNvars);

  float Tolerance = conf.getFloat("TestInput.tolerance");
  std::vector<float> TestFloatData(ExpectedNlocs, 0.0);
  TestIO->ReadVar(FloatGrpName, FloatVarName, FloatVarShape, TestFloatData.data());
  for (std::size_t i = 0; i < ExpectedNlocs; i++) {
    EXPECT(oops::is_close(TestFloatData[i], ExpectedFloatData[i], Tolerance));
  }

  std::vector<int> TestIntData(ExpectedNvars, 0);
  TestIO->ReadVar(IntGrpName, IntVarName, IntVarShape, TestIntData.data());
  for (std::size_t i = 0; i < TestIntData.size(); i++) {
    EXPECT(TestIntData[i] == ExpectedIntData[i]);
  }

  std::unique_ptr<char[]> TestCharData(new char[ExpectedNrecs * 5]);
  TestIO->ReadVar(CharGrpName, CharVarName, CharVarShape, TestCharData.get());
  std::vector<std::string> TestStrings =
          CharArrayToStringVector(TestCharData.get(), CharVarShape);
  std::vector<std::string> ExpectedStrings =
          CharArrayToStringVector(ExpectedCharData.get(), CharVarShape);
  for (std::size_t i = 0; i < TestStrings.size(); i++) {
    EXPECT(TestStrings[i] == ExpectedStrings[i]);
  }

  std::unique_ptr<char[]> TestChar2Data(new char[ExpectedNvars * 5]);
  TestIO->ReadVar(Char2GrpName, Char2VarName, Char2VarShape, TestChar2Data.get());
  std::vector<std::string> TestStrings2 =
          CharArrayToStringVector(TestChar2Data.get(), Char2VarShape);
  std::vector<std::string> ExpectedStrings2 =
          CharArrayToStringVector(ExpectedChar2Data.get(), Char2VarShape);
  for (std::size_t i = 0; i < TestStrings2.size(); i++) {
    EXPECT(TestStrings2[i] == ExpectedStrings2[i]);
  }

  std::unique_ptr<char[]> TestChar3Data(new char[ExpectedNlocs * 20]);
  TestIO->ReadVar(Char3GrpName, Char3VarName, Char3VarShape, TestChar3Data.get());
  std::vector<std::string> TestStrings3 =
          CharArrayToStringVector(TestChar3Data.get(), Char3VarShape);
  std::vector<std::string> ExpectedStrings3 =
          CharArrayToStringVector(ExpectedChar3Data.get(), Char3VarShape);
  for (std::size_t i = 0; i < TestStrings3.size(); i++) {
    EXPECT(TestStrings3[i] == ExpectedStrings3[i]);
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
    ts.emplace_back(CASE("fileio/IodaIO/testGrpVarIter")
      { testGrpVarIter(); });
    ts.emplace_back(CASE("fileio/IodaIO/testReadVar")
      { testReadVar(); });
    ts.emplace_back(CASE("fileio/IodaIO/testWriteVar")
      { testWriteVar(); });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
