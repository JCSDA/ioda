/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameMetadata.h"

#include "eckit/exception/Exceptions.h"
#include "eckit/serialisation/ResizableMemoryStream.h"

namespace osdf {

//----------------------------------------------------------------------
FrameMetadata::FrameMetadata()
                  : chanNums_({}),
                    varsWithChans_({}),
                    varDimNames_({}),
                    numVars_(0),
                    dateTimeEpoch_("None") {
}

//----------------------------------------------------------------------
// setters
void FrameMetadata::setChanNums(const std::vector<int> & chanNums) {
  // For now, only set the first time. The assumption is that there exists
  // only one channel dimension and those channel numbers are being passed
  // into this function.
  if (chanNums_.empty()) {
    chanNums_ = chanNums;
  }
}

void FrameMetadata::setVarsWithChans(const std::unordered_set<std::string> & varsWithChans) {
  varsWithChans_ = varsWithChans;
}

void FrameMetadata::setNumVars(const int numVars) {
  numVars_ = numVars;
}

void FrameMetadata::setDateTimeEpoch(const std::string & epochString) {
  dateTimeEpoch_ = epochString;
}

//----------------------------------------------------------------------
// getters
const std::vector<int> & FrameMetadata::getChanNums() const {
  return chanNums_;
}

const std::unordered_set<std::string> & FrameMetadata::getVarsWithChans() const {
  return varsWithChans_;
}

const std::vector<std::string> & FrameMetadata::getVarDimNames(const std::string & varName) const {
  const auto it = varDimNames_.find(varName);
  if (it != varDimNames_.end()) {
    return it->second;
  } else {
    throw eckit::BadValue(
      "FrameMetadata::getVarDimNames: No dimension names found for variable: "
       + varName, Here());
  }
}

int FrameMetadata::getNumVars() const {
  return numVars_;
}

std::string FrameMetadata::getDateTimeEpoch() const {
  return dateTimeEpoch_;
}

//----------------------------------------------------------------------
void FrameMetadata::incrNumVars() {
  ++numVars_;
}

//----------------------------------------------------------------------
void FrameMetadata::addVarToVarsWithChans(const std::string & varName) {
  varsWithChans_.insert(varName);
}

//----------------------------------------------------------------------
void FrameMetadata::addVarDimNames(const std::string & varName,
                                   const std::vector<std::string> & varDimNames) {
  varDimNames_[varName] = varDimNames;
}

//----------------------------------------------------------------------
std::size_t FrameMetadata::removeVarDimNames(const std::string &varName) {
  return varDimNames_.erase(varName);
}

//----------------------------------------------------------------------
bool FrameMetadata::varHasChannels(const std::string & varName) const {
  return (varsWithChans_.find(varName) != varsWithChans_.end());
}

//----------------------------------------------------------------------
std::size_t FrameMetadata::serialize(eckit::Buffer & bufr) const {
  // Serialize in the following format using the eckit::Buffer and
  // eckit::ResizableMemoryStream utilities.
  //
  // 1. numVars_ (int)
  //      integer
  //
  // 2. dateTimeEpoch_ (string)
  //      string
  //
  // 3. chanNums_ (vector<int>)
  //      size_t - size of vector
  //      vector entries (int)
  //
  // 4. varsWithChans_ (unordered_set<string>)
  //      size_t - size of set
  //      vector entries (string)
  //
  // 5. varDimNames (unordered_map<strin, vector<string>>)
  //      size_t - size of map
  //      key,value pair
  //        size_t - size of vector
  //        vector entries (strings)
  //
  eckit::ResizableMemoryStream memStream(bufr);

  // numVars_
  memStream << numVars_;

  // dateTimeEpoch_
  memStream << dateTimeEpoch_;

  // chanNums_
  memStream << chanNums_.size();
  for (std::size_t i = 0; i < chanNums_.size(); ++i) {
    memStream << chanNums_[i];
  }

  // varsWithChans_
  memStream << varsWithChans_.size();
  for (const std::string & varName : varsWithChans_) {
    memStream << varName;
  }

  // varDimNames_
  memStream << varDimNames_.size();
  for (const std::pair<std::string, std::vector<std::string>> varDimInfo : varDimNames_) {
    memStream << varDimInfo.first;
    memStream << varDimInfo.second.size();
    for (const std::string & dimName : varDimInfo.second) {
      memStream << dimName;
    }
  }
  return memStream.position();
}

//----------------------------------------------------------------------
void FrameMetadata::deserialize(eckit::Buffer & bufr) {
  // Follow the layout described above in the serialize function.
  eckit::ResizableMemoryStream memStream(bufr);

  // numVars_
  memStream >> numVars_;

  // dateTimeEpoch_
  memStream >> dateTimeEpoch_;

  // chanNums_
  std::size_t numEntries;
  memStream >> numEntries;
  chanNums_.clear();
  chanNums_.resize(numEntries);
  for (std::size_t i = 0; i < numEntries; ++i) {
    memStream >> chanNums_[i];
  }

  // varsWithChans_
  memStream >> numEntries;
  varsWithChans_.clear();
  for (std::size_t i = 0; i < numEntries; ++i) {
    std::string varName;
    memStream >> varName;
    varsWithChans_.insert(varName);
  }

  // varDimNames_
  memStream >> numEntries;
  varDimNames_.clear();
  for (std::size_t i = 0; i < numEntries; ++i) {
    std::string varName;
    memStream >> varName;
    std::size_t numDimNames;
    memStream >> numDimNames;
    std::vector<std::string> dimNames(numDimNames);
    for (std::size_t j = 0; j < numDimNames; ++j) {
      memStream >> dimNames[j];
    }
    varDimNames_[varName] = dimNames;
  }
}

//----------------------------------------------------------------------
std::size_t FrameMetadata::bufrSize() const {
  // Add up the number of bytes of each data member. This will get a close
  // enough estimate of the actual size used by the eckit::Buffer
  // and eckit::ResizableMemoryStream utilities. The ResizableMemoryStream
  // checks if you are going to overrun the buffer and allocates more
  // space if necessary.
  //
  // This follows the layout described in the serialize function

  // numVars_
  std::size_t numBytes = sizeof(int);

  // dateTimeEpoch_
  numBytes += dateTimeEpoch_.size();

  // chanNums_
  numBytes += sizeof(std::size_t);  // size of vector
  numBytes += (chanNums_.size()) * sizeof(int);

  // varsWithChans_
  numBytes += sizeof(std::size_t);   // size of set
  for (const std::string & varName : varsWithChans_) {
    numBytes += varName.size();
  }

  // varDimNames_
  numBytes += sizeof(std::size_t);  // size of map
  for (const std::pair<std::string, std::vector<std::string>> varInfo : varDimNames_) {
    numBytes += varInfo.first.size();
    numBytes += sizeof(std::size_t);   // size of string vector (variable dim names)
    for (const std::string & varDimName : varInfo.second) {
      numBytes += varDimName.size();
    }
  }
  return numBytes;
}

//----------------------------------------------------------------------
bool FrameMetadata::operator==(const FrameMetadata & refFrameMetadata) const {
  // checke each data member for equality.
  const bool match = (this->numVars_ == refFrameMetadata.numVars_) &&
                     (this->dateTimeEpoch_ == refFrameMetadata.dateTimeEpoch_) &&
                     (this->chanNums_ == refFrameMetadata.chanNums_) &&
                     (this->varsWithChans_ == refFrameMetadata.varsWithChans_) &&
                     (this->varDimNames_ == refFrameMetadata.varDimNames_);
  return match;
}

}  // namespace osdf
