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

void testConstructor() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::string FileName;
  std::size_t MaxFrameSize;
  std::unique_ptr<ioda::IodaIO> TestIO;

  std::size_t Nlocs;
  std::size_t Nvars;
  std::size_t ExpectedNlocs;
  std::size_t ExpectedNvars;

  // Contructor in read mode
  FileName = conf.getString("TestInput.filename");
  MaxFrameSize = conf.getUnsigned("TestInput.max_frame_size", IODAIO_DEFAULT_FRAME_SIZE);
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", MaxFrameSize));
  EXPECT(TestIO.get());

  // Constructor in read mode is also responsible for setting nobs and nlocs
  ExpectedNlocs = conf.getInt("TestInput.nlocs");
  ExpectedNvars = conf.getInt("TestInput.nvars");

  Nlocs = TestIO->nlocs();
  Nvars = TestIO->nvars();

  EXPECT(ExpectedNlocs == Nlocs);
  EXPECT(ExpectedNvars == Nvars);

  // Constructor in write mode
  FileName = conf.getString("TestOutput.filename");
  MaxFrameSize = conf.getUnsigned("TestOutput.max_frame_size", IODAIO_DEFAULT_FRAME_SIZE);

  ExpectedNlocs = conf.getInt("TestOutput.nlocs");
  ExpectedNvars = conf.getInt("TestOutput.nvars");

  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", MaxFrameSize));
  EXPECT(TestIO.get());
  }

// -----------------------------------------------------------------------------

void testContainers() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
  std::vector<eckit::LocalConfiguration> obstypes;

  std::string FileName;
  std::size_t MaxFrameSize;
  std::unique_ptr<ioda::IodaIO> TestIO;

  // Constructor in read mode will generate a group variable container, 
  // a dimension container and a frame container.
  FileName = conf.getString("TestInput.filename");
  MaxFrameSize = conf.getUnsigned("TestInput.max_frame_size", IODAIO_DEFAULT_FRAME_SIZE);
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", MaxFrameSize));
  EXPECT(TestIO.get());

  // Test the group, variable iterators by walking through the entire list of variables
  // and check the count of variables (total number in the file) with the
  // expected count.
  std::size_t VarCount = 0;
  std::size_t ExpectedVarCount = conf.getInt("TestInput.nvars");
  for (IodaIO::GroupIter igrp = TestIO->group_begin(); igrp != TestIO->group_end(); ++igrp) {
    std::string GroupName = TestIO->group_name(igrp);

    for (IodaIO::VarIter ivar = TestIO->var_begin(igrp); ivar != TestIO->var_end(igrp); ++ivar) {
      VarCount++;
    }
  }
  EXPECT(VarCount == ExpectedVarCount);

  // Test the dimension container. Contains dimension name, id, size.
  std::vector<std::string> DimNames;
  std::vector<std::size_t> DimIds;
  std::vector<std::size_t> DimSizes;
  std::vector<std::string> ExpectedDimNames = conf.getStringVector("TestInput.dimensions.names");
  std::vector<std::size_t> ExpectedDimIds = conf.getUnsignedVector("TestInput.dimensions.ids");
  std::vector<std::size_t> ExpectedDimSizes = conf.getUnsignedVector("TestInput.dimensions.sizes");
  for (IodaIO::DimIter idim = TestIO->dim_begin(); idim != TestIO->dim_end(); ++idim) {
    DimNames.push_back(TestIO->dim_name(idim));
    DimIds.push_back(TestIO->dim_id(idim));
    DimSizes.push_back(TestIO->dim_size(idim));
  }
  for (std::size_t i = 0; i < DimNames.size(); i++) {
    EXPECT(DimNames[i] == ExpectedDimNames[i]);
    EXPECT(DimIds[i] == ExpectedDimIds[i]);
    EXPECT(DimSizes[i] == ExpectedDimSizes[i]);
  }
}

// -----------------------------------------------------------------------------

void testReadVar() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::vector<eckit::LocalConfiguration> obstypes;
  std::vector<std::string> GrpVarNames;

  std::string FileName;
  std::size_t MaxFrameSize;
  std::string TestObsType;
  std::unique_ptr<ioda::IodaIO> TestIO;

  FileName = conf.getString("TestInput.filename");
  MaxFrameSize = conf.getUnsigned("TestInput.max_frame_size", IODAIO_DEFAULT_FRAME_SIZE);
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", MaxFrameSize));

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
    if (VarType == "int") {
      std::vector<int> TestVarData(VarSize, 0);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData);
      std::vector<int> ExpectedVarData = conf.getIntVector(ExpectedVarDataName);
      for (std::size_t j = 0; j < TestVarData.size(); j++) {
        EXPECT(TestVarData[j] == ExpectedVarData[j]);
      }
    } else if (VarType == "float") {
      std::vector<float> TestVarData(VarSize, 0.0);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData);
      std::vector<float> ExpectedVarData = conf.getFloatVector(ExpectedVarDataName);
      for (std::size_t j = 0; j < TestVarData.size(); j++) {
        EXPECT(oops::is_close<float>(TestVarData[j], ExpectedVarData[j], Tolerance));
      }
    } else if (VarType == "double") {
      std::vector<double> TestVarData(VarSize, 0.0);
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData);
      std::vector<double> ExpectedVarData = conf.getDoubleVector(ExpectedVarDataName);
      for (std::size_t j = 0; j < TestVarData.size(); j++) {
        EXPECT(oops::is_close<double>(TestVarData[j], ExpectedVarData[j], Tolerance));
      }
    } else if (VarType == "string") {
      std::vector<std::string> TestVarData(VarSize, "");
      TestIO->ReadVar(GroupName, VarName, VarShape, TestVarData);
      std::vector<std::string> ExpectedVarData = conf.getStringVector(ExpectedVarDataName);
      for (std::size_t j = 0; j < TestVarData.size(); j++) {
        EXPECT(TestVarData[j] == ExpectedVarData[j]);
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
  std::size_t MaxFrameSize = conf.getUnsigned("TestOutput.max_frame_size",
                                               IODAIO_DEFAULT_FRAME_SIZE);
  std::size_t ExpectedNlocs = conf.getInt("TestOutput.nlocs");
  std::size_t ExpectedNvars = conf.getInt("TestOutput.nvars");
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", MaxFrameSize));

  // Float data
  std::vector<float> ExpectedFloatData(ExpectedNlocs, 0.0);
  for (std::size_t i = 0; i < ExpectedNlocs; ++i) {
    ExpectedFloatData[i] = float(i) + 0.5;
  }
  std::vector<std::size_t> FloatVarShape(1, ExpectedNlocs);
  std::string FloatGrpName = "MetaData";
  std::string FloatVarName = "test_float";
  TestIO->WriteVar(FloatGrpName, FloatVarName, FloatVarShape, ExpectedFloatData);

  // Int data
  std::vector<int> ExpectedIntData(ExpectedNvars, 0);
  for (std::size_t i = 0; i < ExpectedNvars; ++i) {
    ExpectedIntData[i] = i * 2;
  }
  std::vector<std::size_t> IntVarShape(1, ExpectedNvars);
  std::string IntGrpName = "VarMetaData";
  std::string IntVarName = "test_int";
  TestIO->WriteVar(IntGrpName, IntVarName, IntVarShape, ExpectedIntData);

  // Char data
  std::vector<std::string> ExpectedStrings(ExpectedNlocs, "");
  for (std::size_t i = 0; i < ExpectedNlocs; ++i) {
    ExpectedStrings[i] = std::string("Hello World: ") + std::to_string(i);
  }
  std::vector<std::size_t> StringVarShape(1, ExpectedNlocs);
  std::string StringGrpName = "MetaData";
  std::string StringVarName = "test_char";
  TestIO->WriteVar(StringGrpName, StringVarName, StringVarShape, ExpectedStrings);

  // Try char data with same string size
  std::vector<std::string> ExpectedStrings2(ExpectedNvars, "");
  for (std::size_t i = 0; i < ExpectedNvars; ++i) {
    ExpectedStrings2[i] = std::string("ABCD: ") + std::to_string(i);
  }
  std::vector<std::size_t> String2VarShape(1, ExpectedNvars);
  std::string String2GrpName = "VarMetaData";
  std::string String2VarName = "test_char2";
  TestIO->WriteVar(String2GrpName, String2VarName, String2VarShape, ExpectedStrings2);

  // Try char data with different shape
  std::vector<std::string> ExpectedStrings3(ExpectedNlocs, "");
  for (std::size_t i = 0; i < ExpectedNlocs; ++i) {
    ExpectedStrings3[i] =
         std::string("2018-04-15T00:00:0") + std::to_string(i) + std::string("Z");
  }
  std::vector<std::size_t> String3VarShape(1, ExpectedNlocs);
  std::string String3GrpName = "MetaData";
  std::string String3VarName = "datetime";
  TestIO->WriteVar(String3GrpName, String3VarName, String3VarShape, ExpectedStrings3);

  // open the file we just created and see if it contains what we just wrote into it
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", MaxFrameSize));

  std::size_t TestNlocs = TestIO->nlocs();
  std::size_t TestNvars = TestIO->nvars();

  EXPECT(TestNlocs == ExpectedNlocs);
  EXPECT(TestNvars == ExpectedNvars);

  float Tolerance = conf.getFloat("TestInput.tolerance");
  std::vector<float> TestFloatData(ExpectedNlocs, 0.0);
  TestIO->ReadVar(FloatGrpName, FloatVarName, FloatVarShape, TestFloatData);
  for (std::size_t i = 0; i < ExpectedNlocs; i++) {
    EXPECT(oops::is_close(TestFloatData[i], ExpectedFloatData[i], Tolerance));
  }

  std::vector<int> TestIntData(ExpectedNvars, 0);
  TestIO->ReadVar(IntGrpName, IntVarName, IntVarShape, TestIntData);
  for (std::size_t i = 0; i < TestIntData.size(); i++) {
    EXPECT(TestIntData[i] == ExpectedIntData[i]);
  }

  std::vector<std::string> TestStrings(ExpectedNlocs);
  TestIO->ReadVar(StringGrpName, StringVarName, StringVarShape, TestStrings);
  for (std::size_t i = 0; i < TestStrings.size(); i++) {
    EXPECT(TestStrings[i] == ExpectedStrings[i]);
  }

  std::vector<std::string> TestStrings2(ExpectedNvars);
  TestIO->ReadVar(String2GrpName, String2VarName, String2VarShape, TestStrings2);
  for (std::size_t i = 0; i < TestStrings2.size(); i++) {
    EXPECT(TestStrings2[i] == ExpectedStrings2[i]);
  }

  std::vector<std::string> TestStrings3(ExpectedNlocs);
  TestIO->ReadVar(String3GrpName, String3VarName, String3VarShape, TestStrings3);
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
    ts.emplace_back(CASE("fileio/IodaIO/testContainers")
      { testContainers(); });
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
