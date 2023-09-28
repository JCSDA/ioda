/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file upgrade.cpp
* @brief A program to upgrade ioda files to a newer format.
* 
* Call program as: ioda-upgrade.x YAML_settings_file [input files] ... output_directory
*/

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "eckit/config/YAMLConfiguration.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/runtime/Main.h"
#include "../../../../../mains/validator/AttributeChecks.h"
#include "../../../../../mains/validator/Log.h"
#include "../../../../../mains/validator/Params.h"
#include "ioda/core/IodaUtils.h"
#include "ioda/Engines/EngineUtils.h"
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/StringFuncs.h"
#include "ioda/Variables/Fill.h"
#include "ioda/Variables/VarUtils.h"

// Annoying header junk on Windows. Who in their right mind defines a macro with
// the same name as a standard library function?
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using namespace ioda::VarUtils;

constexpr ioda::Dimensions_t defaultChunkSize = 100;

/// @brief Check and adjust the chunk sizes used in the VariableCreationParameters struct
/// @details A chunk size of zero is not acceptable, so in the case when a dimension is of
/// zero size, we still want to use a non-zero chunk size spec. This function will simply
/// check the chunk size specs in a VariableCreationParameter spec and change zero sizes
/// to the newChunkSize argument value.
/// @param[out] params The VariabelCreationParameters struct that is being checked/adjusted
/// @param[in] newChunkSize the value to be used for the adjusted chunk size
void checkAdjustChunkSizes(ioda::VariableCreationParameters & params,
                           const ioda::Dimensions_t & newChunkSize) {
  // A chunk size of zero is not acceptable. Also all of the dimensions scales and
  // and variables are set up to use chunking (which is necessary to use unlimited
  // max size) and potentially use the unlimited max size feature. So, if the chunks
  // parameter is set to zero, change it to the newChunkSize parameter.
  for (auto & i : params.chunks) {
    if (i == 0) {
      i = newChunkSize;
    }
  }
}

std::string renameDimension(const std::string &inName) {
  std::string outName = inName;
  if (inName == "nlocs") outName = "Location";
  if (inName == "nchans") outName = "Channel";
  return outName;
}

/// @brief Copy data from oldvar into newvar. Offsets are supported for variable combination. 
/// @param oldvar is the old variable.
/// @param newvar is the new variable.
/// @param base is the ObsGroup root object. Used in detecting ioda file versions.
/// @todo Add offset, oldvar_dims, newvar_dims.
void copyData(const Vec_Named_Variable& old, ioda::Variable& newvar, const ioda::ObsGroup& base,
              const std::string & newVarName, const std::map<int, int> & chanNumToIndex) {
  using namespace ioda;
  using namespace std;

  // DEBUG(ryan): Add the old variable names as a string attribute to ensure proper alignment.
  //vector<string> var_alignment;
  //for (const auto& v : old) var_alignment.push_back(v.name);
  //newvar.atts.add<string>("check_copy_vars", var_alignment);

  // Loop over each variable in old and apply to the appropriate place in newvar.
  for (size_t i = 0; i < old.size(); ++i) {
    const Variable oldvar   = old[i].var;

    Dimensions oldvar_dims  = oldvar.getDimensions();
    Dimensions newvar_dims  = newvar.getDimensions();
    size_t sz_type_in_bytes = oldvar.getType().getSize();
    if (oldvar.isA<std::string>()) {
      // Some old ioda files have really odd string formats. We detect these here and
      // repack the strings appropriately.
      vector<string> buf_in;
      oldvar.read<string>(buf_in);
      newvar.write<string>(buf_in);
    } else {
      vector<char> buf(oldvar_dims.numElements * sz_type_in_bytes);
      oldvar.read(gsl::make_span<char>(buf.data(), buf.size()), oldvar.getType());
      newvar.write(gsl::make_span<char>(buf.data(), buf.size()), newvar.getType());
    }
  }
}

/// @brief Copy attributes from src to dest. Ignore duplicates and dimension scales.
/// @param src is the source.
/// @param dest is the destination.
void copyAttributes(const ioda::Has_Attributes& src, ioda::Has_Attributes& dest) {
  using namespace ioda;
  using namespace std;
  vector<pair<string, Attribute>> srcAtts = src.openAll();

  for (const auto &s : srcAtts) {
    // This set contains the names of atttributes that need to be stripped off of
    // variables coming from the input file. The items in the list are related to
    // dimension scales and will confuse the netcdf API and tools if allowed to be
    // copied to the output file variables.
    //
    // In other words, these attributes assist the netcdf API in navigating the
    // association of variables with dimension scales and have meaning to the netcdf API. 
    // These represent the associations in the input file and need to be stripped off
    // since the associations in the output file will be re-created (and will not
    // necessarily match the associations in the input file).
    const set<string> ignored_names{
        "CLASS",
        "DIMENSION_LIST",
        "NAME",
        "REFERENCE_LIST",
        "_Netcdf4Coordinates",
        "_Netcdf4Dimid",
        "_nc3_strict"
        };
    if (ignored_names.count(s.first)) continue;
    if (dest.exists(s.first)) continue;

    Dimensions dims  = s.second.getDimensions();
    Type typ                = s.second.getType();
    size_t sz_type_in_bytes = typ.getSize();

    // Some variable attributes consist of an empty string in which case
    // numElements is zero. In this is the case, create an empty string in the
    // destination output, but make it consist of the null byte.
    if (dims.numElements == 0) {
      vector<char> buf(1, '\0');
      Attribute newatt = dest.create(s.first, typ, { 1 });
      newatt.write(gsl::make_span<char>(buf.data(), buf.size()), typ);
    } else {
      // copy from src attribute to dest attribute
      vector<char> buf(dims.numElements * sz_type_in_bytes);
      s.second.read(gsl::make_span<char>(buf.data(), buf.size()), typ);

      Attribute newatt = dest.create(s.first, typ, dims.dimsCur);
      newatt.write(gsl::make_span<char>(buf.data(), buf.size()), typ);
    }
  }
}

std::map<std::string, std::string> getOldNewNameMap(const std::string &yamlMappingName) {
  using namespace ioda_validate;
  ioda_validate::IODAvalidateParameters mappingParams_;
  eckit::PathName yamlfilename(yamlMappingName);
  eckit::YAMLConfiguration yaml(yamlfilename);
  mappingParams_.validateAndDeserialize(yaml);
  auto varParamsDefault = mappingParams_.vardefaults.value();
  auto vVarParams       = mappingParams_.variables.value();
  std::map<std::string, VariableParameters> mVarParams;
  std::map<std::string, std::string> mOldNewVarNames;
  for (const auto &v : vVarParams) {
    VariableParameters resultingVarParams = v;
    // Replace any of the missing parameters with parameters from the defaults.
    if (!v.base.atts.value()) resultingVarParams.base.atts = varParamsDefault.atts;
    if (!v.base.canBeMetadata.value())
      resultingVarParams.base.canBeMetadata = varParamsDefault.canBeMetadata;
    if (!v.base.dimNames.value()) resultingVarParams.base.dimNames = varParamsDefault.dimNames;
    if (!v.base.type.value()) resultingVarParams.base.type = varParamsDefault.type;

    // Variable name can be either a single string or a vector of strings.
    // Unfortunately, the Parameters parser is buggy here.
    // try {
    // Vector of strings case
    std::vector<std::string> names = v.varname.value().as<std::vector<std::string>>();
    for (const auto &name : names) {
      mVarParams[name] = resultingVarParams;
      if (name != names[0]) mOldNewVarNames[name] = names[0];
    }
  }
  return mOldNewVarNames;
}

void splitVarGroup(const std::string & vargrp, std::string & var, std::string & grp) {
  const size_t at = vargrp.find("@");
  const size_t slash = vargrp.find_last_of("/");

  if (at != std::string::npos) {
    // ioda v1 syntax
    var = vargrp.substr(0, at);
    grp = vargrp.substr(at + 1, std::string::npos);
    const size_t no_at = grp.find("@");
    ASSERT(no_at == std::string::npos);
  } else if (slash != std::string::npos) {
    // ioda v2 syntax
    grp = vargrp.substr(0, slash);
    var = vargrp.substr(slash + 1, std::string::npos);
  } else {
    // vargrp has no "@" nor "/" then assume that vargrp is a variable
    // name (with no group specified).
    var = vargrp;
    grp = "";
  }
}

std::string getNewNamingConventionsName(const std::string & inVarName,
                                        std::map<std::string, std::string> & lookupMap) {
  if (lookupMap.empty()) return inVarName;
  std::string var, group, outVarName;
  splitVarGroup(inVarName, var, group);
  if (group == "VarMetaData") group = "MetaData";
  if (lookupMap.find(var) == lookupMap.end()) {
    outVarName = group + "/" + var;
  } else {
    outVarName = group + "/" + lookupMap[var];
  }
  return outVarName;
}

bool upgradeFile(const std::string& inputName, const std::string& outputName,
                 std::map<std::string, std::string>& namingConventionsMap) {
  // Open file, determine dimension scales and variables.  
  using namespace ioda;
  using namespace std;
  const Group in = Engines::HH::openMemoryFile(inputName);

  Vec_Named_Variable varList, dimVarList;
  VarDimMap dimsAttachedToVars;
  Dimensions_t maxVarSize0;

  collectVarDimInfo(in, varList, dimVarList, dimsAttachedToVars, maxVarSize0);

  // Create the output file

  // TODO(ryan): Fix this odd workaround where the map searches fail oddly.
  map<string, Vec_Named_Variable> dimsAttachedToVars_bystring;
  for (const auto& val : dimsAttachedToVars)
    dimsAttachedToVars_bystring[convertV1PathToV2Path(val.first.name)] = val.second;

  // Construct the ObsGroup with the same scales as the input file.
  //
  // There are some cases where extraneous dimensions get included. An extraneous
  // dimension is one that is not attached to any variable in the file. Exclude defining
  // extraneous dimensions in the output file. To help with this, create a set
  // of dim names and use this to mark which dimensions are being used.
  set<string> attachedDims;
  for (const auto & ivar : dimsAttachedToVars_bystring) {
      for (const auto & idim : dimsAttachedToVars_bystring.at(ivar.first)) {
          attachedDims.insert(idim.name);
      }
  }

  std::vector<int> readChannelNumbers;
  NewDimensionScales_t newdims;
  for (const auto& dim : dimVarList) {
    // Read channel numbers tp be written out later
    if (readChannelNumbers.empty()) {
      if (dim.name == "Channel") dim.var.read<int>(readChannelNumbers);
      if (dim.name == "nchans") dim.var.read<int>(readChannelNumbers);
    }
    // Write out dimensions making sure that Location is a 32 bit int
    // and Channel is a 32 bit int.
    if (attachedDims.find(dim.name) != attachedDims.end()) {
      if ((dim.name == "Location") || (dim.name == "nlocs")) {
        auto location_size = dim.var.getDimensions().numElements;
        auto nds = NewDimensionScale<int32_t>("Location", gsl::narrow<Dimensions_t>(location_size),
                                              gsl::narrow<Dimensions_t>(location_size),
                                              gsl::narrow<Dimensions_t>(location_size));
        newdims.push_back(nds);
      } else if ((dim.name == "Channel") || (dim.name == "nchans")) {
        auto channel_size = dim.var.getDimensions().numElements;
        auto nds = NewDimensionScale<int32_t>("Channel", gsl::narrow<Dimensions_t>(channel_size),
                                              gsl::narrow<Dimensions_t>(channel_size),
                                              gsl::narrow<Dimensions_t>(channel_size));
        newdims.push_back(nds);
      } else {
        newdims.push_back(NewDimensionScale(renameDimension(dim.name), dim.var,
                          ScaleSizes{Unspecified, Unspecified, defaultChunkSize}));
      }
    }
  }

  Group g_out = Engines::HH::createFile(outputName,
      Engines::BackendCreateModes::Truncate_If_Exists,
      Engines::HH::HDF5_Version_Range{Engines::HH::HDF5_Version::V18,
      Engines::HH::HDF5_Version::V18});
  ObsGroup out = ObsGroup::generate(g_out, newdims);

  // Copy attributes from the root group
  copyAttributes(in.atts, out.atts);

  // Open all new scales
  map<string, Variable> newscales, newvars;
  for (const auto& dim : newdims) newscales[dim->name_] = out.vars[dim->name_];
  // Copy missing attributes from old scales.
  for (const Named_Variable& d : dimVarList) {
    if (attachedDims.find(d.name) != attachedDims.end())
        copyAttributes(d.var.atts, newscales.at(renameDimension(d.name)).atts);
  }

  // We want to convert older date time styles to the new epoch date time style.
  // This means that we want to skip over passing the older style variables to
  // the output file, but use them if necessary to create the epoch style.
  // There are three styles that can be encountered:
  //    offset - associated variable is MetaData/time which is an offset in hours
  //             from a reference given in the global attribute "date_time"
  //    string - associated variable is MetaData/datetime which is an absolute
  //             time represented in an ISO 8601 format string
  //    epoch -  associated variable is MetaData/dateTime which is an offset in seconds
  //             from a reference given in the variable's units attribute.
  //
  // If we encounter MetaData/dateTime in the input file, we want to copy it to the output
  // file.
  //
  // If we do not encounter MetaData/dateTime but do encounter MetaData/datetime, we want
  // to convert MetaData/datetime to an epoch style representation and copy it into
  // the variable MetaData/dateTime in the output file.
  //
  // If we do not encounter MetaData/dateTime nor MetaData/datetime but do encounter
  // MetaData/time, we want to convert MetaData/time to an epoch style representation and
  // copy it to the variable MetaData/dateTime in the output file.
  //
  // For all of the cases we want to skip writing MetaData/datetime and MetaData/time to
  // the output file if they exist in the input file.
  //
  // Determine which of the cases above we have in the input file
  bool useEpochDtime = false;
  bool useStringDtime = false;
  bool useOffsetDtime = false;
  // First record which variables exist in the input file, then apply precedence rules
  for (auto & oldVar : varList) {
      if (oldVar.name == "MetaData/dateTime") { useEpochDtime = true; }
      if (oldVar.name == "MetaData/datetime") { useStringDtime = true; }
      if (oldVar.name == "MetaData/time") { useOffsetDtime = true; }
  }
  if (useEpochDtime) {
      // epoch style takes precedence over the string and offset styles
      useStringDtime = false;
      useOffsetDtime = false;
  }
  if (useStringDtime) {
      // string style takes precendence over offset style
      useOffsetDtime = false;
  }

  // Make all variables and store handles. Do not attach dimension scales yet.
  // Loop is split for ungrouped vs grouped vars.
  auto makeNewVar = [&newvars,&out](const Named_Variable &oldVar, const Dimensions &dims,
                                    const VariableCreationParameters &params,
                                    const std::string & newVarName) {
    // TODO(ryan): turn on chunking and compression everywhere relevant.
    VariableCreationParameters adjustedParams = params;
    adjustedParams.chunk                      = true;
    {
      // Ideal chunking is a bit complicated.
      // Start with using all dimensions. If this is greater than 6400,
      // reduce the rightmost dimension. If rightmost dimension equals 1,
      // then target the second-to-last dimension, and so on.
      adjustedParams.chunks = dims.dimsCur;  // Initial suggestion.
      auto& c               = adjustedParams.chunks;
      const Dimensions_t max_chunk_size = 6400;
      while (accumulate(c.begin(), c.end(),
        static_cast<Dimensions_t>(1), multiplies<Dimensions_t>())> max_chunk_size)
      {
        auto dim = c.rbegin();
        while (*dim == 1) dim++;
        *dim /= 2;
      }

    }

    // make sure we are not specifying zero chunk sizes
    checkAdjustChunkSizes(adjustedParams, defaultChunkSize);

    adjustedParams.compressWithGZIP();

    // Make sure we are using the netcdf fill value (ie, _FillValue attribute) if it
    // is specified. This action will correct the issue when the netcdf and hdf5
    // fill values differ.
    forAnySupportedVariableType(
        oldVar.var,
        [&](auto typeDiscriminator) {
            typedef decltype(typeDiscriminator) T;
            ioda::detail::FillValueData_t oldVarFvData = oldVar.var.getFillValue();
            T oldVarFillValue = ioda::detail::getFillValue<T>(oldVarFvData);
            adjustedParams.setFillValue<T>(oldVarFillValue);
        },
        ThrowIfVariableIsOfUnsupportedType(oldVar.name));

    newvars[oldVar.name]
      = out.vars.create(newVarName, oldVar.var.getType(), dims, adjustedParams);
    return newvars[oldVar.name];
  };
  VarDimMap dimsForNewVars;
  // create vars to write out, including copy of their attributes
  for (const auto& oldVar : varList) {
    // skip over the old style date time variables if they exist
    if ((oldVar.name == "MetaData/datetime") || (oldVar.name == "MetaData/time")) { continue; }
    Dimensions dims = oldVar.var.getDimensions();
    // TODO(ryan): copy over other attributes?
    VariableCreationParameters params = oldVar.var.getCreationParameters(false, false);
    Variable newvar = makeNewVar(oldVar, dims, params,
                                 getNewNamingConventionsName(oldVar.name, namingConventionsMap));
    copyAttributes(oldVar.var.atts, newvar.atts);
    const Vec_Named_Variable old_attached_dims
      = dimsAttachedToVars_bystring.at(convertV1PathToV2Path(oldVar.name));
    dimsForNewVars[Named_Variable{oldVar.name, newvar}] = old_attached_dims;
  }

  // Attach all dimension scales to all variables.
  // We separate this from the variable creation (above) since we might want to implement a
  // collective call.
  {
    vector<pair<Variable, vector<Variable>>> out_dimsAttachedToVars;
    auto make_out_dimsAttachedToVars
      = [&newvars, &newscales, &out_dimsAttachedToVars](const Vec_Named_Variable& olddims,
                                                        const Named_Variable& m) {
          Variable newvar{newvars[m.name]};
          vector<Variable> newdims;
          for (const auto& d : olddims)
            newdims.emplace_back(newscales[renameDimension(d.name)]);
          // Check for an old-format string. If found, drop the last dimension.
          if (m.var.isA<string>()) {
            if (m.var.getType().getSize() == 1) {
              newdims.pop_back();
            }
          }
          out_dimsAttachedToVars.emplace_back(make_pair(newvar, newdims));
        };
    for (const auto& m : varList) {
      // skip over the old style date time variables if they exist
      if ((m.name == "MetaData/datetime") || (m.name == "MetaData/time")) { continue; }
      make_out_dimsAttachedToVars(dimsForNewVars.at(m), m);
    }
    out.vars.attachDimensionScales(out_dimsAttachedToVars);
  }

  // If we are using one of the old style date time variables, MetaData/dateTime was
  // not in the input file so we need to create it here.
  if (useStringDtime || useOffsetDtime) {
      // Determine the epoch
      std::string epochDtimeString;
      if (useStringDtime) {
        // Using string datetime, set the epoch to the linux standard epoch
        epochDtimeString = std::string("1970-01-01T00:00:00Z");
      } else {
        // Using offset datetime, set the epoch to the "date_time" global attribute
        int refDtimeInt;
        in.atts.open("date_time").read<int>(refDtimeInt);

        int year = refDtimeInt / 1000000;     // refDtimeInt contains YYYYMMDDhh
        int tempInt = refDtimeInt % 1000000;
        int month = tempInt / 10000;       // tempInt contains MMDDhh
        tempInt = tempInt % 10000;
        int day = tempInt / 100;           // tempInt contains DDhh
        int hour = tempInt % 100;
        util::DateTime refDtime(year, month, day, hour, 0, 0);

        epochDtimeString = refDtime.toString();
      }

      // create the MetaData/DateTime variable
      VariableCreationParameters params;
      std::vector<Variable> dimVars;
      dimVars.push_back(out.vars.open("Location"));
      util::DateTime epochDtime(epochDtimeString);
      util::DateTime fillValDtime("1900-01-01T00:00:00Z");
      params.setFillValue<int64_t>((fillValDtime - epochDtime).toSeconds());
      Variable destVar =
          out.vars.createWithScales<int64_t>("MetaData/dateTime", dimVars, params);

      destVar.atts.add<std::string>("units", std::string("seconds since ") + epochDtimeString);
  }
  
  cout << "\n Copying data:\n";

  // Copy over all data.
  // Do this for both variables and scales!
  for (const auto& oldvar : varList) {
    // skip over the old style date time variables if they exist
    if ((oldvar.name == "MetaData/datetime") || (oldvar.name == "MetaData/time")) { continue; }
    cout << "  " << oldvar.name << "\n";
    copyData(Vec_Named_Variable{oldvar}, newvars[oldvar.name], out, string(""), { });
  }

  // If we are using one of the older style date time formats (offset or string) we
  // need to convert their data to the epoch style and transfer it to the output.
  if (useStringDtime) {
      cout << "  MetaData/dateTime (converted from string representation in MetaData/datetime)\n";

      // Read in string datetimes and convert to time offsets.
      std::vector<std::string> dtStrings;
      Variable stringDtVar = in.vars.open("MetaData/datetime");
      stringDtVar.read<std::string>(dtStrings);

      Variable epochDtVar = out.vars.open("MetaData/dateTime");
      std::string epochString;
      epochDtVar.atts.open("units").read<std::string>(epochString);
      epochString = epochString.substr(14);   // strip off the leading "seconds since "
      util::DateTime epochDtime(epochString);

      std::vector<int64_t> timeOffsets =
          convertDtStringsToTimeOffsets(epochDtime, dtStrings);

      // Transfer the epoch datetime to the new variable.
      epochDtVar.write<int64_t>(timeOffsets);
  } else if (useOffsetDtime) {
      cout << "  MetaData/dateTime (converted from offset representation in MetaData/time)\n";

      // Use the date_time global attribute as the epoch. This means that
      // we just need to convert the float offset times in hours to an
      // int64_t offset in seconds.
      std::vector<float> dtTimeOffsets;
      Variable offsetDtVar = in.vars.open("MetaData/time");
      offsetDtVar.read<float>(dtTimeOffsets);

      std::vector<int64_t> timeOffsets(dtTimeOffsets.size());
      for (std::size_t i = 0; i < dtTimeOffsets.size(); ++i) {
          timeOffsets[i] = static_cast<int64_t>(lround(dtTimeOffsets[i] * 3600.0));
      }

      // Transfer the epoch datetime to the new variable.
      Variable epochDtVar = out.vars.open("MetaData/dateTime");
      epochDtVar.write<int64_t>(timeOffsets);
  }

  if (!readChannelNumbers.empty()) out.vars.open("Channel").write<int>(readChannelNumbers);

  return true;
}

class Upgrader : public eckit::Main {

public:
 virtual ~Upgrader() {}
 explicit Upgrader(int argc, char **argv) : eckit::Main(argc, argv) {}

 int execute() {
   using namespace std;
   try {
     // Program options
     auto doHelp = []() {
       cerr << "Usage: ioda-upgrade-v2-to-v3.x input_file output_file yaml_file\n";
       exit(1);
     };

     string sInputFile;
     string sOutputFile;
     string sYamlMappingFile;

     if (argc() == 4) {
       sInputFile = argv(1);
       sOutputFile = argv(2);
       sYamlMappingFile = argv(3);
     } else {
       doHelp();
     }

     cout << "Input: " << sInputFile << endl;
     cout << "Output: " << sOutputFile << endl;
     cout << "Yaml mapping path: " << sYamlMappingFile << endl;

     map<string, string> namingConventionsMap = getOldNewNameMap(sYamlMappingFile);

     upgradeFile(sInputFile, sOutputFile, namingConventionsMap);
     cout << " Success!\n";

   } catch (const std::exception& e) {
     cerr << "Exception: " << e.what() << endl << endl;
     return 1;
   } catch (...) {
     cerr << "An uncaught exception occurred." << endl << endl;
     return 1;
   }
   return 0;
}

};

int main(int argc, char **argv) {
  Upgrader run(argc, argv);
  return run.execute();
}
