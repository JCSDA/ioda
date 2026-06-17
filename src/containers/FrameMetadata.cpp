/*
 * (C) Copyright 2025-2026 UCAR
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
                  : dimNums_({}),
                    varDimNames_({}),
                    numVars_(0) {
}

//----------------------------------------------------------------------
void FrameMetadata::setDimNums(const std::string &dimName, const std::vector<int> &nums) {
  // We don't want to allow overwriting or duplicating an existing dimension.
  if (dimNums_.count(dimName) == 0) {
    dimNums_.insert({dimName, nums});
  } else {
    throw eckit::BadValue("FrameMetadata::setDimNums: Dimension already registered: " +
                           dimName, Here());
  }
}

const std::vector<int> &FrameMetadata::getDimNums(const std::string &dimName) const {
  static const std::vector<int> emptyVec;
  const auto it = dimNums_.find(dimName);
  return (it != dimNums_.end()) ? it->second : emptyVec;
}

bool FrameMetadata::hasDim(const std::string &dimName) const {
  return dimNums_.count(dimName) > 0;
}

std::vector<std::string> FrameMetadata::getDimNames() const {
  std::vector<std::string> names;
  names.reserve(dimNums_.size());
  for (const auto &kv : dimNums_) {
    names.push_back(kv.first);
  }
  return names;
}

std::string FrameMetadata::varSecondDimName(const std::string &varName) const {
  const auto it = varDimNames_.find(varName);
  if (it == varDimNames_.end()) return "";
  for (const auto &d : it->second) {
    if (d != "Location") return d;
  }
  return "";
}

std::unordered_set<std::string> FrameMetadata::getMultiSliceVars() const {
  std::unordered_set<std::string> result;
  for (const auto &kv : varDimNames_) {
    if (!varSecondDimName(kv.first).empty()) result.insert(kv.first);
  }
  return result;
}

//----------------------------------------------------------------------
// setters
void FrameMetadata::setNumVars(const int numVars) {
  numVars_ = numVars;
}

//----------------------------------------------------------------------
// getters
const std::vector<std::string> &FrameMetadata::getVarDimNames(const std::string &varName) const {
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

//----------------------------------------------------------------------
void FrameMetadata::incrNumVars() {
  ++numVars_;
}

//----------------------------------------------------------------------
void FrameMetadata::addVarDimNames(const std::string &varName,
                                   const std::vector<std::string> &varDimNames) {
  varDimNames_[varName] = varDimNames;
}

//----------------------------------------------------------------------
std::size_t FrameMetadata::removeVarDimNames(const std::string &varName) {
  return varDimNames_.erase(varName);
}

//----------------------------------------------------------------------
// Backward-compatible wrappers
void FrameMetadata::setChanNums(const std::vector<int> &chanNums) {
  if (!hasDim("Channel")) setDimNums("Channel", chanNums);
}

const std::vector<int> &FrameMetadata::getChanNums() const {
  return getDimNums("Channel");
}

bool FrameMetadata::varHasChannels(const std::string &varName) const {
  return varSecondDimName(varName) == "Channel";
}

std::unordered_set<std::string> FrameMetadata::getVarsWithChans() const {
  std::unordered_set<std::string> result;
  for (const auto &kv : varDimNames_) {
    if (varSecondDimName(kv.first) == "Channel") result.insert(kv.first);
  }
  return result;
}

//----------------------------------------------------------------------
std::size_t FrameMetadata::serialize(eckit::Buffer &bufr) const {
  // Serialize in the following format using eckit::Buffer and
  // eckit::ResizableMemoryStream.
  //
  // 1. numVars_ (int)
  //
  // 2. dimNums_ (unordered_map<string, vector<int>>)
  //      size_t - number of entries
  //      per entry:
  //        string - dimension name
  //        size_t - number of coordinate values
  //        int[]  - coordinate values
  //
  // 3. varDimNames_ (unordered_map<string, vector<string>>)
  //      size_t - number of entries
  //      per entry:
  //        string   - variable name
  //        size_t   - number of dimension names
  //        string[] - dimension names
  //
  eckit::ResizableMemoryStream memStream(bufr);

  // numVars_
  memStream << numVars_;

  // dimNums_
  memStream << dimNums_.size();
  for (const auto &kv : dimNums_) {
    memStream << kv.first;
    memStream << kv.second.size();
    for (int n : kv.second) {
      memStream << n;
    }
  }

  // varDimNames_
  memStream << varDimNames_.size();
  for (const auto &kv : varDimNames_) {
    memStream << kv.first;
    memStream << kv.second.size();
    for (const std::string &dimName : kv.second) {
      memStream << dimName;
    }
  }

  return memStream.position();
}

//----------------------------------------------------------------------
void FrameMetadata::deserialize(eckit::Buffer &bufr) {
  // Follow the layout described in the serialize function.
  eckit::ResizableMemoryStream memStream(bufr);

  // numVars_
  memStream >> numVars_;

  // dimNums_
  std::size_t numEntries;
  memStream >> numEntries;
  dimNums_.clear();
  for (std::size_t i = 0; i < numEntries; ++i) {
    std::string dimName;
    memStream >> dimName;
    std::size_t numNums;
    memStream >> numNums;
    std::vector<int> nums(numNums);
    for (std::size_t j = 0; j < numNums; ++j) {
      memStream >> nums[j];
    }
    dimNums_[dimName] = nums;
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
  // Add up the number of bytes of each data member to estimate the buffer size.
  // The ResizableMemoryStream will grow the buffer if needed.
  // This follows the layout described in the serialize function.

  // numVars_
  std::size_t numBytes = sizeof(int);

  // dimNums_
  numBytes += sizeof(std::size_t);  // number of entries
  for (const auto &kv : dimNums_) {
    numBytes += kv.first.size();
    numBytes += sizeof(std::size_t);              // number of coordinate values
    numBytes += kv.second.size() * sizeof(int);   // coordinate values
  }

  // varDimNames_
  numBytes += sizeof(std::size_t);  // number of entries
  for (const auto &kv : varDimNames_) {
    numBytes += kv.first.size();
    numBytes += sizeof(std::size_t);  // number of dimension names
    for (const std::string &dimName : kv.second) {
      numBytes += dimName.size();
    }
  }

  return numBytes;
}

//----------------------------------------------------------------------
bool FrameMetadata::operator==(const FrameMetadata &refFrameMetadata) const {
  return (this->numVars_ == refFrameMetadata.numVars_) &&
         (this->dimNums_ == refFrameMetadata.dimNums_) &&
         (this->varDimNames_ == refFrameMetadata.varDimNames_);
}

}  // namespace osdf
