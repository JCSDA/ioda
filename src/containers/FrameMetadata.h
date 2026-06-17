/*
 * (C) Copyright 2025-2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMEMETADATA_H_
#define CONTAINERS_FRAMEMETADATA_H_

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "eckit/io/Buffer.h"

namespace osdf {
class FrameMetadata {
 public:
  FrameMetadata();

  //----------------------------------------------------------------------
  // Data member accessors

  /// \brief register coordinate values for a named second dimension
  /// \param dimName dimension name (e.g. "Channel", "Level", "nfactors")
  /// \param nums coordinate values for that dimension
  void setDimNums(const std::string &dimName, const std::vector<int> &nums);

  /// \brief return the coordinate values for a named dimension, or an empty
  ///        vector if the dimension has not been registered
  const std::vector<int> &getDimNums(const std::string &dimName) const;

  /// \brief return true if a named dimension has been registered
  bool hasDim(const std::string &dimName) const;

  /// \brief return the names of all registered dimensions
  std::vector<std::string> getDimNames() const;

  /// \brief set the number of variables
  /// \param numVars number of variables according to the details below
  /// \details This data member holds the total number of observation
  /// variables in the frame. This is typically not the number of columns
  /// for two reasons. First only the ObsValue columns are included in
  /// the count, and not columns such as the MetaData columns. Second,
  /// when a second dimension is used, all of the ObsValue columns with matching
  /// names except for the "_<index>" suffixes are counted as one variable.
  /// This lines up with how the ObsSpace counts variables.
  void setNumVars(const int numVars);

  /// \brief return the dimensionality of a variable
  const std::vector<std::string> &getVarDimNames(const std::string &varName) const;

  /// \brief return the number of variables
  int getNumVars() const;

  //----------------------------------------------------------------------
  // utilities

  /// \brief increment the variable count
  void incrNumVars();

  /// \brief add variable dimensionality to the variable dimensionality map
  /// \param varName variable name
  /// \param varDimNames list of dimension names in proper order
  void addVarDimNames(const std::string &varName, const std::vector<std::string> &varDimNames);

  /// \brief remove variable dimensionality from variable dimensionality map
  /// \param varName variable name to remove from the map
  std::size_t removeVarDimNames(const std::string &varName);

  /// \brief serialize for MPI data transfer
  /// \param bufr eckit buffer that will contain the serialized data
  /// \return total number of bytes that were serialized
  std::size_t serialize(eckit::Buffer &bufr) const;

  /// \brief deserialize for MPI data transfer
  /// \param bufr eckit buffer that contains the serialized data
  void deserialize(eckit::Buffer &bufr);

  /// \brief helper function to properly size the eckit buffer
  std::size_t bufrSize() const;

  /// \brief overload == operator
  bool operator==(const FrameMetadata &refFrameMetadata) const;

  //----------------------------------------------------------------------
  // Backward-compatible wrappers — kept for Phase 1/2/3 transition; removed in Phase 4

  /// \brief set channel numbers; wrapper for setDimNums("Channel", chanNums)
  void setChanNums(const std::vector<int> &chanNums);

  /// \brief return channel numbers; wrapper for getDimNums("Channel")
  const std::vector<int> &getChanNums() const;

  /// \brief return true if variable has a Channel second dimension
  bool varHasChannels(const std::string &varName) const;

  /// \brief return computed set of variables that have a Channel second dimension
  std::unordered_set<std::string> getVarsWithChans() const;

  /// \brief return the second (first non-"Location") dim name of a variable, or "" if the
  ///        variable is 1D/Location-only or not registered
  std::string varSecondDimName(const std::string &varName) const;

  /// \brief return all variable names that have any non-"Location" dimension
  std::unordered_set<std::string> getMultiSliceVars() const;

 private:
  /// \brief dimension name to coordinate values (e.g. "Channel" -> {1,3,5,...,22})
  std::unordered_map<std::string, std::vector<int>> dimNums_;

  /// \brief dimension names for each variable
  std::unordered_map<std::string, std::vector<std::string>> varDimNames_;

  /// \brief number of frame variables
  int numVars_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMEMETADATA_H_
