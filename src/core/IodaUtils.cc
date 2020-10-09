/*
 * (C) Copyright 2018-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <fstream>
#include <iomanip>

#include "ioda/core/IodaUtils.h"

#include "ioda/Variables/Variable.h"

#include "oops/util/DateTime.h"


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

// -----------------------------------------------------------------------------
std::vector<std::string> listAllVars(const Group & group, std::string varPath) {
    std::vector<std::string> varList;

    // Add in variables at this level
    std::vector<std::string> myVars;
    for (auto & varName : group.vars.list()) {
        myVars.push_back(varPath + varName);
    }
    varList.insert(varList.end(), myVars.begin(), myVars.end());

    // Traverse to child groups and append their var lists
    for (auto & childGroup : group.list()) {
        std::string childVarPath = varPath + childGroup + std::string("/");
        std::vector<std::string> childVarList =
            listAllVars(group.open(childGroup), childVarPath);
        varList.insert(varList.end(), childVarList.begin(), childVarList.end());
    }

    return varList;
}

// -----------------------------------------------------------------------------
void getVarLists(const Group & group, std::vector<std::string> & varList,
                 std::vector<std::string> & dimVarList) {
    // Get a list of all variables
    std::vector<std::string> allVars = listAllVars(group, std::string(""));

    varList.clear();
    dimVarList.clear();
    for (auto & varName : allVars) {
        if (varIsDimScale(group, varName)) {
            // To help speed up the genDimAttachedToVars routine, it is advantageous
            // to make nlocs be the first in the list. There are typically a vary small
            // number of dimension variables, so it doesn't hurt to use insert to put
            // nlocs at the front.
            if (varName == "nlocs") {
                dimVarList.insert(dimVarList.begin(), varName);
            } else {
                dimVarList.push_back(varName);
            }
        } else {
            varList.push_back(varName);
        }
    }
}

//------------------------------------------------------------------------------------
Dimensions_t varSize0(const Group & group, const std::string & varName) {
    Dimensions varDims = group.vars.open(varName).getDimensions();
    return varDims.dimsCur[0];
}

//------------------------------------------------------------------------------------
Dimensions_t maxVarSize0(const Group & group, const std::vector<std::string> & varList) {
    Dimensions_t size0Max = 0;
    for (auto & varName : varList) {
        Dimensions_t size0 = varSize0(group, varName);
        if (size0 > size0Max) {
            size0Max = size0;
        }
    }
    return size0Max;
}

//------------------------------------------------------------------------------------
std::type_index varDtype(const Group & group, const std::string & varName) {
    Variable var = group.vars.open(varName);
    std::type_index varType(typeid(std::string));
    if (var.isA<int>()) {
        varType = typeid(int);
    } else if (var.isA<float>()) {
        varType = typeid(float);
    }
    return varType;
}

//------------------------------------------------------------------------------------
bool varIsDimScale(const Group & group, const std::string & varName) {
    Variable var = group.vars.open(varName);
    return var.isDimensionScale();
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
std::vector<util::DateTime> convertRefOffsetToDtime(const int refIntDtime,
                                                    const std::vector<float> & timeOffsets) {
    // convert refDtime to a DateTime object
    int Year = refIntDtime / 1000000;
    int TempInt = refIntDtime % 1000000;
    int Month = TempInt / 10000;
    TempInt = TempInt % 10000;
    int Day = TempInt / 100;
    int Hour = TempInt % 100;
    util::DateTime refDtime(Year, Month, Day, Hour, 0, 0);

    // Convert offset time to a Duration and add to RefDate.
    std::size_t dtimeSize = timeOffsets.size();
    std::vector<util::DateTime> dateTimeValues(dtimeSize);
    for (std::size_t i = 0; i < dtimeSize; ++i) {
        util::DateTime dateTime =
            refDtime + util::Duration(round(timeOffsets[i] * 3600));
        dateTimeValues[i] = dateTime;
    }
    return dateTimeValues;
}

//------------------------------------------------------------------------------------
void getFrameStringVar(const Variable & stringVar, const Selection feSelect,
                       const Selection beSelect, const Dimensions_t frameCount,
                       std::vector<std::string> & stringVector) {
    Dimensions varDims = stringVar.getDimensions();
    if (varDims.dimensionality > 1) {
        // The shape of the data in the stringArray vector will be the same
        // as the shape of the frame variable (dimsCur), but with the first
        // dimension size adjusted to the size of the selection in the backend for
        // the first dimension.
        std::vector<Dimensions_t> varShape = varDims.dimsCur;
        varShape[0] = frameCount;
        std::vector<std::string> stringArray =
            stringVar.readAsVector<std::string>(feSelect, beSelect);
        stringVector = StringArrayToStringVector(stringArray, varShape);
    } else {
        stringVar.read<std::string>(stringVector, feSelect, beSelect);
    }
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
        obsConfig.getSubConfigurations("test data.write dimensions");
    std::vector<eckit::LocalConfiguration> writeVarConfigs =
        obsConfig.getSubConfigurations("test data.write variables");

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
std::string uniquifyFileName(const std::string & fileName, const std::size_t rankNum,
                             const std::size_t timeRankNum) {
    // Attach the rank number to the output file name to avoid collisions when running
    // with multiple MPI tasks.
    std::string uniqueFileName = fileName;

    // Find the left-most dot in the file name, and use that to pick off the file name
    // and file extension.
    std::size_t found = uniqueFileName.find_last_of(".");
    if (found == std::string::npos)
      found = uniqueFileName.length();

    // Get the process rank number and format it
    std::ostringstream ss;
    ss << "_" << std::setw(4) << std::setfill('0') << rankNum;
    if (timeRankNum >= 0) ss << "_" << timeRankNum;

    // Construct the output file name
    return uniqueFileName.insert(found, ss.str());
}

// -----------------------------------------------------------------------------
VarDimMap genDimsAttachedToVars(const Has_Variables & varContainer,
                                const std::vector<std::string> & varList,
                                const std::vector<std::string> & dimVarList) {
    VarDimMap dimsAttachedToVars;
    for (auto & varName : varList) {
        Variable var = varContainer.open(varName);
        std::vector<std::string> dimVarNames;
        for (std::size_t i = 0; i < var.getDimensions().dimensionality; ++i) {
            for (auto & dimVarName : dimVarList) {
                Variable dimVar = varContainer.open(dimVarName);
                if (var.isDimensionScaleAttached(i, dimVar)) {
                    dimVarNames.push_back(dimVarName);
                    break;
                }
            }
        }
        dimsAttachedToVars.insert(
            std::pair<std::string, std::vector<std::string>>(varName, dimVarNames));
    }
    return dimsAttachedToVars;
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
        std::string vname = varName.substr(pos);
        oldFormat = vname + std::string("@") + gname;
    }
    return oldFormat;
}

// -----------------------------------------------------------------------------
}  // namespace ioda
