/*
 * (C) Copyright 2017-2021 UCAR
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

namespace osdf {
class FrameMetadata {
 public:
  FrameMetadata();

  //----------------------------------------------------------------------
  // Data member accessors

  /// \brief set channel numbers vector
  /// \param chanNums channel numbers used in column entries
  /// \details This set of channel numbers must line up with the names of
  /// the columns where there exists a "_<channel-number>" suffix for
  /// each variable. This vector is empty when channels are not being used.
  void setChanNums(const std::vector<int> & chanNums);

  /// \brief set the "variables using channels" set
  /// \param varsWithChans list of variables using channels
  void setVarsWithChans(const std::unordered_set<std::string> & varsWithChans);

  /// \brief set the number of variables
  /// \param numVars number of variables according to the details below
  /// \details This data member holds the total number of observation
  /// variables in the frame. This is typically not the number of columns
  /// for two reasons. First only the ObsValue columns are included in
  /// the count, and not columns such as the MetaData columns. Second,
  /// when channels are used, all of the ObsValue columns with matching
  /// names except for the "_<channel-number>" suffixes are counted as
  /// one variable. This lines up with how the ObsSpace counts variables.
  void setNumVars(const int numVars);

  /// \brief set the datetime epoch value
  /// \param epochString epoch string value
  /// \details This data member holds an ISO 8601 value (UTC) that
  /// represents the epoch for the MetaData/dateTime variable values.
  void setDateTimeEpoch(const std::string & epochString);


  /// \brief return the channel numbers vector
  const std::vector<int> & getChanNums() const;

  /// \brief return the "variables using channels" set
  const std::unordered_set<std::string> & getVarsWithChans() const;

  /// \brief return the dimensionality of a variable
  const std::vector<std::string> & getVarDimNames(const std::string & varName) const;

  /// \brief return the number of variables
  int getNumVars() const;

  /// \brief return the datetime epoch value
  std::string getDateTimeEpoch() const;

  //----------------------------------------------------------------------
  // utilities

  /// \brief increment the variable count
  void incrNumVars();

  /// \brief add variable to variables with channels list
  /// \param varName new variable name to add to the list
  void addVarToVarsWithChans(const std::string & varName);

  /// \brief add variable dimensionality to the variable dimensionality map
  /// \param varName new variable name to add to the list
  /// \param varDimNames list of dimension names in proper order
  void addVarDimNames(const std::string & varName, const std::vector<std::string> & varDimNames);

  /// \brief returns true if var is in the variables with channels list
  /// \param varName new variable name to add to the list
  bool varHasChannels(const std::string & varName) const;

 private:
  /// \brief frame channel numbers
  std::vector<int> chanNums_;

  /// \brief frame variables with channels
  std::unordered_set<std::string> varsWithChans_;

  // \brief dimension names for vars with channels
  std::unordered_map<std::string, std::vector<std::string>> varDimNames_;

  /// \brief number of frame variables
  int numVars_;

  /// \brief datetime epoch value
  std::string dateTimeEpoch_;
};
}  // namespace osdf

#endif  // CONTAINERS_FRAMEMETADATA_H_
