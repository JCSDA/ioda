/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FrameMetadata.h"
#include "eckit/exception/Exceptions.h"

namespace osdf {

//----------------------------------------------------------------------
FrameMetadata::FrameMetadata()
                  : chanNums_({}),
                    varsWithChans_({}),
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
bool FrameMetadata::varHasChannels(const std::string & varName) const {
  return (varsWithChans_.find(varName) != varsWithChans_.end());
}

}  // namespace osdf
