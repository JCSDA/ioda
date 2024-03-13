/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_engines_pub_ODC
 *
 * @{
 * \file ODC.cpp
 * \brief ODB / ODC engine bindings
 */
#include <algorithm>
#include <regex>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>
#include <ctime>

#include "DataFromSQL.h"

#include "eckit/io/MemoryHandle.h"
#include "ioda/Engines/ODC.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/config.h"  // Auto-generated. Defines *_FOUND.
#include "ioda/Types/Type.h"
#include "oops/util/Logger.h"

#if odc_FOUND
# include "eckit/config/LocalConfiguration.h"
# include "eckit/config/YAMLConfiguration.h"
# include "eckit/filesystem/PathName.h"
# include "odc/api/odc.h"
# include "odc/Writer.h"
# include "./DataFromSQL.h"
# include "./OdbQueryParameters.h"
# include "../../Layouts/Layout_ObsGroup_ODB_Params.h"
#endif

namespace ioda {
namespace Engines {
namespace ODC {

namespace {

// -------------------------------------------------------------------------------------------------
// Initialization

#if odc_FOUND
#else
/// @brief Standard message when the ODC API is unavailable.
const char odcMissingMessage[] {
  "The ODB / ODC engine is disabled. Either odc, eckit, or oops were "
  "not found at compile time."};
#endif

/// @brief Function initializes the ODC API, just once.
void initODC() {
  static bool inited = false;
  if (!inited) {
#if odc_FOUND
    odc_initialise_api();
#else
    throw Exception(odcMissingMessage, ioda_Here());
#endif
    inited = true;
  }
}

#if odc_FOUND

// -------------------------------------------------------------------------------------------------

/// Convert an epoch to a util::DateTime
// todo: keep unified with the version in IodaUtils.cc
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

// -------------------------------------------------------------------------------------------------
// (Very simple) SQL expression parsing

/// Parsed SQL column expression.
struct ParsedColumnExpression {
  /// If `expression` is a bitfield column member name (of the form `column.member[@table]`,
  /// where `@table` is optional), split it into the column name `column[@table]` and member
  /// name `member`. Otherwise leave it unchanged.
  explicit ParsedColumnExpression(const std::string &expression) {
    static const std::regex re("(\\w+)(?:\\.(\\w+))?(?:@(.+))?");
    std::smatch match;
    if (std::regex_match(expression, match, re)) {
      // This is an identifier of the form column[.member][@table]
      column += match[1].str();
      if (match[3].length()) {
         column += '@';
         column += match[3].str();
      }
      member = match[2];
    } else {
      // This is a more complex expression
      column = expression;
    }
  }

  std::string column;  //< Column name (possibly including table name) or a more general expression
  std::string member;  //< Bitfield member name (may be empty)
};

// -------------------------------------------------------------------------------------------------
// Query file parsing

/// \brief The set of ODB column members selected by the query file.
///
/// (Only bitfield columns have members; other columns can only be selected as a whole. Bitfield
/// columns can also be selected as a whole.)
class MemberSelection {
 public:
  /// Create an empty selection.
  MemberSelection() = default;

  /// Return true if the whole column has been selected, false otherwise.
  bool allMembersSelected() const { return allMembersSelected_; }
  /// Return the set of selected members or an empty set if the whole column has been selected.
  const std::set<std::string> &selectedMembers() const { return selectedMembers_; }

  /// Add member `member` to the selection.
  void addMember(const std::string &member) {
    if (!allMembersSelected_)
      selectedMembers_.insert(member);
  }

  /// Add all members to the selection.
  void addAllMembers() {
    allMembersSelected_ = true;
    selectedMembers_.clear();
  }

  /// Return the intersection of `members` with the set of selected members.
  std::set<std::string> intersectionWith(const std::set<std::string> &members) const {
    if (allMembersSelected()) {
      return members;
    } else {
      std::set<std::string> intersection;
      std::set_intersection(members.begin(), members.end(),
                            selectedMembers_.begin(), selectedMembers_.end(),
                            std::inserter(intersection, intersection.end()));
      return intersection;
    }
  }

 private:
  std::set<std::string> selectedMembers_;
  /// True if the column has been selected as a whole (i.e. effectively all members are selected)
  bool allMembersSelected_ = false;
};

/// The set of ODB columns selected by the query file (possibly only partially, i.e. including only
/// a subset of bitfield members).
class ColumnSelection {
 public:
  ColumnSelection() = default;

  void addColumn(const std::string &column) {
    members_[column].addAllMembers();
  }

  void addColumnMember(const std::string &column, const std::string &member) {
    members_[column].addMember(member);
  }

  /// Return the sorted list of
  std::vector<std::string> columns() const {
    std::vector<std::string> result;
    for (const auto &columnAndMembers : members_) {
      result.push_back(columnAndMembers.first);
    }
    return result;
  }

  const MemberSelection &columnMembers(const std::string &column) const {
    return members_.at(column);
  }

 private:
  std::map<std::string, MemberSelection> members_;
};

/// Select columns and column members specified in the `variables` list in the query file.
void addQueryColumns(ColumnSelection &selection, const OdbQueryParameters &queryParameters) {
  for (const OdbVariableParameters &varParameters : queryParameters.variables.value()) {
    ParsedColumnExpression parsedSource(varParameters.name);
    if (parsedSource.member.empty()) {
      selection.addColumn(parsedSource.column);
    } else {
      selection.addColumnMember(parsedSource.column, parsedSource.member);
    }
  }
}

// -------------------------------------------------------------------------------------------------
// Mapping file parsing

/// A column not treated as a bitfield. (If may technically be a bitfield column, but if so, it is
/// to be treated as a normal int column.)
class NonbitfieldColumnMapping {
public:
  NonbitfieldColumnMapping() = default;

  void markAsVarnoIndependent() { varnoIndependent_ = true; }

  void dimensionOfVarno(int varno) { dimensionOfVarno_ = varno; }

  void addVarno(int varno) {
    varnos_.insert(varno);
  }

  void checkConsistency(const std::string &column) const {
    if (varnoIndependent_ && !varnos_.empty())
      throw eckit::UserError("Column '" + column +
                             "' is declared both as varno-independent and varno-dependent",
                             Here());
  }

  void createIodaVariables(const DataFromSQL &sqlData,
                           const std::string &column,
                           const std::vector<int> &varnoSelection,
                           const VariableCreationParameters &creationParams,
                           ObsGroup &og) const {
    if (varnoIndependent_) {
      if (dimensionOfVarno_ == 0) {
        sqlData.createVarnoIndependentIodaVariable(column, og, creationParams);
      } else {
        sqlData.createVarnoDependentIodaVariable(column, dimensionOfVarno_, og,
                                                 creationParams, column);
      }
    } else {
      for (int varno : varnoSelection) {
        if (varnos_.count(varno)) {
          if (sqlData.getObsgroup() == obsgroup_amsr && varno != varno_rawbt)
            continue;
          if (sqlData.getObsgroup() == obsgroup_mwsfy3 && varno != varno_rawbt_mwts)
            continue;
          sqlData.createVarnoDependentIodaVariable(column, varno, og, creationParams);
        }
      }
    }
  }

 private:
  /// If true, the column is treated as varno-independent and mapped to a single ioda variable.
  /// Otherwise the restriction of the column to each of the varnos `varnos_` is mapped to a
  /// separate ioda variable.
  bool varnoIndependent_ = false;
  int dimensionOfVarno_ = 0;
  std::set<int> varnos_;
};

/// A column treated as a bitfield.
class BitfieldColumnMapping {
 public:
  BitfieldColumnMapping() = default;

  void addVarnoIndependentMember(const std::string &member) {
    varnoIndependentMembers_.insert(member);
  }

  void addVarnoDependentMember(int varno, const std::string &member) {
    varnoDependentMembers_[varno].insert(member);
  }

  void checkConsistency(const std::string &column) const {
    for (const auto &varnoAndMembers : varnoDependentMembers_)
      for (const std::string &member : varnoAndMembers.second)
        if (varnoIndependentMembers_.count(member))
          throw eckit::UserError("Bitfield column member '" + column + "." + member +
                                 "' is declared both as varno-independent and varno-dependent",
                                 Here());
  }

  void createIodaVariables(const DataFromSQL &sqlData,
                           const std::string &columnName,
                           const MemberSelection &memberSelection,
                           const std::vector<int> &varnoSelection,
                           const VariableCreationParameters &creationParams,
                           ObsGroup &og) const {
    if (!varnoIndependentMembers_.empty()) {
      const std::set<std::string> members =
          memberSelection.intersectionWith(varnoIndependentMembers_);
      sqlData.createVarnoIndependentIodaVariables(
            columnName, members, og, creationParams);
    }
    if (!varnoDependentMembers_.empty()) {
      for (int varno : varnoSelection) {
        const auto membersIt = varnoDependentMembers_.find(varno);
        if (membersIt != varnoDependentMembers_.end()) {
          if (sqlData.getObsgroup() == obsgroup_amsr && varno != varno_rawbt)
            continue;
          if (sqlData.getObsgroup() == obsgroup_mwsfy3 && varno != varno_rawbt_mwts)
            continue;
          const std::set<std::string> members =
              memberSelection.intersectionWith(membersIt->second);
          sqlData.createVarnoDependentIodaVariables(
                columnName, members, varno, og, creationParams);
        }
      }
    }
  }

 private:
  /// Varno-independent bitfield members (each mapped to a single ioda variable)
  std::set<std::string> varnoIndependentMembers_;
  /// Maps varnos to sets of bitfield members whose restrictions to those varnos are mapped to
  /// separate ioda variables.
  std::map<int, std::set<std::string>> varnoDependentMembers_;
};

/// This object
/// * lists columns and bitfield column members for which a mapping to ioda variables has
///   been defined in a mapping file,
/// * indicates which of them should be treated as varno-dependent, and
/// * lists the varnos for which a mapping of each varno-dependent column or column member has
///   been defined.
struct ColumnMappings {
  std::map<std::string, NonbitfieldColumnMapping> nonbitfieldColumns;
  std::map<std::string, BitfieldColumnMapping> bitfieldColumns;
};

/// Parse the mapping file and return an object
/// * listing columns and bitfield column members for which a mapping to ioda variables has
///   been defined,
/// * indicating which of them should be treated as varno-dependent, and
/// * listing the varnos for which a mapping of each varno-dependent column or column member has
///   been defined.
ColumnMappings collectColumnMappings(const detail::ODBLayoutParameters &layoutParams) {
  ColumnMappings mappings;

  // Process varno-independent columns
  for (const detail::VariableParameters &columnParams : layoutParams.variables.value()) {
    ParsedColumnExpression parsedSource(columnParams.source);
    if (parsedSource.member.empty()) {
      mappings.nonbitfieldColumns[parsedSource.column].markAsVarnoIndependent();
      if (columnParams.varnoWithSameDimensionAsVariable.value().has_value())
        mappings.nonbitfieldColumns[parsedSource.column].dimensionOfVarno(
                    columnParams.varnoWithSameDimensionAsVariable.value().get());
    } else {
      mappings.bitfieldColumns[parsedSource.column].addVarnoIndependentMember(parsedSource.member);
    }
  }

  // Process varno-dependent columns
  for (const detail::VarnoDependentColumnParameters &columnParams :
       layoutParams.varnoDependentColumns.value()) {
    ParsedColumnExpression parsedSource(columnParams.source);
    if (parsedSource.member.empty()) {
      NonbitfieldColumnMapping &mapping = mappings.nonbitfieldColumns[parsedSource.column];
      for (const auto &mappingParams : columnParams.mappings.value()) {
        mapping.addVarno(mappingParams.varno);
      }
    } else {
      BitfieldColumnMapping &mapping = mappings.bitfieldColumns[parsedSource.column];
      for (const auto &mappingParams : columnParams.mappings.value()) {
        mapping.addVarnoDependentMember(mappingParams.varno, parsedSource.member);
      }
    }
  }

  // Process complementary columns
  for (const detail::ComplementaryVariablesParameters &varParams :
       layoutParams.complementaryVariables.value()) {
    // These currently must be string-valued columns, so they cannot be bitfields.
    // And they are varno-independent.
    for (const std::string &input : varParams.inputNames.value()) {
      mappings.nonbitfieldColumns[input].markAsVarnoIndependent();
    }
  }

  // Check consistency (no non-bitfield column or bitfield column member should be declared both
  // as varno-independent and varno-dependent)
  for (const auto &columnAndMapping : mappings.nonbitfieldColumns)
    columnAndMapping.second.checkConsistency(columnAndMapping.first);
  for (const auto &columnAndMapping : mappings.bitfieldColumns)
    columnAndMapping.second.checkConsistency(columnAndMapping.first);

  return mappings;
}

struct ReverseColumnMappings {
  std::map<std::string, std::string> varnoIndependentColumns;
  std::map<std::string, std::string> varnoDependentColumns;
  std::map<std::string, std::string> varnoDependentColumnsNames;
  std::map<std::string, std::string> complimentaryVariableColumns;
};

/// Parse the mapping file and return an object
/// * listing columns and bitfield column members for which a mapping to ioda variables has
///   been defined,
/// * indicating which of them should be treated as varno-dependent, and
/// * listing the varnos for which a mapping of each varno-dependent column or column member has
///   been defined.
ReverseColumnMappings collectReverseColumnMappings(const detail::ODBLayoutParameters &layoutParams,
                                                   const std::vector<std::string> &columns,
                                                   const std::vector<int> &listOfVarNos) {
  ReverseColumnMappings mappings;

  // Process varno-independent columns
  for (const detail::VariableParameters &columnParams : layoutParams.variables.value()) {      
    const auto it = std::find(columns.begin(), columns.end(), columnParams.source.value());
    if (it != columns.end())
      mappings.varnoIndependentColumns[columnParams.name.value()] = columnParams.source.value();
  }

  // Add some default and an optional variables if not present
  if (mappings.varnoIndependentColumns.find("MetaData/latitude") == mappings.varnoIndependentColumns.end())
    mappings.varnoIndependentColumns["MetaData/latitude"] = "lat";
  if (mappings.varnoIndependentColumns.find("MetaData/longitude") == mappings.varnoIndependentColumns.end())
    mappings.varnoIndependentColumns["MetaData/longitude"] = "lon";
  if (mappings.varnoIndependentColumns.find("MetaData/dateTime") == mappings.varnoIndependentColumns.end())
    mappings.varnoIndependentColumns["MetaData/dateTime"] = "date";
  if (std::find(columns.begin(), columns.end(), "receipt_date") != columns.end() &&
      mappings.varnoIndependentColumns.find("MetaData/receiptdateTime") == mappings.varnoIndependentColumns.end())
    mappings.varnoIndependentColumns["MetaData/receiptdateTime"] = "receipt_date";

  for (const detail::VarnoDependentColumnParameters &columnParams :
       layoutParams.varnoDependentColumns.value()) {
    if (columnParams.source.value() == std::string("initial_obsvalue")) {
      for (const auto &mappingParams : columnParams.mappings.value()) {
        const auto it = std::find(listOfVarNos.begin(), listOfVarNos.end(), mappingParams.varno);
        if (it != listOfVarNos.end())
            mappings.varnoDependentColumns[mappingParams.name.value()] = std::to_string(mappingParams.varno);
      }
    }
  }
  // Create name mapping for varno dependent columns
  for (const detail::VarnoDependentColumnParameters &columnParams :
       layoutParams.varnoDependentColumns.value()) {
    const auto it = std::find(columns.begin(), columns.end(), columnParams.source.value());
    if (it != columns.end()) {
      for (const auto &map : columnParams.mappings.value()) {
        const auto varnoIt = std::find(listOfVarNos.begin(), listOfVarNos.end(), map.varno);
        if (varnoIt != listOfVarNos.end()) {
          const std::string ioda_variable_name = columnParams.groupName.value() +
                                                 "/" + map.name.value();
          mappings.varnoDependentColumnsNames[ioda_variable_name] = columnParams.source.value();
        }
      }
    }
  }
  return mappings;
}

struct ColumnInfo {
  std::string column_name;
  TypeClass column_type;
  int column_size;
  int string_length;
  int epoch_year;
  int epoch_month;
  int epoch_day;
  int epoch_hour;
  int epoch_minute;
  int epoch_second;
};

void pushBackVector(std::vector<std::vector<double>> &data_store,
                    const std::vector<double> &inarray,
                    const size_t numlocs, const size_t numchans) {
  if (numchans == 0) {
    ASSERT(inarray.size() == numlocs);
    data_store.push_back(inarray);
  } else if (inarray.size() == numlocs) {
    std::vector<double> data_store_tmp_chans(numlocs * numchans);
    // Copy each location value to all channels
    for (size_t j = 0; j < inarray.size(); j++)
      for (size_t i = 0; i < numchans; i++)
        data_store_tmp_chans[j * numchans + i] = inarray[j];
    data_store.push_back(data_store_tmp_chans);
  } else if (inarray.size() == numchans) {
    std::vector<double> data_store_tmp_chans(numlocs * numchans);
    // Copy each channel value to all channels
    for (size_t j = 0; j < numlocs; j++)
      for (size_t i = 0; i < numchans; i++)
        data_store_tmp_chans[j * numchans + i] = inarray[i];
    data_store.push_back(data_store_tmp_chans);
  } else if (inarray.size() == numchans * numlocs) {
    data_store.push_back(inarray);
  } else {
    oops::Log::info() << "inarray.size() = " << inarray.size() << std::endl;
    oops::Log::info() << "numlocs = " << numlocs << std::endl;
    oops::Log::info() << "numchans = " << numchans << std::endl;
    eckit::Abort("Attempt to write a vector that does not match a given size when writing "
                 "to the ODB file.  Array must be of size numlocs or numchans or numchans*numlocs");
  }
}

std::vector<int> getChannelNumbers(const Group &storageGroup) {
  TypeClass t = storageGroup.vars["Channel"].getType().getClass();
  std::vector<int> Channel;
  if (t == TypeClass::Integer) {
    storageGroup.vars["Channel"].read<int>(Channel);
  } else {
    std::vector<float> ChannelFloat;
    storageGroup.vars["Channel"].read<float>(ChannelFloat);
    for (size_t i = 0; i < ChannelFloat.size(); ++i)
      Channel.emplace_back(static_cast<int>(ChannelFloat[i]));
  }
  return Channel;
}

void setupColumnInfo(const Group &storageGroup, const std::map<std::string, std::string> &reverseColumnMap,
                     std::vector<ColumnInfo> &column_infos, int &num_columns,
                     const bool errorWithColumnNotInObsSpace) {
  const auto objs = storageGroup.listObjects(ObjectType::Variable, true);
  for (auto it = objs.cbegin(); it != objs.cend(); it++) {
    for (size_t i = 0; i < it->second.size(); i++) {
      const auto found = reverseColumnMap.find(it->second[i]);
      if (found != reverseColumnMap.end()) {
        if (!it->second[i].compare(metadata_prefix_size,it->second[i].size(),"dateTime") ||
            !it->second[i].compare(metadata_prefix_size,it->second[i].size(),"receiptdateTime")) {
          std::string datename = "date";
          std::string timename = "time";
          const std::string obsspacename = it->second[i];
          if (obsspacename == "MetaData/receiptdateTime") {
            datename = "receipt_date";
            timename = "receipt_time";
          }
          ColumnInfo date, time;
          date.column_name = datename;
          date.column_type = storageGroup.vars[obsspacename].getType().getClass();
          date.column_size = storageGroup.vars[obsspacename].getType().getSize();
          std::string epochString = storageGroup.vars[obsspacename].atts.open("units").read<std::string>();
          std::size_t pos = epochString.find("seconds since ");
          epochString = epochString.substr(pos+14);
          const int year = std::stoi(epochString.substr(0,4).c_str());
          date.epoch_year = year;
          time.epoch_year = year;
          const int month = std::stoi(epochString.substr(5,2).c_str());
          date.epoch_month = month;
          time.epoch_month = month;
          const int day = std::stoi(epochString.substr(8,2).c_str());
          date.epoch_day = day;
          time.epoch_day = day;
          const int hour = std::stoi(epochString.substr(11,2).c_str());
          date.epoch_hour = hour;
          time.epoch_hour = hour;
          const int minute = std::stoi(epochString.substr(14,2).c_str());
          date.epoch_minute = minute;
          time.epoch_minute = minute;
          const int second = std::stoi(epochString.substr(17,2).c_str());
          date.epoch_second = second;
          time.epoch_second = second;
          date.string_length = 0;
          num_columns++;
          time.column_name = timename;
          time.column_type = storageGroup.vars[obsspacename].getType().getClass();
          time.column_size = storageGroup.vars[obsspacename].getType().getSize();
          time.string_length = 0;
          num_columns++;
          column_infos.push_back(date);
          column_infos.push_back(time);
        } else {
          ColumnInfo col;
          col.column_name = it->second[i];
          col.column_type = storageGroup.vars[col.column_name].getType().getClass();
          col.column_size = storageGroup.vars[col.column_name].getType().getSize();
          if (col.column_type == TypeClass::String) {
            std::vector<std::string> buf;
            storageGroup.vars[col.column_name].read<std::string>(buf);
            size_t len = 0;
            for (size_t j = 0; j < buf.size(); j++) {
              if (buf[j].size() > len) {
                len = buf[j].size();
              }
            }
            col.string_length = len;
            num_columns += 1+((col.string_length-1)/8);
          } else {
            col.string_length = 0;
            num_columns++;
          }
          column_infos.push_back(col);
        }
      }
      if (!it->second[i].compare("Channel")) {
        ColumnInfo col;
        col.column_name = "vertco_reference_1";
        col.column_type = storageGroup.vars["Channel"].getType().getClass();
        col.column_size = storageGroup.vars["Channel"].getType().getSize();
        col.string_length = 0;
        num_columns++;
        column_infos.push_back(col);
      }
    }
  }
  // Check that map entry requested is in the ObsGroup
  for (const auto &map : reverseColumnMap) {
    if (!storageGroup.vars.exists(map.first)) {
      if (errorWithColumnNotInObsSpace) {
        throw eckit::UserError("Variable " + map.first +
                               " requested via the query file is not in the ObsSpace " +
                               "therefore aborting as requested", Here());
      } else {
        oops::Log::warning() << "WARNING: Variable " + map.first + " is in query file "
                             << "but not in ObsSpace therefore not being written out"
                             << std::endl;
      }
    } // end of if found
  } // end of map loop
  // Add the processed data column
  ColumnInfo col;
  col.column_name = "processed_data";
  col.column_type = TypeClass::Integer;
  col.column_size = 4;
  col.string_length = 0;
  num_columns++;
  column_infos.push_back(col);
}

void setupBodyColumnInfo(const Group &storageGroup, const std::map<std::string, std::string> &reverseColumnMap,
                     std::vector<ColumnInfo> &column_infos, std::vector<ColumnInfo> &column_infos_missing,
                     int &num_columns, const bool errorWithColumnNotInObsSpace) {
  std::vector<std::string> col_names;
  std::vector<std::string> obs_space_found;
  const auto objs = storageGroup.listObjects(ObjectType::Variable, true);
  for (auto it = objs.cbegin(); it != objs.cend(); it++) {
    for (size_t i = 0; i < it->second.size(); i++) {
      const auto found = reverseColumnMap.find(it->second[i]);
      if (found != reverseColumnMap.end()) {
        obs_space_found.push_back(it->second[i]);
        const auto alreadyExists = std::find(col_names.begin(), col_names.end(), found->second);
        if (alreadyExists != col_names.end()) continue;
        col_names.push_back(found->second);
        ColumnInfo col;
        col.column_name = found->second;
        const std::string obsspacename = it->second[i];
        col.column_type = storageGroup.vars[obsspacename].getType().getClass();
        col.column_size = storageGroup.vars[obsspacename].getType().getSize();
        if (col.column_type == TypeClass::String) {
          std::vector<std::string> buf;
          storageGroup.vars[obsspacename].read<std::string>(buf);
          size_t len = 0;
          for (size_t j = 0; j < buf.size(); j++) {
            if (buf[j].size() > len) {
              len = buf[j].size();
            }
          }
          col.string_length = len;
          num_columns += 1+((col.string_length-1)/8);
        } else {
          col.string_length = 0;
          num_columns++;
        }
        column_infos.push_back(col);
      }
    }
  }
  // Check that map entry requested is in the ObsGroup
  // if not create add to the missing which will get written
  // out with missing data
  for (const auto& map : reverseColumnMap) {
    const auto found = std::find(obs_space_found.begin(), obs_space_found.end(), map.first);
    if (found == obs_space_found.end()) {
      const auto alreadyListed = std::find(col_names.begin(), col_names.end(), map.second);
      // Check its a new column rather than a missing varno
      if (alreadyListed == col_names.end()) {
        ColumnInfo col;
        col.column_name = map.second;
        col.column_type = TypeClass::Float;
        col.column_size = 4;
        column_infos_missing.push_back(col);
        col_names.push_back(map.second);
      }
      if (errorWithColumnNotInObsSpace) {
        throw eckit::UserError("Variable " + map.first +
                               " requested via the query file is not in the ObsSpace " +
                               "therefore aborting as requested", Here());
      } else {
        oops::Log::warning() << "WARNING: Variable " + map.first + " is in query file "
                             << "but not in ObsSpace therefore assumming float and writing out with missing data"
                             << std::endl;
      }
    } // end of if found
  } // end of map loop
}

void setODBColumn(std::map<std::string, std::string> &columnMappings,
                  const ColumnInfo v, odc::Writer<>::iterator writer, int &column_number) {
  std::map<std::string,std::string>::iterator it2;
  std::string colname2 = "";
  for (it2 = columnMappings.begin(); it2 != columnMappings.end(); it2++) {
    if (it2->first == v.column_name) {
      colname2 = it2->second;
    }
  }
  if (colname2 == "") {
    colname2 = v.column_name;
    if (colname2.rfind(metadata_prefix, 0) == 0) {
      colname2.erase(0, metadata_prefix_size);
    }
  }
  // transform name to lower case
  std::transform(colname2.begin(), colname2.end(), colname2.begin(),
      [](unsigned char c){ return std::tolower(c); });
  if (v.column_type == TypeClass::Integer) {
    writer->setColumn(column_number, colname2, odc::api::INTEGER);
    column_number++;
  } else if (v.column_type == TypeClass::String) {
    if (v.string_length <= 8) {
      writer->setColumn(column_number, colname2, odc::api::STRING);
      column_number++;
    } else {
      for (int i = 0; i < 1+((v.string_length-1)/8); i++) {
        writer->setColumn(column_number, colname2 + std::string("_") + std::to_string(i + 1),
                          odc::api::STRING);
        column_number++;
      }
    }
  } else {
    writer->setColumn(column_number, colname2, odc::api::REAL);
    column_number++;
  }
}

void setODBBodyColumn(const ColumnInfo &v, odc::Writer<>::iterator writer, int &column_number) {
  // Column size 1 is a bool, this will be put in the odb as an integer
  if (v.column_type == TypeClass::Integer || v.column_size == 1) {
    writer->setColumn(column_number, v.column_name, odc::api::INTEGER);
    column_number++;
  } else if (v.column_type == TypeClass::String) {
    if (v.string_length <= 8) {
      writer->setColumn(column_number, v.column_name, odc::api::STRING);
      column_number++;
    } else {
      for (int i = 0; i < 1+((v.string_length-1)/8); i++) {
        writer->setColumn(column_number, v.column_name + std::string("_") + std::to_string(i + 1),
                          odc::api::STRING);
        column_number++;
      }
    }
  } else {
    writer->setColumn(column_number, v.column_name, odc::api::REAL);
    column_number++;
  }
}

void setupVarnos(const Group &storageGroup, const std::vector<int> &listOfVarNos,
                 const std::map<std::string, std::string> &mapping,
                 const bool errorWithColumnNotInObsSpace,
                 std::vector<int> &varnos,
                 std::vector<std::string> &varno_names) {
  for (const auto &map : mapping) {
    const std::string derived_obsvalue_name = std::string(derived_obsvalue_prefix) + map.first;
    const std::string obsvalue_name = std::string(obsvalue_prefix) + map.first;
    if (storageGroup.vars.exists(obsvalue_name) ||
        storageGroup.vars.exists(derived_obsvalue_name)) {
      varnos.push_back(std::stoi(map.second));
      varno_names.push_back(map.first);
    } else {
      if (errorWithColumnNotInObsSpace) {
        throw eckit::UserError("varno associated with " + map.first +
                               " requested via the query file is not in the ObsSpace " +
                               "therefore aborting as requested", Here());
      } else {
        oops::Log::warning() << "WARNING: varno associated with " + map.first + " is in query file "
                             << "but not in ObsSpace therefore not being written out"
                             << std::endl;
      }
    }
  }
}

void fillFloatArray(const Group & storageGroup, const std::string varname,
                    const int numrows, std::vector<double> & outdata, std::string odbType, std::vector<int> extendeds) {
  const bool derived_varname = varname.rfind("Derived", 0) == 0;
  const bool metadata_varname = varname.rfind("MetaData", 0) == 0;
  const bool derived_odb = odbType == "derived";
  if (storageGroup.vars.exists(varname)) {
    std::vector<float> buffer;
    storageGroup.vars[varname].read<float>(buffer);
    const ioda::Variable var = storageGroup.vars[varname];
    const float fillValue  = ioda::detail::getFillValue<float>(var.getFillValue());
    if (derived_odb) {
      if (metadata_varname) {
        for (int j = 0; j < numrows; j++) {
          if (fillValue == buffer[j]) {
            outdata[j] = odb_missing_float;
          } else {
            outdata[j] = buffer[j];
          }
        }
      } else {
        for (int j = 0; j < numrows; j++) {
          if (derived_varname && extendeds[j] == 0 || (!derived_varname && extendeds[j] == 1) || fillValue == buffer[j]) {
            outdata[j] = odb_missing_float;
          } else {
            outdata[j] = buffer[j];
          }
        }
      }
    } else {
      for (int j = 0; j < numrows; j++) {
        if (fillValue == buffer[j]) {
          outdata[j] = odb_missing_float;
        } else {
          outdata[j] = buffer[j];
        }
      }
    }
  } else {
    for (int j = 0; j < numrows; j++)
      outdata[j] = odb_missing_float;
  }
}

void fillIntArray(const Group &storageGroup, const std::string varname,
                  const int numrows, const int columnsize,
                  std::vector<double> &outdata) {
  if (storageGroup.vars.exists(varname)) {
    if (columnsize == 4) {
      std::vector<int> buf;
      storageGroup.vars[varname].read<int>(buf);
      const int fillValue = ioda::detail::getFillValue<int>(storageGroup.vars[varname].getFillValue());
      for (int j = 0; j < numrows; j++) {
        if (fillValue == buf[j]) {
          outdata[j] = odb_missing_int;
        } else {
          outdata[j] = buf[j];
        }
      }
    } else if (columnsize == 8) {
      std::vector<long> buf;
      long fillValue;
      Variable var = storageGroup.vars.open(varname);
      if (var.isA<long>()) {
        var.read<long>(buf);
        fillValue = ioda::detail::getFillValue<long>(var.getFillValue());
      } else if (var.isA<int64_t>()) {
        std::vector<int64_t> buf64;
        var.read<int64_t>(buf64);
        buf.reserve(buf64.size());
        buf.assign(buf64.begin(), buf64.end());
        fillValue = ioda::detail::getFillValue<int64_t>(var.getFillValue());
      } else {
        std::string errMsg("ODB Writer: Unrecognized data type for column size of 8");
        throw Exception(errMsg.c_str(), ioda_Here());
      }
      for (int j = 0; j < numrows; j++) {
        if (fillValue == buf[j]) {
          outdata[j] = odb_missing_int;
        } else {
          outdata[j] = buf[j];
        }
      }
    }
  } else {
    for (int j = 0; j < numrows; j++) outdata[j] = odb_missing_int;
  }
}

void readColumn(const Group &storageGroup, const ColumnInfo column, std::vector<std::vector<double>> &data_store,
                const int number_of_locations, const int number_of_channels, std::string odb_type, std::vector<int> extendeds) {
  if (column.column_name == "date" || column.column_name == "receipt_date") {
    std::string obsspacename = "MetaData/dateTime";
    if (column.column_name == "receipt_date") obsspacename = "MetaData/receiptdateTime";
    const int arraySize = storageGroup.vars[obsspacename].getDimensions().numElements;
    std::vector<double> data_store_date(arraySize);
    std::vector<int64_t> buf;    
    storageGroup.vars[obsspacename].read<int64_t>(buf);
    float fillValue = ioda::detail::getFillValue<float>(storageGroup.vars[obsspacename].getFillValue());
    for (int j = 0; j < arraySize; j++) {
      if (fillValue == buf[j]) {
        data_store_date[j] = odb_missing_float;
      } else {
        // struct tm is being used purely for time arithmetic.  The offset is incorrect but it
        // doesn't matter in this context.
        //
        // Note the struct tm type wants the year to be the number of years since 1900
        // and the month to be the number of months from January (ie, Jan to Dec -> 0 to 11)
        //
        // In order to avoid the 2038 issue (max positive value in a signed 32-bit int
        // offset in seconds referenced to Jan 1, 1970 represents a datetime in Jan 2038)
        // convert the offset in seconds to a set of offsets for seconds, minutes,
        // hours and days and add those to the repective data members of the struct tm.
        int64_t offset = buf[j];
        struct tm time = { 0 };
        time.tm_sec = column.epoch_second + (offset % 60);
        offset /= 60;
        time.tm_min = column.epoch_minute + (offset % 60);
        offset /= 60;
        time.tm_hour = column.epoch_hour + (offset % 24);
        offset /= 24;
        time.tm_mday = column.epoch_day + offset;
        time.tm_mon = column.epoch_month - 1;
        time.tm_year = column.epoch_year - 1900;
        timegm(&time);
        data_store_date[j] =
            (time.tm_year + 1900) * 10000 + (time.tm_mon + 1) * 100 + time.tm_mday;
      }
    }
    pushBackVector(data_store, data_store_date, number_of_locations, number_of_channels);
  } else if (column.column_name == "time" || column.column_name == "receipt_time") {
    std::string obsspacename = "MetaData/dateTime";
    if (column.column_name == "receipt_date") obsspacename = "MetaData/receiptdateTime";
    const int arraySize = storageGroup.vars[obsspacename].getDimensions().numElements;
    std::vector<double> data_store_time(arraySize);
    std::vector<int64_t> buf;    
    storageGroup.vars[obsspacename].read<int64_t>(buf);
    const float fillValue = ioda::detail::getFillValue<float>(storageGroup.vars[obsspacename].getFillValue());
    for (int j = 0; j < arraySize; j++) {
      if (fillValue == buf[j]) {
        data_store_time[j] = odb_missing_float;
      } else {
        // See comments above in the date section.
        int64_t offset = buf[j];
        struct tm time = { 0 };
        time.tm_sec = column.epoch_second + (offset % 60);
        offset /= 60;
        time.tm_min = column.epoch_minute + (offset % 60);
        offset /= 60;
        time.tm_hour = column.epoch_hour + (offset % 24);
        offset /= 24;
        time.tm_mday = column.epoch_day + offset;
        time.tm_mon = column.epoch_month - 1;
        time.tm_year = column.epoch_year - 1900;
        timegm(&time);
        data_store_time[j] = time.tm_hour * 10000 + time.tm_min * 100 + time.tm_sec;
      }
    }
    pushBackVector(data_store, data_store_time, number_of_locations, number_of_channels);
  } else if (column.column_name == "vertco_reference_1") {
    std::vector<int> buf = getChannelNumbers(storageGroup);
    std::vector<double> data_store_chan(number_of_locations * number_of_channels);
    for (int j = 0; j < number_of_locations; j++)
      for (int i = 0; i < number_of_channels; i++)
        data_store_chan[j * number_of_channels + i] = static_cast<double>(buf[i]);
    data_store.push_back(data_store_chan);
  } else if (column.column_name == "processed_data") {
    if (number_of_channels > 0) {
      std::vector<double> data_store_chan(number_of_locations * number_of_channels);
      for (int j = 0; j < number_of_locations; j++)
        for (int i = 0; i < number_of_channels; i++)
          data_store_chan[j * number_of_channels + i] = extendeds[j * number_of_channels + i];
      pushBackVector(data_store, data_store_chan, number_of_locations, number_of_channels);
    } else {
      std::vector<double> data_store_chan(number_of_locations);
      for (int j = 0; j < number_of_locations; j++)
          data_store_chan[j] = extendeds[j];
      pushBackVector(data_store, data_store_chan, number_of_locations, number_of_channels);
    }
  } else if (column.column_type == TypeClass::Float) {
    const int arraySize = storageGroup.vars[column.column_name].getDimensions().numElements;
    std::vector<double> data_store_float(arraySize);
    fillFloatArray(storageGroup, column.column_name,
                   arraySize, data_store_float, odb_type, extendeds);
    pushBackVector(data_store, data_store_float, number_of_locations, number_of_channels);
  } else if (column.column_type == TypeClass::Integer) {
    const int arraySize = storageGroup.vars[column.column_name].getDimensions().numElements;
    std::vector<double> data_store_int(arraySize);
    fillIntArray(storageGroup, column.column_name,
                 arraySize, column.column_size,
                 data_store_int);
    pushBackVector(data_store, data_store_int, number_of_locations, number_of_channels);
  } else if (column.column_type == TypeClass::String) {
    const int arraySize = storageGroup.vars[column.column_name].getDimensions().numElements;
    std::vector<double> data_store_string(arraySize);
    std::vector<std::string> buf;
    storageGroup.vars[column.column_name].read<std::string>(buf);
    int num_cols = 1+((column.string_length-1)/8);
    for (int c = 0; c < num_cols; c++) {
      for (int j = 0; j < arraySize; j++) {
         unsigned char uc[8];
         double dat;
         for (int k = 8 * c; k < std::min(8 * (c+1),static_cast<int>(buf[j].size())); k++) {
           uc[k-c*8] = buf[j][k];
         }
         for (int k = std::min(8 * (c+1),static_cast<int>(buf[j].size())); k < 8*(c+1); k++) {
           uc[k-c*8] = '\0';
         }
         memcpy(&dat, uc, 8);
         data_store_string[j] = dat;
      }
      pushBackVector(data_store, data_store_string, number_of_locations, number_of_channels);
    }
  } else if (column.column_type == TypeClass::Unknown) {
    const int arraySize = storageGroup.vars[column.column_name].getDimensions().numElements;
    std::vector<double> data_store_unknown(arraySize);
    for (int j = 0; j < arraySize; j++) {
      data_store_unknown[j] = -1.0;
    }
    pushBackVector(data_store, data_store_unknown, number_of_locations, number_of_channels);
  }
}

void readBodyColumns(const Group &storageGroup, const ColumnInfo &column, const std::string v,
                     const int number_of_rows,
                     const std::map<std::string, std::string> &reverseMap,
                     std::vector<std::vector<double>> &data_store, std::string odb_type, std::vector<int> extendeds) {
  // Work out the correct ObsSpace variable to read
  std::string obsspacename;
  for (auto const& x : reverseMap) {
    auto lastslash = x.first.find_last_of("/");
    std::string obsspacevar = x.first.substr(lastslash+1);
    if (obsspacevar == v && x.second == column.column_name)
        obsspacename = x.first;
  }
  // Create data_store_tmp
  std::vector<double> data_store_tmp(number_of_rows);
  if (column.column_type == TypeClass::Integer) {
      fillIntArray(storageGroup, obsspacename,
                   number_of_rows, column.column_size,
                   data_store_tmp);
  } else if (obsspacename.substr(0, obsspacename.find("/")) == "DiagnosticFlags") {
    std::vector<char> buf_char;
    storageGroup.vars[obsspacename].read<char>(buf_char);
    const ioda::Variable var = storageGroup.vars[obsspacename];
    const char fillValue = ioda::detail::getFillValue<char>(var.getFillValue());
    for (int j = 0; j < number_of_rows; j++) {
      if (fillValue == buf_char[j]) {
        data_store_tmp[j] = 0;
      } else {
        const int flag = buf_char[j] > 0;
        data_store_tmp[j] = flag;
      }
    }
  } else if (column.column_type == TypeClass::Float) {
    fillFloatArray(storageGroup, obsspacename,
                   number_of_rows, data_store_tmp, odb_type, extendeds);
  } else if (column.column_type == TypeClass::Integer) {
    fillIntArray(storageGroup, obsspacename,
                 number_of_rows, column.column_size,
                 data_store_tmp);
  } else if (column.column_type == TypeClass::String) {
    std::vector<std::string> buf;
    storageGroup.vars[obsspacename].read<std::string>(buf);
    int num_cols = 1+((column.string_length-1)/8);
    for (int c = 0; c < num_cols; c++) {
      for (int j = 0; j < number_of_rows; j++) {
         unsigned char uc[8];
         double dat;
         for (int k = 8 * c; k < std::min(8 * (c+1),static_cast<int>(buf[j].size())); k++) {
           uc[k-c*8] = buf[j][k];
         }
         for (int k = std::min(8 * (c+1),static_cast<int>(buf[j].size())); k < 8*(c+1); k++) {
           uc[k-c*8] = '\0';
         }
         memcpy(&dat, uc, 8);
         data_store_tmp[j] = dat;
      }
    }
  } else if (column.column_type == TypeClass::Unknown) {
    std::vector<double> data_store_tmp(number_of_rows);
    for (int j = 0; j < number_of_rows; j++) {
      data_store_tmp[j] = -1.0;
    }
  }
  // Push back tmp vector to input vector for output
  data_store.push_back(data_store_tmp);
}

void writeODB(const size_t num_varnos, const int number_of_rows, odc::Writer<>::iterator writer,
              const std::vector<std::vector<double>> &data_store,
              const std::vector<std::vector<std::vector<double>>> &data_body_store,
              const int num_indep, const int num_body,
              const int num_body_missing, const std::vector<int> &varnos) {
  for (int row = 0; row < number_of_rows; row++) {
    for (size_t varno = 0; varno < num_varnos; varno++) {
      int col_num = 0;
      // Varno independent variables
      for (int column = 0; column < num_indep; column++) {
        (*writer)[col_num] = data_store[column][row];
        col_num++;        
      }
      // Varno dependent variables
      if (num_varnos > 0) {
        (*writer)[col_num] = varnos[varno];
        col_num++;
        for (int column = 0; column < num_body; column++) {
          (*writer)[col_num] = data_body_store[column][varno][row];
          col_num++;
        }
      }
      // Missing varno dependent variables
      for (int column = 0; column < num_body_missing; column++) {
        (*writer)[col_num] = odb_missing_float;
        col_num++;
      }
      ++writer;
    }
  }
}

// -------------------------------------------------------------------------------------------------

#endif  // odc_FOUND

}  // namespace

Group createFile(const ODC_Parameters& odcparams, Group storageGroup) {

#if odc_FOUND
  const int number_of_locations = storageGroup.vars["Location"].getDimensions().dimsCur[0];
  std::vector<int> extendeds;
  int number_of_rows = number_of_locations;
  int number_of_channels = 0;
  if (storageGroup.vars.exists("Channel")) {
     std::vector<int> channels = getChannelNumbers(storageGroup);
     number_of_rows *= channels.size();
     number_of_channels = channels.size();
  }
  if (storageGroup.vars.exists("MetaData/extendedObsSpace")) {
    storageGroup.vars["MetaData/extendedObsSpace"].read<int>(extendeds);
  } else {
    extendeds.resize(number_of_rows, 0);
  }

  // Read in the query file
  eckit::YAMLConfiguration conf(eckit::PathName(odcparams.queryFile));
  OdbQueryParameters queryParameters;
  queryParameters.validateAndDeserialize(conf);
  ColumnSelection columnSelection;
  addQueryColumns(columnSelection, queryParameters);
  const std::vector<int> &listOfVarNos =
          queryParameters.where.value().varno.value().as<std::vector<int>>();

  // Create mapping from ObsSpace to odb name
  detail::ODBLayoutParameters layoutParams;
  layoutParams.validateAndDeserialize(
        eckit::YAMLConfiguration(eckit::PathName(odcparams.mappingFile)));
  ReverseColumnMappings columnMappings = collectReverseColumnMappings(layoutParams,
                                                                      columnSelection.columns(),
                                                                      listOfVarNos);

  // Setup the varno independent columns and vectors
  int num_varnoIndependnet_columns = 0;
  std::vector<ColumnInfo> column_infos;
  setupColumnInfo(storageGroup, columnMappings.varnoIndependentColumns,
                  column_infos, num_varnoIndependnet_columns, odcparams.missingObsSpaceVariableAbort);
  if (num_varnoIndependnet_columns == 0) return storageGroup;

  // Fill data_store with varno independent data
  // access to this store is [col][rows]
  std::vector<std::vector<double>> data_store;
  for (const auto& v: column_infos) {
    readColumn(storageGroup, v, data_store, number_of_locations, number_of_channels, odcparams.odbType, extendeds);
  }

  // Setup the varno dependent columns and vectors
  std::vector<int> varnos;
  std::vector<std::string> varno_names;
  setupVarnos(storageGroup, listOfVarNos,
              columnMappings.varnoDependentColumns,
              odcparams.missingObsSpaceVariableAbort,
              varnos, varno_names);
  std::vector<ColumnInfo> body_column_infos;
  std::vector<ColumnInfo> body_column_missing_infos;
  int num_body_columns = 0;
  setupBodyColumnInfo(storageGroup, columnMappings.varnoDependentColumnsNames,
                      body_column_infos, body_column_missing_infos, num_body_columns,
                      odcparams.missingObsSpaceVariableAbort);

  const size_t num_varnos = varnos.size();
  const int num_body_columns_missing = body_column_missing_infos.size();
  // 1 added on for varno col
  int total_num_cols = num_varnoIndependnet_columns + num_body_columns +
                       num_body_columns_missing + 1;

  // Read body columns in data store for body columns
  // access to this store is [col][varno][rows]
  std::vector<std::vector<std::vector<double>>> data_store_body;
  for (const auto& col: body_column_infos) {
    std::vector<std::vector<double>> data_tmp;
    for (const auto &varno: varno_names) {
      readBodyColumns(storageGroup, col, varno, number_of_rows,
                      columnMappings.varnoDependentColumnsNames, data_tmp, odcparams.odbType, extendeds);
    }
    data_store_body.push_back(data_tmp);
  }

  // Setup the odb writer object
  eckit::PathName p(odcparams.outputFile);
  odc::Writer<> oda(p);
  odc::Writer<>::iterator writer = oda.begin();

  // Setup to column information
  writer->setNumberOfColumns(total_num_cols);
  int column_number = 0;
  // Varno independent
  for (const auto& v: column_infos) {
    setODBColumn(columnMappings.varnoIndependentColumns, v, writer, column_number);
  }
  // Varno dependent
  writer->setColumn(column_number, "varno",  odc::api::INTEGER);
  column_number++;
  for (const auto& col: body_column_infos) {
    setODBBodyColumn(col, writer, column_number);
  }
  // Varno dependent not in the ObsSpace
  for (const auto& col: body_column_missing_infos) {
    setODBBodyColumn(col, writer, column_number);
  }
  // Write header and data to the ODB file
  writer->writeHeader();
  writeODB(num_varnos, number_of_rows, writer, data_store, data_store_body,
           num_varnoIndependnet_columns, num_body_columns,
           num_body_columns_missing, varnos);
#endif
  return storageGroup;
}

ObsGroup openFile(const ODC_Parameters& odcparams,
  Group storageGroup)
{
  // 1. Check first that the ODB engine is enabled. If the engine
  // is not enabled, then throw an exception.
#if odc_FOUND
  initODC();

  using std::set;
  using std::string;
  using std::vector;

  oops::Log::debug() << "ODC called with " << odcparams.queryFile << "  " <<
                        odcparams.mappingFile << "  " << odcparams.maxNumberChannels <<
                        std::endl;

  // 2. Extract the lists of columns and varnos to select from the query file.

  eckit::YAMLConfiguration conf(eckit::PathName(odcparams.queryFile));
  OdbQueryParameters queryParameters;
  queryParameters.validateAndDeserialize(conf);

  ColumnSelection columnSelection;
  addQueryColumns(columnSelection, queryParameters);

  // TODO(someone): Handle the case of the 'varno' option being set to ALL.
  const vector<int> &varnos = queryParameters.where.value().varno.value().as<std::vector<int>>();

  // 3. Perform the SQL query.

  DataFromSQL sql_data(odcparams.maxNumberChannels);
  {
    std::vector<std::string> columnNames = columnSelection.columns();

    // Temporary: Ensure that obsvalue, if present, is the last item. This ensures ODB
    // conversion tests produce output files with variables arranged in the same order as a previous
    // version of this code. The h5diff tool used by these tests is oddly sensitive to variable order.
    // In case of a future major change necessitating regeneration of the reference output files
    // this code can be removed.
    {
      const auto it = std::find(columnNames.begin(), columnNames.end(), "initial_obsvalue");
      if (it != columnNames.end()) {
        // Move the initial_obsvalue column to the end
        std::rotate(it, it + 1, columnNames.end());
      }
    }
    sql_data.select(columnNames,
                    odcparams.filename,
                    varnos,
                    queryParameters.where.value().query,
                    queryParameters.truncateProfilesToNumLev.value());
  }

  const size_t num_rows = sql_data.numberOfMetadataRows();
  if (num_rows == 0) return storageGroup;

  // 4. Create an ObsGroup, using the mapping file to set up the translation of ODB column names
  // to ioda variable names

  std::vector<std::string> ignores;
  ignores.push_back("Location");
  ignores.push_back("MetaData/dateTime");
  ignores.push_back("MetaData/receiptdateTime");
  // Write out MetaData/initialDateTime if 'time window extended lower bound' is non-missing.
  const util::DateTime missingDate = util::missingValue<util::DateTime>();
  const bool writeInitialDateTime =
    odcparams.timeWindowExtendedLowerBound != missingDate;
  if (writeInitialDateTime)
    ignores.push_back("MetaData/initialDateTime");
  ignores.push_back("Channel");

  // Station ID is constructed from other variables for certain observation types.
  const bool constructStationID =
    sql_data.getObsgroup() == obsgroup_sonde ||
    sql_data.getObsgroup() == obsgroup_oceansound ||
    sql_data.getObsgroup() == obsgroup_surface;
  if (constructStationID)
    ignores.push_back("MetaData/stationIdentification");

  NewDimensionScales_t vertcos = sql_data.getVertcos(varnos[0]);

  auto og = ObsGroup::generate(
    storageGroup,
    vertcos,
    detail::DataLayoutPolicy::generate(
      detail::DataLayoutPolicy::Policies::ObsGroupODB, odcparams.mappingFile, ignores));

  // 5. Determine which columns and bitfield column members are varno-dependent and which aren't
  detail::ODBLayoutParameters layoutParams;
  layoutParams.validateAndDeserialize(
        eckit::YAMLConfiguration(eckit::PathName(odcparams.mappingFile)));
  const ColumnMappings columnMappings = collectColumnMappings(layoutParams);

  // 6. Populate the ObsGroup with variables

  ioda::VariableCreationParameters params;

  // Begin with datetime variables, which are handled specially -- date and time are stored in
  // separate ODB columns, but ioda represents them in a single variable.
  {
    ioda::VariableCreationParameters params_dates = params;
    params_dates.setFillValue<int64_t>(queryParameters.variableCreation.missingInt64);
    // MetaData/dateTime
    ioda::Variable v = og.vars.createWithScales<int64_t>(
    "MetaData/dateTime", {og.vars["Location"]}, params_dates);
    v.atts.add<std::string>("units",
                            queryParameters.variableCreation.epoch);
    v.write(sql_data.getDates("date", "time",
                              getEpochAsDtime(v),
                              queryParameters.variableCreation.missingInt64,
                              odcparams.timeWindowStart,
                              odcparams.timeWindowExtendedLowerBound,
                              queryParameters.variableCreation.timeDisplacement));
    // MetaData/receiptdateTime
    v = og.vars.createWithScales<int64_t>(
    "MetaData/receiptdateTime", {og.vars["Location"]}, params_dates);
    v.atts.add<std::string>("units",
                            queryParameters.variableCreation.epoch);
    v.write(sql_data.getDates("receipt_date", "receipt_time",
                              getEpochAsDtime(v),
                              queryParameters.variableCreation.missingInt64));
    // MetaData/initialDateTime
    if (writeInitialDateTime) {
      v = og.vars.createWithScales<int64_t>
        ("MetaData/initialDateTime", {og.vars["Location"]}, params_dates);
      v.atts.add<std::string>("units",
                              queryParameters.variableCreation.epoch);
      v.write(sql_data.getDates("date", "time",
                                getEpochAsDtime(v),
                                queryParameters.variableCreation.missingInt64));
    }
  }

  if (constructStationID) {
    ioda::Variable v = og.vars.createWithScales<std::string>(
    "MetaData/stationIdentification", {og.vars["Location"]}, params);
    v.write(sql_data.getStationIDs());
  }

  for (const std::string &column : sql_data.getColumns()) {
    // Check if this column requires special treatment...
    if (column == "initial_vertco_reference" && sql_data.getObsgroup() == obsgroup_airs) {
      sql_data.assignChannelNumbers(varno_rawbt, og);
    } else if (column == "initial_vertco_reference" && (sql_data.getObsgroup() == obsgroup_iasi ||
                                                           sql_data.getObsgroup() == obsgroup_cris ||
                                                           sql_data.getObsgroup() == obsgroup_hiras)) {
      sql_data.assignChannelNumbers(varno_rawsca, og);
    } else if (column == "initial_vertco_reference" && (sql_data.getObsgroup() == obsgroup_abiclr ||
                                                        sql_data.getObsgroup() == obsgroup_ahiclr ||
                                                        sql_data.getObsgroup() == obsgroup_atms ||
                                                        sql_data.getObsgroup() == obsgroup_gmihigh ||
                                                        sql_data.getObsgroup() == obsgroup_gmilow ||
                                                        sql_data.getObsgroup() == obsgroup_mwri ||
                                                        sql_data.getObsgroup() == obsgroup_seviriclr ||
                                                        sql_data.getObsgroup() == obsgroup_amsub ||
                                                        sql_data.getObsgroup() == obsgroup_ssmis)) {
      sql_data.assignChannelNumbersSeq(std::vector<int>({varno_rawbt}), og);
    } else if (column == "initial_vertco_reference" && sql_data.getObsgroup() == obsgroup_atovs) {
      sql_data.assignChannelNumbersSeq(std::vector<int>({varno_rawbt_amsu}), og);
    } else if (column == "initial_vertco_reference" && sql_data.getObsgroup() == obsgroup_mwsfy3) {
      sql_data.assignChannelNumbersSeq(std::vector<int>({varno_rawbt_mwts,varno_rawbt_mwhs}), og);
    } else if (column == "initial_vertco_reference" && sql_data.getObsgroup() == obsgroup_amsr) {
      sql_data.assignChannelNumbersSeq(std::vector<int>({varno_rawbt,varno_rawbt_amsr_89ghz}), og);
    // For Scatwind, channels dimension is being used to store wind ambiguities
    } else if (column == "initial_vertco_reference" && sql_data.getObsgroup() == obsgroup_scatwind) {
      sql_data.assignChannelNumbersSeq(std::vector<int>({varno_dd}), og);
    // For GNSS-RO, channels dimension is being used to the observations through the profile
    } else if (column == "vertco_reference_2" && sql_data.getObsgroup() == obsgroup_gnssro) {
      sql_data.assignChannelNumbersSeq(std::vector<int>({varno_bending_angle}), og);
    // For SurfaceCloud, channels dimension is being used for layer number for cloud layers
    } else if (column == "initial_vertco_reference" && sql_data.getObsgroup() == obsgroup_surfacecloud) {
      sql_data.assignChannelNumbersSeq(std::vector<int>({varno_cloud_fraction_covered}), og);
    // When an ODB is written by IODA the Channel variable (and dimension)
    // is written to vertco_reference_1.  This if statement then reads this back in.
    } else if (column == "vertco_reference_1") {
      sql_data.assignChannelNumbers(varnos[0], og, "vertco_reference_1");
   // ... no, it does not.
    } else {      
      // This loop handles columns whose cells should be transferred in their entirety into ioda
      // variables (without splitting into bitfield members)
      const auto nonbitfieldColumnIt = columnMappings.nonbitfieldColumns.find(column);
      if (nonbitfieldColumnIt != columnMappings.nonbitfieldColumns.end()) {
        nonbitfieldColumnIt->second.createIodaVariables(sql_data, column, varnos, params, og);
      }

      // This loop handles bitfield columns whose members should be transferred into separate
      // ioda variables. Note that the mapping file may legitimately ask for a bitfield column to be
      // transferred whole into a ioda variable and in addition for some some or all of that
      // column's members to be transferred into different variables; so both loops may be entered
      // in succession.
      const auto bitfieldColumnIt = columnMappings.bitfieldColumns.find(column);
      if (bitfieldColumnIt != columnMappings.bitfieldColumns.end()) {
        const MemberSelection &memberSelection = columnSelection.columnMembers(column);
        bitfieldColumnIt->second.createIodaVariables(sql_data, column, memberSelection,
                                                     varnos, params, og);
      }
    }
  }

  og.vars.stitchComplementaryVariables();

  return og;
#else
  throw Exception(odcMissingMessage, ioda_Here());
#endif
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
