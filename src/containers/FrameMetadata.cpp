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
                    dimCoords_({}),
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

void FrameMetadata::setDimCoords(const std::string &dimName, const DimCoords &coords) {
  const auto it = dimNums_.find(dimName);
  if (it == dimNums_.end()) {
    throw eckit::BadValue("FrameMetadata::setDimCoords: Dimension not registered: " +
                          dimName, Here());
  }
  if (coords.values.size() != it->second.size()) {
    throw eckit::BadValue("FrameMetadata::setDimCoords: Number of coordinate values (" +
                          std::to_string(coords.values.size()) + ") does not match the " +
                          "number of dimension numbers (" + std::to_string(it->second.size()) +
                          ") for dimension: " + dimName, Here());
  }
  dimCoords_[dimName] = coords;
}

bool FrameMetadata::hasDimCoords(const std::string &dimName) const {
  return dimCoords_.count(dimName) > 0;
}

const DimCoords &FrameMetadata::getDimCoords(const std::string &dimName) const {
  const auto it = dimCoords_.find(dimName);
  if (it == dimCoords_.end()) {
    throw eckit::BadValue("FrameMetadata::getDimCoords: No coordinate values recorded for "
                          "dimension: " + dimName, Here());
  }
  return it->second;
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

std::string FrameMetadata::varSliceDimName(const std::string &varName) const {
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
    if (!varSliceDimName(kv.first).empty()) result.insert(kv.first);
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
  //        size_t - number of dimension numbers
  //        int[]  - dimension numbers
  //
  // 3. dimCoords_ (unordered_map<string, DimCoords>)
  //      size_t - number of entries
  //      per entry:
  //        string   - dimension name
  //        int       - coordinate storage type
  //        size_t    - number of coordinate values
  //        int64_t[] - coordinate values
  //
  // 4. varDimNames_ (unordered_map<string, vector<string>>)
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

  // dimCoords_
  memStream << dimCoords_.size();
  for (const auto &kv : dimCoords_) {
    memStream << kv.first;
    memStream << static_cast<int>(kv.second.type);
    memStream << kv.second.values.size();
    for (const std::int64_t v : kv.second.values) {
      // Use long long to ensure that the value is serialized as 8 bytes.
      // Without the long long cast, the values may be truncated by the
      // eckit Stream operator <<.
      memStream << static_cast<long long>(v);  // NOLINT(runtime/int)
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

  // dimCoords_
  memStream >> numEntries;
  dimCoords_.clear();
  for (std::size_t i = 0; i < numEntries; ++i) {
    std::string dimName;
    memStream >> dimName;
    int coordType;
    memStream >> coordType;
    std::size_t numValues;
    memStream >> numValues;
    DimCoords coords;
    coords.type = static_cast<DimCoordType>(coordType);
    coords.values.resize(numValues);
    for (std::size_t j = 0; j < numValues; ++j) {
      long long v;  // NOLINT(runtime/int)
      memStream >> v;
      coords.values[j] = v;
    }
    dimCoords_[dimName] = coords;
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
    numBytes += sizeof(std::size_t);              // number of dimension numbers
    numBytes += kv.second.size() * sizeof(int);   // dimension numbers
  }

  // dimCoords_
  numBytes += sizeof(std::size_t);  // number of entries
  for (const auto &kv : dimCoords_) {
    numBytes += kv.first.size();
    numBytes += sizeof(int);                              // coordinate storage type
    numBytes += sizeof(std::size_t);                      // number of coordinate values
    numBytes += kv.second.values.size() * sizeof(std::int64_t);  // coordinate values
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
         (this->dimCoords_ == refFrameMetadata.dimCoords_) &&
         (this->varDimNames_ == refFrameMetadata.varDimNames_);
}

}  // namespace osdf
