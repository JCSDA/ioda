/*
 * (C) Copyright 2018-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

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
std::vector<std::string> listDimVars(const Group & group) {
    std::vector<std::string> varList;
    std::vector<std::string> allVars = listAllVars(group, std::string(""));

    for (auto & varName : allVars) {
        if (varIsDimScale(group, varName)) {
            varList.push_back(varName);
        }
    }
    return varList;
}

// -----------------------------------------------------------------------------
std::vector<std::string> listVars(const Group & group) {
    std::vector<std::string> varList;
    std::vector<std::string> allVars = listAllVars(group, std::string(""));

    for (auto & varName : allVars) {
        if (!varIsDimScale(group, varName)) {
            varList.push_back(varName);
        }
    }
    return varList;
}

//------------------------------------------------------------------------------------
Dimensions_t varSize0(const Group & group, const std::string & varName) {
    Dimensions varDims = group.vars.open(varName).getDimensions();
    return varDims.dimsCur[0];
}

//------------------------------------------------------------------------------------
Dimensions_t maxVarSize0(const Group & group) {
    Dimensions_t size0Max = 0;
    for (auto & varName : listVars(group)) {
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
bool varIsDist(const Group & group, const std::string & varName) {
    bool isDist;
    Variable var = group.vars.open(varName);
    Variable nlocsVar = group.vars.open("nlocs");
    if (var.isDimensionScale()) {
        isDist = false;
    } else {
        isDist = var.isDimensionScaleAttached(0, nlocsVar);
    }
    return isDist;
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
void getReadFrameStringVar(const Variable & stringVar, const Dimensions_t frameStart,
                           const Dimensions_t frameCount,
                           std::vector<std::string> & stringVector) {
   // Generate the selection objects
   Dimensions varDims = stringVar.getDimensions();
   std::vector<Dimensions_t> counts = varDims.dimsCur;
   counts[0] = frameCount;
   std::vector<Dimensions_t> feStarts(counts.size(), 0);
   std::vector<Dimensions_t> beStarts(counts.size(), 0);
   beStarts[0] = frameStart;

   Selection feSelect;
   feSelect.extent(counts).select({ SelectionOperator::SET, feStarts, counts });
   Selection beSelect;
   beSelect.select({ SelectionOperator::SET, beStarts, counts });

   if (varDims.dimensionality > 1) {
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
}  // namespace ioda
