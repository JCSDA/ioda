/*
 * (C) Copyright 2018-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <iomanip>
#include <set>

#include "ioda/core/IodaUtils.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/VarUtils.h"

#include "oops/util/DateTime.h"
#include "oops/util/missingValues.h"

namespace ioda {

// -----------------------------------------------------------------------------

std::vector<std::size_t> CharShapeFromStringVector(
                                  const std::vector<std::string> & StringVector) {
  std::size_t MaxStrLen = 0;
  for (std::size_t i = 0; i < StringVector.size(); i++) {
    std::size_t StrSize = StringVector[i].size();
    if (StrSize > MaxStrLen) {
      MaxStrLen = StrSize;
    }
  }

  std::vector<std::size_t> Shape{ StringVector.size(), MaxStrLen };
  return Shape;
}

// -----------------------------------------------------------------------------

std::vector<std::string> CharArrayToStringVector(const char * CharData,
                                            const std::vector<std::size_t> & CharShape) {
  // CharShape[0] is the number of strings
  // CharShape[1] is the length of each string
  std::size_t Nstrings = CharShape[0];
  std::size_t StrLength = CharShape[1];

  std::vector<std::string> StringVector(Nstrings, "");
  for (std::size_t i = 0; i < Nstrings; i++) {
    // Copy characters for i-th string into a char vector
    std::vector<char> CharVector(StrLength, ' ');
    for (std::size_t j = 0; j < StrLength; j++) {
      CharVector[j] = CharData[(i*StrLength) + j];
    }

    // Convert the char vector to a single string. Any trailing white space will be
    // included in the string, so strip off the trailing white space.
    //
    // In order to include null characters in the white space list, the (char *, size_t)
    // form of the string constructor needs to be used. The size_t (2nd) argument says
    // how many characters to use from the "buffer" (1st argument). If the (char *) form
    // of the string constructor is use, the null character terminates the string and only
    // those characters leading up to the null are used.
    std::string WhiteSpace(" \t\n\r\f\v\0", 7);
    std::string String(CharVector.begin(), CharVector.end());
    String.erase(String.find_last_not_of(WhiteSpace) + 1, std::string::npos);
    StringVector[i] = String;
  }

  return StringVector;
}

// -----------------------------------------------------------------------------

void StringVectorToCharArray(const std::vector<std::string> & StringVector,
                             const std::vector<std::size_t> & CharShape, char * CharData) {
  // CharShape[0] is the number of strings, and CharShape[1] is the maximum
  // string lenghth. Walk through the string vector, copy the string and fill
  // with white space at the ends of strings if necessary.
  for (std::size_t i = 0; i < CharShape[0]; i++) {
    for (std::size_t j = 0; j < CharShape[1]; j++) {
      std::size_t ichar = (i * CharShape[1]) + j;
      if (j < StringVector[i].size()) {
        CharData[ichar] = StringVector[i].data()[j];
      } else {
        CharData[ichar] = ' ';
      }
    }
  }
}

// -----------------------------------------------------------------------------

std::string TypeIdName(const std::type_info & TypeId) {
  std::string TypeName;
  if (TypeId == typeid(int)) {
    TypeName = "integer";
  } else if (TypeId == typeid(float)) {
    TypeName = "float";
  } else if (TypeId == typeid(double)) {
    TypeName = "double";
  } else if (TypeId == typeid(std::string)) {
    TypeName = "string";
  } else if (TypeId == typeid(util::DateTime)) {
    TypeName = "DateTime";
  } else {
    TypeName = TypeId.name();
  }

  return TypeName;
}

// -----------------------------------------------------------------------------
std::size_t FindMaxStringLength(const std::vector<std::string> & StringVector) {
  std::size_t MaxStringLength = 0;
  for (std::size_t i = 0; i < StringVector.size(); ++i) {
    if (StringVector[i].size() > MaxStringLength) {
      MaxStringLength = StringVector[i].size();
    }
  }
  return MaxStringLength;
}

// -----------------------------------------------------------------------------
std::string fullVarName(const std::string & groupName, const std::string & varName) {
    return groupName + std::string("/") + varName;
}

//------------------------------------------------------------------------------------
std::type_index varDtype(const Group & group, const std::string & varName) {
    Variable var = group.vars.open(varName);
    std::type_index varType(typeid(std::string));
    VarUtils::forAnySupportedVariableType(
          var,
          [&](auto typeDiscriminator) {
              varType = typeid(typeDiscriminator);
          },
          VarUtils::ThrowIfVariableIsOfUnsupportedType(varName));
    return varType;
}

//------------------------------------------------------------------------------------
bool varIsDimScale(const Group & group, const std::string & varName) {
    Variable var = group.vars.open(varName);
    return var.isDimensionScale();
}

//------------------------------------------------------------------------------------
util::DateTime getEpochAsDtime(const Variable & dtVar) {
  // get the units attribute and strip off the "seconds since " part. For now,
  // we are restricting the units to "seconds since " and will be expanding that
  // in the future to other time units (hours, days, minutes, etc).
  std::string epochString = dtVar.atts.open("units").read<std::string>();
  std::size_t pos = epochString.find("seconds since ");
  if (pos == std::string::npos) {
    std::string errorMsg =
        std::string("For now, only supporting 'seconds since' form of ") +
        std::string("units for MetaData/dateTime variable");
    Exception(errorMsg.c_str(), ioda_Here());
  }
  epochString.replace(pos, pos+14, "");

  return util::DateTime(epochString);
}

//------------------------------------------------------------------------------------
void openCreateEpochDtimeVar(const std::string & groupName, const std::string & varName,
                             const util::DateTime & newEpoch, Variable & epochDtVar,
                             Has_Variables & destVarContainer) {
  std::string fullVarName = groupName + "/" + varName;
  if (destVarContainer.exists(fullVarName)) {
    // Variable already exists, simply open it.
    epochDtVar = destVarContainer.open(fullVarName);
  } else {
    // Variable does not exist, need to create it. Use the newEpoch for the units attribute.
    std::vector<Variable> dimVars(1, destVarContainer.open("Location"));
    VariableCreationParameters params = VariableCreationParameters::defaults<int64_t>();
    // Don't want compression in the memory image.
    params.noCompress();
    // TODO(srh) For now use a default chunk size for the chunk size in the creation
    // parameters when the first dimension is Location. This is being done since the
    // size of Location can vary across MPI tasks, and we need it to be constant for
    // the parallel io to work properly. The assigned chunk size may need to be optimized
    // further than using the rough guess declared in VarUtils::DefaultChunkSize.
    std::vector<ioda::Dimensions_t> chunkDims(1, VarUtils::DefaultChunkSize);
    params.setChunks(chunkDims);
    epochDtVar = destVarContainer.createWithScales<int64_t>(fullVarName, dimVars, params);
    std::string epochString = "seconds since " + newEpoch.toString();
    epochDtVar.atts.add("units", epochString);
  }
}

//------------------------------------------------------------------------------------
std::vector<util::DateTime> convertDtStringsToDtime(const std::vector<std::string> & dtStrings) {
    // Convert ISO 8601 strings directly to DateTime objects
    std::size_t dtimeSize = dtStrings.size();
    std::vector<util::DateTime> dateTimeValues(dtimeSize);
    for (std::size_t i = 0; i < dtimeSize; ++i) {
        util::DateTime dateTime(dtStrings[i]);
        dateTimeValues[i] = dateTime;
    }
    return dateTimeValues;
}

//------------------------------------------------------------------------------------
std::vector<util::DateTime> convertEpochDtToDtime(const util::DateTime epochDtime,
                                                  const std::vector<int64_t> & timeOffsets) {
  const util::DateTime missingDateTime = util::missingValue(missingDateTime);
  const int64_t missingInt64 = util::missingValue(missingInt64);
  std::vector<util::DateTime> dateTimes;
  dateTimes.reserve(timeOffsets.size());
  for (std::size_t i = 0; i < timeOffsets.size(); ++i) {
    if (timeOffsets[i] == missingInt64) {
      dateTimes.emplace_back(missingDateTime);
    } else {
      const util::Duration timeDiff(timeOffsets[i]);
      dateTimes.emplace_back(epochDtime + timeDiff);
    }
  }
  return dateTimes;
}

//------------------------------------------------------------------------------------
std::vector<int64_t> convertDtimeToTimeOffsets(const util::DateTime epochDtime,
                                               const std::vector<util::DateTime> & dtimes) {
  const util::DateTime missingDateTime = util::missingValue(missingDateTime);
  const int64_t missingInt64 = util::missingValue(missingInt64);
  std::vector<int64_t> timeOffsets(dtimes.size());
  for (std::size_t i = 0; i < dtimes.size(); ++i) {
    if (dtimes[i] == missingDateTime) {
      timeOffsets[i] = missingInt64;
    } else {
      const util::Duration timeDiff = dtimes[i] - epochDtime;
      timeOffsets[i] = timeDiff.toSeconds();
    }
  }
  return timeOffsets;
}

//------------------------------------------------------------------------------------
std::vector<int64_t> convertDtStringsToTimeOffsets(const util::DateTime epochDtime,
                                                   const std::vector<std::string> & dtStrings) {
  std::vector<int64_t> timeOffsets(dtStrings.size());
  for (std::size_t i = 0; i < dtStrings.size(); ++i) {
    util::DateTime dtime(dtStrings[i]);
    util::Duration timeDiff = dtime - epochDtime;
    timeOffsets[i] = timeDiff.toSeconds();
  }
  return timeOffsets;
}

//------------------------------------------------------------------------------------
std::vector<std::string> StringArrayToStringVector(
                                               const std::vector<std::string> & arrayData,
                                               const std::vector<Dimensions_t> & arrayShape) {
    // arrayShape[0] is the number of strings
    // arrayShape[1] is the length of each string
    std::size_t nstrings = arrayShape[0];
    std::size_t strLength = arrayShape[1];

    //
    std::vector<std::string> stringVector(nstrings, "");
    for (std::size_t i = 0; i < nstrings; i++) {
        std::string oneString = "";
        for (std::size_t j = 0; j < strLength; j++) {
            oneString += arrayData[(i*strLength) + j];
        }

        // Strip off trainling whitespace.
        //
        // In order to include null characters in the white space list, the (char *, size_t)
        // form of the string constructor needs to be used. The size_t (2nd) argument says
        // how many characters to use from the "buffer" (1st argument). If the (char *) form
        // of the string constructor is use, the null character terminates the string and only
        // those characters leading up to the null are used.
        std::string WhiteSpace(" \t\n\r\f\v\0", 7);
        oneString.erase(oneString.find_last_not_of(WhiteSpace) + 1, std::string::npos);
        stringVector[i] = oneString;
    }

  return stringVector;
}

// -----------------------------------------------------------------------------
void setOfileParamsFromTestConfig(const eckit::LocalConfiguration & obsConfig,
                                  ioda::ObsSpaceParameters & obsParams) {
    // Get dimensions and variables sub configurations
    std::vector<eckit::LocalConfiguration> writeDimConfigs =
        obsConfig.getSubConfigurations("write dimensions");
    std::vector<eckit::LocalConfiguration> writeVarConfigs =
        obsConfig.getSubConfigurations("write variables");

    // Add the dimensions scales to the ObsIo parameters
    std::map<std::string, Dimensions_t> dimSizes;
    for (std::size_t i = 0; i < writeDimConfigs.size(); ++i) {
        std::string dimName = writeDimConfigs[i].getString("name");
        Dimensions_t dimSize = writeDimConfigs[i].getInt("size");
        bool isUnlimited = writeDimConfigs[i].getBool("unlimited", false);

        if (isUnlimited) {
            obsParams.setDimScale(dimName, dimSize, Unlimited, dimSize);
        } else {
            obsParams.setDimScale(dimName, dimSize, dimSize, dimSize);
        }
        dimSizes.insert(std::pair<std::string, Dimensions_t>(dimName, dimSize));
    }

    // Add the maximum variable size to the ObsIo parmeters
    Dimensions_t maxVarSize = 0;
    for (std::size_t i = 0; i < writeVarConfigs.size(); ++i) {
        std::vector<std::string> dimNames = writeVarConfigs[i].getStringVector("dims");
        Dimensions_t varSize0 = dimSizes.at(dimNames[0]);
        if (varSize0 > maxVarSize) {
            maxVarSize = varSize0;
        }
    }
    obsParams.setMaxVarSize(maxVarSize);
}

// -----------------------------------------------------------------------------
std::string convertNewVnameToOldVname(const std::string & varName) {
    // New format is "Group/Variable", old format is "Variable@Group"
    std::string oldFormat;
    std::size_t pos = varName.find("/");
    if (pos == std::string::npos) {
        // no slash, just return the input string as is
        oldFormat = varName;
    } else {
        std::string gname = varName.substr(0, pos);
        std::string vname = varName.substr(pos + 1);
        oldFormat = vname + std::string("@") + gname;
    }
    return oldFormat;
}

// -----------------------------------------------------------------------------
}  // namespace ioda
