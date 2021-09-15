/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Engines/Factory.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"


int main(int argc, char** argv) {
  std::cout << "You have entered " << argc
       << " arguments:" << "\n";
  for (int i = 0; i < argc; ++i)
      std::cout << argv[i] << "\n";

  std::string filepath;
  if (argc == 1) {
    filepath = argv[0];
  } else if (argc == 2) {
    filepath = argv[1];
  } else {
    throw;
  }

  // Create backend using an hdf5 file for reading.
  ioda::Engines::BackendNames backendName;
  backendName = ioda::Engines::BackendNames::Hdf5File;
  ioda::Engines::BackendCreationParameters backendParams;
  backendParams.fileName = filepath;
  backendParams.action = ioda::Engines::BackendFileActions::Open;
  backendParams.openMode = ioda::Engines::BackendOpenModes::Read_Only;
  ioda::Group group = ioda::Engines::constructBackend(backendName, backendParams);

  // Test reading the coordinates attribute of this variable.
  ioda::Variable oberrVar = group.vars["air_temperature@ObsError"];
  std::string coord_names;
  oberrVar.atts["coordinates"].read(coord_names);

  if (coord_names != "observation_type@MetaData index") {
    throw;
  }

  // Test combinations (4 total) of memory and attribute being fixed length or variable
  // length strings.
  //
  // For now, we have to fake it a bit with the fixed length memory side, since there
  // isn't quite full support yet in ioda to automatically get the memory side typed
  // correctly. In this test we know what the sizes of the expected fixed length strings
  // are and we can build the correct data type for the read commands. The expected values
  // from the file attributes are:
  //
  //     Attribute Name             Value                Length
  //   fixlen_string_attr    "fixed length string"         19
  //   varlen_string_attr    "variable length string"      22
  ioda::Variable var = group.vars["air_pressure@MetaData"];
  std::string memVlenString;

  // memory is variable length string, attribute is variable length string
  var.atts["varlen_string_attr"].read(memVlenString);
  if (memVlenString != "variable length string") {
    throw;
  }
  // memory is variable length string, attribute is fixed length string
  var.atts["fixlen_string_attr"].read(memVlenString);
  if (memVlenString != "fixed length string") {
    throw;
  }

  // memory is fixed length string, attribute is variable length string
  // expected value is 22 characters long
  ioda::Type typeFlenString_22 =
      var.atts.getTypeProvider()->makeStringType(22, typeid(std::string));
  std::vector<char> readBuffer_22(22);
  var.atts["varlen_string_attr"].read(gsl::make_span(readBuffer_22), typeFlenString_22);
  std::string memFlenString(readBuffer_22.data(), 22);
  if (memFlenString != "variable length string") {
    throw;
  }
  // memory is fixed length string, attribute is fixed length string
  // expected value is 19 characters long
  ioda::Type typeFlenString_19 =
      var.atts.getTypeProvider()->makeStringType(19, typeid(std::string));
  std::vector<char> readBuffer_19(19);
  var.atts["fixlen_string_attr"].read(gsl::make_span(readBuffer_19), typeFlenString_19);
  memFlenString = std::string(readBuffer_19.data(), 19);
  if (memFlenString != "fixed length string") {
    throw;
  }

  return 0;
}
