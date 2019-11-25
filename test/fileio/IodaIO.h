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

#include <algorithm>
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

std::size_t GetMaxStringSize(const std::vector<std::string> & Strings) {
  std::size_t MaxSize = 0;
  for (std::size_t i = 0; i < Strings.size(); ++i) {
    if (Strings[i].size() > MaxSize) {
      MaxSize = Strings[i].size();
    }
  }
  return MaxSize;
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
    EXPECT(TestIO->frame_start(iframe) == FrameStarts[i]);
    EXPECT(TestIO->frame_size(iframe) == FrameSizes[i]);
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
      std::string VarGrpName =
          TestIO->frame_int_get_vname(idata) + "@" + TestIO->frame_int_get_gname(idata);
      std::vector<int> FrameData = TestIO->frame_int_get_data(idata);
      for (std::size_t i = 0; i < FrameData.size(); ++i) {
        IntVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // Float variables
    for (IodaIO::FrameFloatIter idata = TestIO->frame_float_begin();
                                idata != TestIO->frame_float_end(); ++idata) {
      std::string VarGrpName =
          TestIO->frame_float_get_vname(idata) + "@" + TestIO->frame_float_get_gname(idata);
      std::vector<float> FrameData = TestIO->frame_float_get_data(idata);
      for (std::size_t i = 0; i < FrameData.size(); ++i) {
        FloatVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // Double variables
    for (IodaIO::FrameDoubleIter idata = TestIO->frame_double_begin();
                                 idata != TestIO->frame_double_end(); ++idata) {
      std::string VarGrpName =
          TestIO->frame_double_get_vname(idata) + "@" + TestIO->frame_double_get_gname(idata);
      std::vector<double> FrameData = TestIO->frame_double_get_data(idata);
      for (std::size_t i = 0; i < FrameData.size(); ++i) {
        DoubleVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // String variables
    for (IodaIO::FrameStringIter idata = TestIO->frame_string_begin();
                                 idata != TestIO->frame_string_end(); ++idata) {
      std::string VarGrpName =
          TestIO->frame_string_get_vname(idata) + "@" + TestIO->frame_string_get_gname(idata);
      std::vector<std::string> FrameData = TestIO->frame_string_get_data(idata);
      for (std::size_t i = 0; i < FrameData.size(); ++i) {
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

  // Try writing variables specified in the config into a file, then read the file
  // check that you get the same data back.
  std::string FileName = conf.getString("TestOutput.filename");
  std::size_t MaxFrameSize = conf.getUnsigned("TestOutput.max_frame_size",
                                               IODAIO_DEFAULT_FRAME_SIZE);
  std::size_t ExpectedNlocs = conf.getInt("TestOutput.nlocs");
  std::size_t ExpectedNvars = conf.getInt("TestOutput.nvars");

  std::size_t MaxVarSize = 0;

  std::map<std::string, std::vector<int>>::iterator iint;
  std::map<std::string, std::vector<float>>::iterator ifloat;
  std::map<std::string, std::vector<std::string>>::iterator istring;

  std::map<std::string, std::vector<int>> IntVars;
  std::map<std::string, std::vector<float>> FloatVars;
  std::map<std::string, std::vector<std::string>> StringVars;

  std::map<std::string, std::vector<int>> ExpectedIntVars;
  std::map<std::string, std::vector<float>> ExpectedFloatVars;
  std::map<std::string, std::vector<std::string>> ExpectedStringVars;

  // Read in the variable data
  std::vector<eckit::LocalConfiguration> var_config = 
                        conf.getSubConfigurations("TestOutput.variables");

  for (std::size_t i = 0; i < var_config.size(); i++) {
    std::string VarGrpName = var_config[i].getString("name");
    std::string VarType = var_config[i].getString("type");

    if (VarType == "int") {
      ExpectedIntVars[VarGrpName] = var_config[i].getIntVector("values");
      IntVars[VarGrpName] = std::vector<int>(ExpectedIntVars[VarGrpName].size(),0);
      MaxVarSize = std::max(MaxVarSize, ExpectedIntVars[VarGrpName].size());
    } else if (VarType == "float") {
      ExpectedFloatVars[VarGrpName] = var_config[i].getFloatVector("values");
      FloatVars[VarGrpName] = std::vector<float>(ExpectedFloatVars[VarGrpName].size(),0.0);
      MaxVarSize = std::max(MaxVarSize, ExpectedFloatVars[VarGrpName].size());
    } else if (VarType == "string") {
      ExpectedStringVars[VarGrpName] = var_config[i].getStringVector("values");
      StringVars[VarGrpName] = std::vector<std::string>(ExpectedStringVars[VarGrpName].size(),"");
      MaxVarSize = std::max(MaxVarSize, ExpectedStringVars[VarGrpName].size());
    }
  }

  // Write the test data into the file. When writing, need to initialize the frame info
  // container, the dim info container and the group,variable info container.
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "W", MaxFrameSize));
  TestIO->frame_info_init(MaxVarSize);
  TestIO->dim_insert("nlocs", ExpectedNlocs); 
  TestIO->dim_insert("nvars", ExpectedNvars); 

  std::string GroupName;
  std::string VarName;
  for (iint = ExpectedIntVars.begin(); iint != ExpectedIntVars.end(); ++iint) {
    ExtractGrpVarName(iint->first, GroupName, VarName);
    std::vector<std::size_t> VarShape(1, (iint->second).size());
    TestIO->grp_var_insert(GroupName, VarName, "int", VarShape, iint->first, "int");
  }
  for (ifloat = ExpectedFloatVars.begin(); ifloat != ExpectedFloatVars.end(); ++ifloat) {
    ExtractGrpVarName(ifloat->first, GroupName, VarName);
    std::vector<std::size_t> VarShape(1, (ifloat->second).size());
    TestIO->grp_var_insert(GroupName, VarName, "float", VarShape, ifloat->first, "float");
  }
  for (istring = ExpectedStringVars.begin(); istring != ExpectedStringVars.end(); ++istring) {
    ExtractGrpVarName(istring->first, GroupName, VarName);
    std::vector<std::size_t> VarShape(1, (istring->second).size());
    std::size_t MaxStringSize = GetMaxStringSize(istring->second);
    TestIO->grp_var_insert(GroupName, VarName, "string", VarShape, istring->first, "string",
                           MaxStringSize);
  }

  for (IodaIO::FrameIter iframe = TestIO->frame_begin();
                         iframe != TestIO->frame_end(); ++iframe) {
    TestIO->frame_data_init();
    std::size_t FrameStart = TestIO->frame_start(iframe);
    std::size_t FrameSize = TestIO->frame_size(iframe);

    for (iint = ExpectedIntVars.begin(); iint != ExpectedIntVars.end(); ++iint) {
      ExtractGrpVarName(iint->first, GroupName, VarName);
      std::vector<std::size_t> VarShape = TestIO->var_shape(GroupName, VarName);

      if (VarShape[0] > FrameStart) {
        std::size_t VarSize;
        if (FrameStart + FrameSize > VarShape[0]) {
          VarSize = VarShape[0] - FrameStart;
        } else {
          VarSize = FrameSize;
        }

        std::vector<int>::iterator Start = ExpectedIntVars[iint->first].begin() + FrameStart;
        std::vector<int>::iterator End = Start + VarSize;
        std::vector<int> FrameData(Start, End);
        TestIO->frame_int_put_data(GroupName, VarName, FrameData);
      }
    }

    for (ifloat = ExpectedFloatVars.begin(); ifloat != ExpectedFloatVars.end(); ++ifloat) {
      ExtractGrpVarName(ifloat->first, GroupName, VarName);
      std::vector<std::size_t> VarShape = TestIO->var_shape(GroupName, VarName);

      if (VarShape[0] > FrameStart) {
        std::size_t VarSize;
        if (FrameStart + FrameSize > VarShape[0]) {
          VarSize = VarShape[0] - FrameStart;
        } else {
          VarSize = FrameSize;
        }

        std::vector<float>::iterator Start =
                               ExpectedFloatVars[ifloat->first].begin() + FrameStart;
        std::vector<float>::iterator End = Start + VarSize;
        std::vector<float> FrameData(Start, End);
        TestIO->frame_float_put_data(GroupName, VarName, FrameData);
      }
    }

    for (istring = ExpectedStringVars.begin(); istring != ExpectedStringVars.end(); ++istring) {
      ExtractGrpVarName(istring->first, GroupName, VarName);
      std::vector<std::size_t> VarShape = TestIO->var_shape(GroupName, VarName);

      if (VarShape[0] > FrameStart) {
        std::size_t VarSize;
        if (FrameStart + FrameSize > VarShape[0]) {
          VarSize = VarShape[0] - FrameStart;
        } else {
          VarSize = FrameSize;
        }

        std::vector<std::string>::iterator Start =
                               ExpectedStringVars[istring->first].begin() + FrameStart;
        std::vector<std::string>::iterator End = Start + VarSize;
        std::vector<std::string> FrameData(Start, End);
        TestIO->frame_string_put_data(GroupName, VarName, FrameData);
      }
    }

    // Write the frame into the file
    TestIO->frame_write(iframe);
  }

  // Read the data from the file we just created.
  TestIO.reset(ioda::IodaIOfactory::Create(FileName, "r", MaxFrameSize));
  for (IodaIO::FrameIter iframe = TestIO->frame_begin();
                         iframe != TestIO->frame_end(); ++iframe) {
    std::size_t FrameStart = TestIO->frame_start(iframe);
    std::size_t FrameSize = TestIO->frame_size(iframe);

    // Fill in the current frame from the file
    TestIO->frame_read(iframe);

    // Integer variables
    for (IodaIO::FrameIntIter idata = TestIO->frame_int_begin();
                              idata != TestIO->frame_int_end(); ++idata) {
      std::string GroupName = TestIO->frame_int_get_gname(idata);
      std::string VarName = TestIO->frame_int_get_vname(idata);
      std::vector<int> FrameData;
      TestIO->frame_int_get_data(GroupName, VarName, FrameData);

      std::string VarGrpName = VarName + "@" + GroupName;
      for (std::size_t i = 0; i < FrameData.size(); ++i) {
        IntVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // Float variables
    for (IodaIO::FrameFloatIter idata = TestIO->frame_float_begin();
                                idata != TestIO->frame_float_end(); ++idata) {
      std::string GroupName = TestIO->frame_float_get_gname(idata);
      std::string VarName = TestIO->frame_float_get_vname(idata);
      std::vector<float> FrameData;
      TestIO->frame_float_get_data(GroupName, VarName, FrameData);

      std::string VarGrpName = VarName + "@" + GroupName;
      for (std::size_t i = 0; i < FrameData.size(); ++i) {
        FloatVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }

    // String variables
    for (IodaIO::FrameStringIter idata = TestIO->frame_string_begin();
                                 idata != TestIO->frame_string_end(); ++idata) {
      std::string GroupName = TestIO->frame_string_get_gname(idata);
      std::string VarName = TestIO->frame_string_get_vname(idata);
      std::vector<std::string> FrameData;
      TestIO->frame_string_get_data(GroupName, VarName, FrameData);

      std::string VarGrpName = VarName + "@" + GroupName;
      for (std::size_t i = 0; i < FrameData.size(); ++i) {
        StringVars[VarGrpName][FrameStart + i] = FrameData[i];
      }
    }
  }

  // Check the variables read from the file against the expected values.
  for (iint = IntVars.begin(); iint != IntVars.end(); ++iint) {
    std::vector<int> IntVect = iint->second;
    std::vector<int> ExpectedIntVect = ExpectedIntVars[iint->first];
    for (std::size_t i = 0; i < IntVect.size(); i++) {
      EXPECT(IntVect[i] == ExpectedIntVect[i]);
    }
  }

  float FloatTol = conf.getFloat("TestOutput.tolerance");
  for (ifloat = FloatVars.begin(); ifloat != FloatVars.end(); ++ifloat) {
    std::vector<float> FloatVect = ifloat->second;
    std::vector<float> ExpectedFloatVect = ExpectedFloatVars[ifloat->first];
    for (std::size_t i = 0; i < FloatVect.size(); i++) {
      EXPECT(oops::is_close(FloatVect[i], ExpectedFloatVect[i], FloatTol));
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
