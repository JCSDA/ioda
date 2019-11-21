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
  MaxFrameSize = conf.getUnsigned("TestInput.frames.max_frame_size",
                                  IODAIO_DEFAULT_FRAME_SIZE);
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
  MaxFrameSize = conf.getUnsigned("TestInput.frames.max_frame_size",
                                  IODAIO_DEFAULT_FRAME_SIZE);
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

  // Test the frame info container.
  std::vector<std::size_t> FrameStarts = conf.getUnsignedVector("TestInput.frames.starts");
  std::vector<std::size_t> FrameSizes = conf.getUnsignedVector("TestInput.frames.sizes");
  std::size_t i = 0;
  for (IodaIO::FrameIter iframe = TestIO->frame_begin();
                         iframe != TestIO->frame_end(); ++iframe) {
    EXPECT(iframe->start == FrameStarts[i]);
    EXPECT(iframe->size == FrameSizes[i]);
    i++;
  } 
}

// -----------------------------------------------------------------------------

void testReadVar() {
  const eckit::LocalConfiguration conf(::test::TestEnvironment::config());

  std::vector<eckit::LocalConfiguration> obstypes;
  std::vector<std::string> GrpVarNames;

  std::unique_ptr<ioda::IodaIO> TestIO;

  // Get the input file name and the frame size.
  std::string FileName = conf.getString("TestInput.filename");
  std::size_t MaxFrameSize = conf.getUnsigned("TestInput.frames.max_frame_size",
                                              IODAIO_DEFAULT_FRAME_SIZE);
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", MaxFrameSize));

  // Get the number of locations
  std::size_t Nlocs = conf.getUnsigned("TestInput.nlocs");

  // Read in the set of test variables from the configuration into a map.
  // Create another map with the same variables to hold the data from the file.
  // Then compare the contents of the maps to complete the test.
  std::vector<eckit::LocalConfiguration> var_config = 
                        conf.getSubConfigurations("TestInput.variables");

  std::map<std::string, std::vector<int>> IntVars;
  std::map<std::string, std::vector<float>> FloatVars;
  std::map<std::string, std::vector<double>> DoubleVars;
  std::map<std::string, std::vector<std::string>> StringVars;

  std::map<std::string, std::vector<int>> ExpectedIntVars;
  std::map<std::string, std::vector<float>> ExpectedFloatVars;
  std::map<std::string, std::vector<double>> ExpectedDoubleVars;
  std::map<std::string, std::vector<std::string>> ExpectedStringVars;

  for (std::size_t i = 0; i < var_config.size(); i++) {
    std::string VarGrpName = var_config[i].getString("name");
    std::string VarType = var_config[i].getString("type");

    if (VarType == "int") {
      ExpectedIntVars[VarGrpName] = var_config[i].getIntVector("values");
      IntVars[VarGrpName] = std::vector<int>(ExpectedIntVars[VarGrpName].size(),0);
    } else if (VarType == "float") {
      ExpectedFloatVars[VarGrpName] = var_config[i].getFloatVector("values");
      FloatVars[VarGrpName] = std::vector<float>(ExpectedFloatVars[VarGrpName].size(),0.0);
    } else if (VarType == "double") {
      ExpectedDoubleVars[VarGrpName] = var_config[i].getDoubleVector("values");
      DoubleVars[VarGrpName] = std::vector<double>(ExpectedDoubleVars[VarGrpName].size(),0.0);
    } else if (VarType == "string") {
      ExpectedStringVars[VarGrpName] = var_config[i].getStringVector("values");
      StringVars[VarGrpName] = std::vector<std::string>(ExpectedStringVars[VarGrpName].size(),"");
    }
  }

  for (IodaIO::FrameIter iframe = TestIO->frame_begin();
                         iframe != TestIO->frame_end(); ++iframe) {
    std::size_t FrameStart = TestIO->frame_start(iframe);
    std::size_t FrameSize = TestIO->frame_size(iframe);

    // Fill in the current frame from the file
    TestIO->frame_read(iframe);

    // Integer variables
    for (IodaIO::FrameIntIter idata = TestIO->frame_int_begin();
                              idata != TestIO->frame_int_end(); ++idata) {
      std::string VarGrpName = TestIO->frame_int_get_name(idata);
      std::vector<int> FrameData = TestIO->frame_int_get_data(idata);
      for (std::size_t i = 0; i < FrameSize; ++i) {
        IntVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // Float variables
    for (IodaIO::FrameFloatIter idata = TestIO->frame_float_begin();
                                idata != TestIO->frame_float_end(); ++idata) {
      std::string VarGrpName = TestIO->frame_float_get_name(idata);
      std::vector<float> FrameData = TestIO->frame_float_get_data(idata);
      for (std::size_t i = 0; i < FrameSize; ++i) {
        FloatVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // Double variables
    for (IodaIO::FrameDoubleIter idata = TestIO->frame_double_begin();
                                 idata != TestIO->frame_double_end(); ++idata) {
      std::string VarGrpName = TestIO->frame_double_get_name(idata);
      std::vector<double> FrameData = TestIO->frame_double_get_data(idata);
      for (std::size_t i = 0; i < FrameSize; ++i) {
        DoubleVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // String variables
    for (IodaIO::FrameStringIter idata = TestIO->frame_string_begin();
                                 idata != TestIO->frame_string_end(); ++idata) {
      std::string VarGrpName = TestIO->frame_string_get_name(idata);
      std::vector<std::string> FrameData = TestIO->frame_string_get_data(idata);
      for (std::size_t i = 0; i < FrameSize; ++i) {
        StringVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }
  }

  // Check the variables read from the file against the expected values.
  std::map<std::string, std::vector<int>>::iterator iint;
  std::map<std::string, std::vector<float>>::iterator ifloat;
  std::map<std::string, std::vector<double>>::iterator idouble;
  std::map<std::string, std::vector<std::string>>::iterator istring;

  for (iint = IntVars.begin(); iint != IntVars.end(); ++iint) {
    std::vector<int> IntVect = iint->second;
    std::vector<int> ExpectedIntVect = ExpectedIntVars[iint->first];
    for (std::size_t i = 0; i < IntVect.size(); i++) {
      EXPECT(IntVect[i] == ExpectedIntVect[i]);
    }
  }

  float FloatTol = conf.getFloat("TestInput.tolerance");
  for (ifloat = FloatVars.begin(); ifloat != FloatVars.end(); ++ifloat) {
    std::vector<float> FloatVect = ifloat->second;
    std::vector<float> ExpectedFloatVect = ExpectedFloatVars[ifloat->first];
    for (std::size_t i = 0; i < FloatVect.size(); i++) {
      EXPECT(oops::is_close(FloatVect[i], ExpectedFloatVect[i], FloatTol));
    }
  }

  double DoubleTol = conf.getDouble("TestInput.tolerance");
  for (idouble = DoubleVars.begin(); idouble != DoubleVars.end(); ++idouble) {
    std::vector<double> DoubleVect = idouble->second;
    std::vector<double> ExpectedDoubleVect = ExpectedDoubleVars[idouble->first];
    for (std::size_t i = 0; i < DoubleVect.size(); i++) {
      EXPECT(oops::is_close(DoubleVect[i], ExpectedDoubleVect[i], DoubleTol));
    }
  }

  for (istring = StringVars.begin(); istring != StringVars.end(); ++istring) {
    std::vector<std::string> StringVect = istring->second;
    std::vector<std::string> ExpectedStringVect = ExpectedStringVars[istring->first];
    for (std::size_t i = 0; i < StringVect.size(); i++) {
      EXPECT(StringVect[i] == ExpectedStringVect[i]);
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
///   TestIO->WriteVar(FloatGrpName, FloatVarName, FloatVarShape, ExpectedFloatData);

  // Int data
  std::vector<int> ExpectedIntData(ExpectedNvars, 0);
  for (std::size_t i = 0; i < ExpectedNvars; ++i) {
    ExpectedIntData[i] = i * 2;
  }
  std::vector<std::size_t> IntVarShape(1, ExpectedNvars);
  std::string IntGrpName = "VarMetaData";
  std::string IntVarName = "test_int";
///   TestIO->WriteVar(IntGrpName, IntVarName, IntVarShape, ExpectedIntData);

  // Char data
  std::vector<std::string> ExpectedStrings(ExpectedNlocs, "");
  for (std::size_t i = 0; i < ExpectedNlocs; ++i) {
    ExpectedStrings[i] = std::string("Hello World: ") + std::to_string(i);
  }
  std::vector<std::size_t> StringVarShape(1, ExpectedNlocs);
  std::string StringGrpName = "MetaData";
  std::string StringVarName = "test_char";
///   TestIO->WriteVar(StringGrpName, StringVarName, StringVarShape, ExpectedStrings);

  // Try char data with same string size
  std::vector<std::string> ExpectedStrings2(ExpectedNvars, "");
  for (std::size_t i = 0; i < ExpectedNvars; ++i) {
    ExpectedStrings2[i] = std::string("ABCD: ") + std::to_string(i);
  }
  std::vector<std::size_t> String2VarShape(1, ExpectedNvars);
  std::string String2GrpName = "VarMetaData";
  std::string String2VarName = "test_char2";
///   TestIO->WriteVar(String2GrpName, String2VarName, String2VarShape, ExpectedStrings2);

  // Try char data with different shape
  std::vector<std::string> ExpectedStrings3(ExpectedNlocs, "");
  for (std::size_t i = 0; i < ExpectedNlocs; ++i) {
    ExpectedStrings3[i] =
         std::string("2018-04-15T00:00:0") + std::to_string(i) + std::string("Z");
  }
  std::vector<std::size_t> String3VarShape(1, ExpectedNlocs);
  std::string String3GrpName = "MetaData";
  std::string String3VarName = "datetime";
///   TestIO->WriteVar(String3GrpName, String3VarName, String3VarShape, ExpectedStrings3);

  // open the file we just created and see if it contains what we just wrote into it
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", MaxFrameSize));

  std::size_t TestNlocs = TestIO->nlocs();
  std::size_t TestNvars = TestIO->nvars();

  EXPECT(TestNlocs == ExpectedNlocs);
  EXPECT(TestNvars == ExpectedNvars);

  float Tolerance = conf.getFloat("TestInput.tolerance");
  std::vector<float> TestFloatData(ExpectedNlocs, 0.0);
///   TestIO->ReadVar(FloatGrpName, FloatVarName, FloatVarShape, TestFloatData);
  for (std::size_t i = 0; i < ExpectedNlocs; i++) {
    EXPECT(oops::is_close(TestFloatData[i], ExpectedFloatData[i], Tolerance));
  }

  std::vector<int> TestIntData(ExpectedNvars, 0);
///   TestIO->ReadVar(IntGrpName, IntVarName, IntVarShape, TestIntData);
  for (std::size_t i = 0; i < TestIntData.size(); i++) {
    EXPECT(TestIntData[i] == ExpectedIntData[i]);
  }

  std::vector<std::string> TestStrings(ExpectedNlocs);
///   TestIO->ReadVar(StringGrpName, StringVarName, StringVarShape, TestStrings);
  for (std::size_t i = 0; i < TestStrings.size(); i++) {
    EXPECT(TestStrings[i] == ExpectedStrings[i]);
  }

  std::vector<std::string> TestStrings2(ExpectedNvars);
///   TestIO->ReadVar(String2GrpName, String2VarName, String2VarShape, TestStrings2);
  for (std::size_t i = 0; i < TestStrings2.size(); i++) {
    EXPECT(TestStrings2[i] == ExpectedStrings2[i]);
  }

  std::vector<std::string> TestStrings3(ExpectedNlocs);
///   TestIO->ReadVar(String3GrpName, String3VarName, String3VarShape, TestStrings3);
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
///     ts.emplace_back(CASE("fileio/IodaIO/testWriteVar")
///       { testWriteVar(); });
  }
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_INTERFACE_OBSSPACE_H_
