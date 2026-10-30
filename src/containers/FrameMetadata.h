/*
 * (C) Copyright 2025-2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_FRAMEMETADATA_H_
#define CONTAINERS_FRAMEMETADATA_H_

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "eckit/io/Buffer.h"

namespace osdf {

/// \brief storage type of a dimension's coordinate variable in its source file
enum class DimCoordType : std::int8_t {
  eInt,
  eInt64,
  eFloat
};

/// \brief coordinate values of a dimension as they appear in its source file
/// \details These are kept apart from the dimension numbers (see setDimNums) so that
///          the writer can reproduce the source coordinates, whatever numbers are used
///          to key the per-slice columns. The reader only accepts coordinates holding
///          whole numbers (int, int64, or float with whole values), so the values are
///          held exactly as int64.
struct DimCoords {
  std::vector<std::int64_t> values;
  DimCoordType type = DimCoordType::eInt;

  bool operator==(const DimCoords &other) const {
    return (values == other.values) && (type == other.type);
  }
};

class FrameMetadata {
 public:
  FrameMetadata();

  //----------------------------------------------------------------------
  // Data member accessors

  /// \brief register the dimension numbers for a named slice dimension
  /// \details The dimension numbers are the "_<number>" suffixes of the per-slice
  ///          column names. For Channel they are the channel numbers. For every other
  ///          slice dimension they are the 1-based positions 1..N.
  /// \param dimName dimension name (e.g. "Channel", "Level", "nfactors")
  /// \param nums dimension numbers for that dimension
  void setDimNums(const std::string &dimName, const std::vector<int> &nums);

  /// \brief return the dimension numbers for a named dimension, or an empty
  ///        vector if the dimension has not been registered
  const std::vector<int> &getDimNums(const std::string &dimName) const;

  /// \brief record the source file coordinate values for a named slice dimension
  /// \param dimName dimension name
  /// \param coords coordinate values and their storage type
  /// \throws eckit::BadValue if the dimension is not registered (see setDimNums), or if the
  ///         number of coordinate values differs from the number of dimension numbers
  void setDimCoords(const std::string &dimName, const DimCoords &coords);

  /// \brief return true if source file coordinate values were recorded for a dimension
  bool hasDimCoords(const std::string &dimName) const;

  /// \brief return the source file coordinate values for a named dimension
  /// \throws eckit::BadValue if no coordinate values were recorded for the dimension
  const DimCoords &getDimCoords(const std::string &dimName) const;

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
  /// when a slice dimension is used, all of the ObsValue columns with matching
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

  /// \brief return the name of a variable's slice dimension -- its first dimension
  ///        other than "Location" (e.g. "Channel", "Level") -- or "" if the variable
  ///        is 1D/Location-only or not registered
  std::string varSliceDimName(const std::string &varName) const;

  /// \brief return all variable names that have a slice dimension (any dimension
  ///        other than "Location")
  std::unordered_set<std::string> getMultiSliceVars() const;

 private:
  /// \brief dimension name to dimension numbers (e.g. "Channel" -> {1,3,5,...,22})
  std::unordered_map<std::string, std::vector<int>> dimNums_;

  /// \brief dimension name to source file coordinate values
  std::unordered_map<std::string, DimCoords> dimCoords_;

  /// \brief dimension names for each variable
  std::unordered_map<std::string, std::vector<std::string>> varDimNames_;

  /// \brief number of frame variables
  int numVars_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMEMETADATA_H_
