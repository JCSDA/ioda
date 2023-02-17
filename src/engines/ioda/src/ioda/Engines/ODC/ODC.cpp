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

#if odc_FOUND && eckit_FOUND && oops_FOUND
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

/// @brief Standard message when the ODC API is unavailable.
const char odcMissingMessage[] {
  "The ODB / ODC engine is disabled. Either odc, eckit, or oops were "
  "not found at compile time."};

/// @brief Function initializes the ODC API, just once.
void initODC() { static bool inited = false;
  if (!inited) {
#if odc_FOUND && eckit_FOUND && oops_FOUND
    odc_initialise_api();
#else
    throw Exception(odcMissingMessage, ioda_Here());
#endif
    inited = true;
  }
}

#if odc_FOUND && eckit_FOUND && oops_FOUND

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

/// Select columns that are required regardless of whether they are included in the query or not.
void addMandatoryColumns(ColumnSelection &selection) {
  for (const char* column :
       {"lat", "lon", "date", "time", "seqno", "varno", "ops_obsgroup", "vertco_type"}) {
    selection.addColumn(column);
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
      sqlData.createVarnoIndependentIodaVariable(column, og, creationParams);
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
  std::map<std::string, std::string> nonbitfieldColumns;
};

/// Parse the mapping file and return an object
/// * listing columns and bitfield column members for which a mapping to ioda variables has
///   been defined,
/// * indicating which of them should be treated as varno-dependent, and
/// * listing the varnos for which a mapping of each varno-dependent column or column member has
///   been defined.
ReverseColumnMappings collectReverseColumnMappings(const detail::ODBLayoutParameters &layoutParams) {
  ReverseColumnMappings mappings;

  // Process varno-independent columns
  for (const detail::VariableParameters &columnParams : layoutParams.variables.value()) {
    mappings.nonbitfieldColumns[columnParams.name.value()] = columnParams.source.value();
  }
  for (const detail::VarnoDependentColumnParameters &columnParams :
       layoutParams.varnoDependentColumns.value()) {
    if (columnParams.source.value() == std::string("initial_obsvalue")) {
      for (const auto &mappingParams : columnParams.mappings.value()) {
        mappings.nonbitfieldColumns[mappingParams.name.value()] = std::to_string(mappingParams.varno);
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

void readColumn(Group storageGroup, const ColumnInfo column, std::vector<std::vector<double>> &data_store, int number_of_rows) {
  if (column.column_name == "date") {
    std::vector<int64_t> buf;
    std::vector<double> data_store_tmp(number_of_rows);
    storageGroup.vars["MetaData/dateTime"].read<int64_t>(buf);
    float ff = ioda::detail::getFillValue<float>(storageGroup.vars["MetaData/dateTime"].getFillValue());
    for (int j = 0; j < number_of_rows; j++) {
      if (ff == buf[j]) {
        data_store_tmp[j] = odb_missing_float;
      } else {
        // struct tm is being used purely for time arithmetic.  The offset is incorrect but it
        // doesn't matter in this context.
        struct tm time = { 0 };
        time.tm_year = column.epoch_year;
        time.tm_mon = column.epoch_month;
        time.tm_mday = column.epoch_day;
        time.tm_hour = column.epoch_hour;
        time.tm_min = column.epoch_minute;
        time.tm_sec = column.epoch_second + static_cast<int>(buf[j]);
        timegm(&time);
        data_store_tmp[j] = time.tm_year * 10000 + time.tm_mon * 100 + time.tm_mday;
      }
    }
    data_store.push_back(data_store_tmp);
  } else if (column.column_name == "time") {
    std::vector<int64_t> buf;
    std::vector<double> data_store_tmp(number_of_rows);
    storageGroup.vars["MetaData/dateTime"].read<int64_t>(buf);
    float ff = ioda::detail::getFillValue<float>(storageGroup.vars["MetaData/dateTime"].getFillValue());
    for (int j = 0; j < number_of_rows; j++) {
      if (ff == buf[j]) {
        data_store_tmp[j] = odb_missing_float;
      } else {
        // struct tm is being used purely for time arithmetic.  The offset is incorrect but it
        // doesn't matter in this context.
        struct tm time = { 0 };
        time.tm_year = column.epoch_year;
        time.tm_mon = column.epoch_month;
        time.tm_mday = column.epoch_day;
        time.tm_hour = column.epoch_hour;
        time.tm_min = column.epoch_minute;
        time.tm_sec = column.epoch_second + static_cast<int>(buf[j]);
        timegm(&time);
        data_store_tmp[j] = time.tm_hour * 10000 + time.tm_min * 100 + time.tm_sec;
      }
    }
    data_store.push_back(data_store_tmp);
  } else if (column.column_type == TypeClass::Float) {
    std::vector<float> buf;
    std::vector<double> data_store_tmp(number_of_rows);
    storageGroup.vars[column.column_name].read<float>(buf);
    float ff = ioda::detail::getFillValue<float>(storageGroup.vars[column.column_name].getFillValue());
    for (int j = 0; j < number_of_rows; j++) {
      if (ff == buf[j]) {
        data_store_tmp[j] = odb_missing_float;
      } else {
        data_store_tmp[j] = buf[j];
      }
    }
    data_store.push_back(data_store_tmp);
  } else if (column.column_type == TypeClass::Integer) {
    if (column.column_size == 4) {
      std::vector<int> buf;
      std::vector<double> data_store_tmp(number_of_rows);
      storageGroup.vars[column.column_name].read<int>(buf);
      int ff = ioda::detail::getFillValue<int>(storageGroup.vars[column.column_name].getFillValue());
      for (int j = 0; j < number_of_rows; j++) {
        if (ff == buf[j]) {
          data_store_tmp[j] = odb_missing_int;
        } else {
          data_store_tmp[j] = buf[j];
        }
      }
      data_store.push_back(data_store_tmp);
    } else if (column.column_size == 8) {
      std::vector<long> buf;
      std::vector<double> data_store_tmp(number_of_rows);
      long ff;
      Variable var = storageGroup.vars.open(column.column_name);
      if (var.isA<long>()) {
        var.read<long>(buf);
        ff = ioda::detail::getFillValue<long>(var.getFillValue());
      } else if (var.isA<int64_t>()) {
        std::vector<int64_t> buf64;
        var.read<int64_t>(buf64);
        buf.reserve(buf64.size());
        buf.assign(buf64.begin(), buf64.end());
        ff = ioda::detail::getFillValue<int64_t>(var.getFillValue());
      } else {
        std::string errMsg("ODB Writer: Unrecognized data type for column size of 8");
        throw Exception(errMsg.c_str(), ioda_Here());
      }
      for (int j = 0; j < number_of_rows; j++) {
        if (ff == buf[j]) {
          data_store_tmp[j] = odb_missing_int;
        } else {
          data_store_tmp[j] = buf[j];
        }
      }
      data_store.push_back(data_store_tmp);
    }
  } else if (column.column_type == TypeClass::String) {
    std::vector<std::string> buf;
    storageGroup.vars[column.column_name].read<std::string>(buf);
    int num_cols = 1+((column.string_length-1)/8);
    for (int c = 0; c < num_cols; c++) {
      std::vector<double> data_store_tmp(number_of_rows);
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
      data_store.push_back(data_store_tmp);
    }
  } else if (column.column_type == TypeClass::Unknown) {
    std::vector<double> data_store_tmp(number_of_rows);
    for (int j = 0; j < number_of_rows; j++) {
      data_store_tmp[j] = -1.0;
    }
    data_store.push_back(data_store_tmp);
  }
}

int getNumberOfRows(Group storageGroup) {
  TypeClass t = storageGroup.vars["Location"].getType().getClass();
  if (t == TypeClass::Integer) {
    std::vector<int> Location;
    storageGroup.vars["Location"].read<int>(Location);
    return Location.size();
  } else {
    std::vector<float> Location;
    storageGroup.vars["Location"].read<float>(Location);
    return Location.size();
  }
}

void setupColumnInfo(Group storageGroup, std::vector<ColumnInfo> &column_infos, int number_of_rows, int &num_columns) {
  const auto objs = storageGroup.listObjects(ObjectType::Variable, true);
  for (auto it = objs.cbegin(); it != objs.cend(); it++) {
    for (int i = 0; i < it->second.size(); i++) {
      if (!it->second[i].compare(0, metadata_prefix_size, metadata_prefix)) {
        if (!it->second[i].compare(metadata_prefix_size,it->second[i].size(),"dateTime")) {
          ColumnInfo date, time;
          date.column_name = "date";
          date.column_type = storageGroup.vars["MetaData/dateTime"].getType().getClass();
          date.column_size = storageGroup.vars["MetaData/dateTime"].getType().getSize();
          std::string epochString = storageGroup.vars["MetaData/dateTime"].atts.open("units").read<std::string>();
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
          time.column_name = "time";
          time.column_type = storageGroup.vars["MetaData/dateTime"].getType().getClass();
          time.column_size = storageGroup.vars["MetaData/dateTime"].getType().getSize();
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
            int len = 0;
            for (int j = 0; j < number_of_rows; j++) {
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
  }
}

void setODBColumn(ReverseColumnMappings columnMappings, const ColumnInfo v, odc::Writer<>::iterator writer, int &column_number) {
  std::map<std::string,std::string>::iterator it2;
  std::string colname2 = "";
  for (it2 = columnMappings.nonbitfieldColumns.begin(); it2 != columnMappings.nonbitfieldColumns.end(); it2++) {
    if (it2->first == v.column_name) {
      colname2 = it2->second;
    }
  }
  if (colname2 == "") {
    std::string colname3 = v.column_name;
    if (colname3 != "date" && colname3 != "time") {
      colname3.erase(0, metadata_prefix_size);
    }
    if (v.column_type == TypeClass::Integer) {
      writer->setColumn(column_number, colname3, odc::api::INTEGER);
      column_number++;
    } else if (v.column_type == TypeClass::String) {
      if (v.string_length <= 8) {
        writer->setColumn(column_number, colname3 + std::string("_0"), odc::api::STRING);
        column_number++;
      } else {
        for (int i = 0; i < 1+((v.string_length-1)/8); i++) {
          writer->setColumn(column_number, colname3 + std::string("_") + std::to_string(i), odc::api::STRING);
          column_number++;
        }
      }
    } else {
      writer->setColumn(column_number, colname3, odc::api::REAL);
      column_number++;
    }
  } else {
    if (v.column_type == TypeClass::Integer) {
      writer->setColumn(column_number, colname2, odc::api::INTEGER);
      column_number++;
    } else if (v.column_type == TypeClass::String) {
      if (v.string_length <= 8) {
        writer->setColumn(column_number, colname2 + std::string("_0"), odc::api::STRING);
        column_number++;
      } else {
        for (int i = 0; i < 1+((v.string_length-1)/8); i++) {
          writer->setColumn(column_number, colname2 + std::string("_") + std::to_string(i), odc::api::STRING);
          column_number++;
        }
      }
    } else {
      writer->setColumn(column_number, colname2, odc::api::REAL);
      column_number++;
    }
  }
}

void setupVarnos(Group storageGroup, std::vector<std::string> &obsvalue_columns, ReverseColumnMappings columnMappings, size_t &num_varnos, std::vector<int> &varnos) {
  const auto objs = storageGroup.listObjects(ObjectType::Variable, true);
  for (auto it = objs.cbegin(); it != objs.cend(); it++) {
    for (int i = 0; i < it->second.size(); i++) {
      std::string s = it->second[i];
      if (!s.compare(0, obsvalue_prefix_size, obsvalue_prefix)) {
        std::map<std::string,std::string>::iterator it2;
        std::string colname = s;
        colname.erase(0, obsvalue_prefix_size);
        obsvalue_columns.push_back(colname);
        int colname2;
        for (it2 = columnMappings.nonbitfieldColumns.begin(); it2 != columnMappings.nonbitfieldColumns.end(); it2++) {
          if (it2->first == colname) {
            colname2 = std::stoi(it2->second);
            num_varnos++;
            varnos.push_back(colname2);
          }
        }
      }
    }
  }
}

void readBodyColumns(Group storageGroup, const std::string v, int number_of_rows, std::vector<std::vector<double>> &obsvalue_store, std::vector<std::vector<double>> &effective_error_store, std::vector<std::vector<int>> &diagnosticflags_store, std::vector<std::vector<double>> &initial_obsvalue_store, std::vector<std::vector<int>> &effective_qc, std::vector<std::vector<double>> &obserror_store, std::vector<std::vector<double>> &derived_obserror_store, std::vector<std::vector<double>> &hofx_store, std::vector<std::vector<double>> &obsbias_store) {
    std::string effective_error_name = std::string(effective_error_prefix) + v;
    std::string obserror_name = std::string(obserror_prefix) + v;
    std::string derived_obserror_name = std::string(derived_obserror_prefix) + v;
    std::string hofx_name = std::string(hofx_prefix) + v;
    std::string obsbias_name = std::string(obsbias_prefix) + v;
    std::string qc_name = std::string(qc_prefix) + v;
    std::string derived_obsvalue_name;
    std::string initial_obsvalue_name;
    if (storageGroup.vars.exists(std::string(derived_obsvalue_prefix)+v)) {
      derived_obsvalue_name = std::string(derived_obsvalue_prefix) + v;
      initial_obsvalue_name = std::string(obsvalue_prefix) + v;
    } else {
      derived_obsvalue_name = std::string(obsvalue_prefix) + v;
      initial_obsvalue_name = std::string("");
    }
    std::vector<std::string> flags{"BackPerfFlag","BackRejectFlag","BuddyPerfFlag","BuddyRejectFlag","ClimPerfFlag","ClimRejectFlag","ConsistencyFlag","DataCorrectFlag","ExtremeValueFlag","FinalRejectFlag","NoAssimFlag","PermCorrectFlag","PermRejectFlag"};
    TypeClass t = storageGroup.vars[derived_obsvalue_name].getType().getClass();
    if (t == TypeClass::Float) {
      std::vector<float> buf;
      std::vector<int> buf_int;
      std::vector<char> buf_char;
      std::vector<double> obsvalue_store_tmp(number_of_rows);
      std::vector<double> initial_obsvalue_store_tmp(number_of_rows);
      std::vector<double> effective_error_store_tmp(number_of_rows);
      std::vector<double> hofx_store_tmp(number_of_rows);
      std::vector<double> obsbias_store_tmp(number_of_rows);
      std::vector<double> obserror_store_tmp(number_of_rows);
      std::vector<double> derived_obserror_store_tmp(number_of_rows);
      std::vector<int> diagnosticflags_store_tmp(number_of_rows);
      std::vector<int> effective_qc_tmp(number_of_rows);
      storageGroup.vars[derived_obsvalue_name].read<float>(buf);
      ioda::Variable var = storageGroup.vars[derived_obsvalue_name];
      float ff = ioda::detail::getFillValue<float>(var.getFillValue());
      for (int j = 0; j < number_of_rows; j++) {
        if (ff == buf[j]) {
          obsvalue_store_tmp[j] = odb_missing_float;
        } else {
          obsvalue_store_tmp[j] = buf[j];
        }
      }
      if (storageGroup.vars.exists(initial_obsvalue_name)) {
        storageGroup.vars[initial_obsvalue_name].read<float>(buf);
        var = storageGroup.vars[initial_obsvalue_name];
        ff = ioda::detail::getFillValue<float>(var.getFillValue());
        for (int j = 0; j < number_of_rows; j++) {
          if (ff == buf[j]) {
            initial_obsvalue_store_tmp[j] = odb_missing_float;
          } else {
            initial_obsvalue_store_tmp[j] = buf[j];
          }
        }
      } else {
        for (int j = 0; j < number_of_rows; j++) {
          initial_obsvalue_store_tmp[j] = odb_missing_float;
        }
      }
      if (storageGroup.vars.exists(effective_error_name)) {
        storageGroup.vars[effective_error_name].read<float>(buf);
        var = storageGroup.vars[effective_error_name];
        ff = ioda::detail::getFillValue<float>(var.getFillValue());
        for (int j = 0; j < number_of_rows; j++) {
          if (ff == buf[j]) {
            effective_error_store_tmp[j] = odb_missing_float;
          } else {
            effective_error_store_tmp[j] = buf[j];
          }
        }
      } else {
        for (int j = 0; j < number_of_rows; j++) {
          effective_error_store_tmp[j] = odb_missing_float;
        }
      }
      if (storageGroup.vars.exists(hofx_name)) {
        storageGroup.vars[hofx_name].read<float>(buf);
        var = storageGroup.vars[hofx_name];
        ff = ioda::detail::getFillValue<float>(var.getFillValue());
        for (int j = 0; j < number_of_rows; j++) {
          if (ff == buf[j]) {
            hofx_store_tmp[j] = odb_missing_float;
          } else {
            hofx_store_tmp[j] = buf[j];
          }
        }
      } else {
        for (int j = 0; j < number_of_rows; j++) {
          hofx_store_tmp[j] = odb_missing_float;
        }
      }
      if (storageGroup.vars.exists(obsbias_name)) {
        storageGroup.vars[obsbias_name].read<float>(buf);
        var = storageGroup.vars[obsbias_name];
        ff = ioda::detail::getFillValue<float>(var.getFillValue());
        for (int j = 0; j < number_of_rows; j++) {
          if (ff == buf[j]) {
            obsbias_store_tmp[j] = odb_missing_float;
          } else {
            obsbias_store_tmp[j] = buf[j];
          }
        }
      } else {
        for (int j = 0; j < number_of_rows; j++) {
          obsbias_store_tmp[j] = odb_missing_float;
        }
      }
      if (storageGroup.vars.exists(obserror_name)) {
        storageGroup.vars[obserror_name].read<float>(buf);
        var = storageGroup.vars[obserror_name];
        ff = ioda::detail::getFillValue<float>(var.getFillValue());
        for (int j = 0; j < number_of_rows; j++) {
          if (ff == buf[j]) {
            obserror_store_tmp[j] = odb_missing_float;
          } else {
            obserror_store_tmp[j] = buf[j];
          }
        }
      } else {
        for (int j = 0; j < number_of_rows; j++) {
          obserror_store_tmp[j] = odb_missing_float;
        }
      }
      if (storageGroup.vars.exists(derived_obserror_name)) {
        storageGroup.vars[derived_obserror_name].read<float>(buf);
        var = storageGroup.vars[derived_obserror_name];
        ff = ioda::detail::getFillValue<float>(var.getFillValue());
        for (int j = 0; j < number_of_rows; j++) {
          if (ff == buf[j]) {
            derived_obserror_store_tmp[j] = odb_missing_float;
          } else {
            derived_obserror_store_tmp[j] = buf[j];
          }
        }
      } else {
        for (int j = 0; j < number_of_rows; j++) {
          derived_obserror_store_tmp[j] = odb_missing_float;
        }
      }
      if (storageGroup.vars.exists(qc_name)) {
        storageGroup.vars[qc_name].read<int>(buf_int);
        var = storageGroup.vars[qc_name];
        ff = ioda::detail::getFillValue<int>(var.getFillValue());
        for (int j = 0; j < number_of_rows; j++) {
          if (ff == buf_int[j]) {
            effective_qc_tmp[j] = odb_missing_int;
          } else {
            effective_qc_tmp[j] = buf_int[j];
          }
        }
      } else {
        for (int j = 0; j < number_of_rows; j++) {
          effective_qc_tmp[j] = odb_missing_int;
        }
      }
      bool first_flag = true;
      for (int ii = 0; ii < flags.size(); ii++) {
        std::string diagnosticflags_name = std::string("DiagnosticFlags/") + flags[ii] + "/" + v;
        if (first_flag) {
          if (storageGroup.vars.exists(diagnosticflags_name)) {
            storageGroup.vars[diagnosticflags_name].read<char>(buf_char);
            var = storageGroup.vars[diagnosticflags_name];
            char ff = ioda::detail::getFillValue<char>(var.getFillValue());
            for (int j = 0; j < number_of_rows; j++) {
              if (ff == buf_char[j]) {
                diagnosticflags_store_tmp[j] = 0;
              } else {
                int flag = 0;
                if (buf_char[j] > 0) flag = flag | 1;
                diagnosticflags_store_tmp[j] = flag;
              }
            }
          } else {
            for (int j = 0; j < number_of_rows; j++) {
              diagnosticflags_store_tmp[j] = 0;
            }
          }
        } else {
          if (storageGroup.vars.exists(diagnosticflags_name)) {
            storageGroup.vars[diagnosticflags_name].read<char>(buf_char);
            var = storageGroup.vars[diagnosticflags_name];
            char ff = ioda::detail::getFillValue<char>(var.getFillValue());
            for (int j = 0; j < number_of_rows; j++) {
              if (ff != buf_char[j]) {
                if (buf_char[j] > 0) diagnosticflags_store_tmp[j] = diagnosticflags_store_tmp[j] | (ii+1);
              }
            }
          }
        }
        first_flag = false;
      }
      obsvalue_store.push_back(obsvalue_store_tmp);
      if (storageGroup.vars.exists(initial_obsvalue_name)) {
        initial_obsvalue_store.push_back(initial_obsvalue_store_tmp);
      } else {
        std::vector<double> tmp;
        initial_obsvalue_store.push_back(tmp);
      }
      effective_error_store.push_back(effective_error_store_tmp);
      hofx_store.push_back(hofx_store_tmp);
      obsbias_store.push_back(obsbias_store_tmp);
      obserror_store.push_back(obserror_store_tmp);
      derived_obserror_store.push_back(derived_obserror_store_tmp);
      diagnosticflags_store.push_back(diagnosticflags_store_tmp);
      effective_qc.push_back(effective_qc_tmp);
    }
}

void writeODB(size_t num_varnos, int number_of_rows, odc::Writer<>::iterator writer, std::vector<std::vector<double>> &data_store, std::vector<std::vector<double>> &obsvalue_store, std::vector<std::vector<double>> &effective_error_store, std::vector<std::vector<int>> &diagnosticflags_store, std::vector<std::vector<double>> &initial_obsvalue_store, std::vector<std::vector<int>> &effective_qc, std::vector<std::vector<double>> &obserror_store, std::vector<std::vector<double>> &derived_obserror_store, std::vector<std::vector<double>> &hofx_store, std::vector<std::vector<double>> &obsbias_store, int num_columns, std::vector<int> varnos) {
  for (int varno = 0; varno < num_varnos; varno++) {
    for (int row = 0; row < number_of_rows; row++) {
      for (int column = 0; column < num_columns-10; column++) {
        (*writer)[column] = data_store[column][row];
      }
      (*writer)[num_columns-1] = obsbias_store[varno][row];
      (*writer)[num_columns-2] = hofx_store[varno][row];
      (*writer)[num_columns-7] = effective_error_store[varno][row];
      (*writer)[num_columns-6] = obserror_store[varno][row];
      (*writer)[num_columns-5] = derived_obserror_store[varno][row];
      (*writer)[num_columns-8] = obsvalue_store[varno][row];
      if (initial_obsvalue_store[varno].size() > 0) (*writer)[num_columns-9] = initial_obsvalue_store[varno][row];
      if (num_varnos > 0) (*writer)[num_columns-10] = varnos[varno];
      (*writer)[num_columns-4] = diagnosticflags_store[varno][row];
      (*writer)[num_columns-3] = effective_qc[varno][row];
      ++writer;
    }
  }
}

// -------------------------------------------------------------------------------------------------

#endif  // odc_FOUND && eckit_FOUND && oops_FOUND

}  // namespace

Group createFile(const ODC_Parameters& odcparams,Group storageGroup) {

#if odc_FOUND && oops_FOUND && eckit_FOUND
  std::vector<ColumnInfo> column_infos;

  int number_of_rows = getNumberOfRows(storageGroup);
  int num_columns = 0;
  setupColumnInfo(storageGroup, column_infos, number_of_rows, num_columns);
  if (num_columns == 0) return storageGroup;
  num_columns +=10;
  detail::ODBLayoutParameters layoutParams;
  layoutParams.validateAndDeserialize(
        eckit::YAMLConfiguration(eckit::PathName(odcparams.mappingFile)));
  ReverseColumnMappings columnMappings = collectReverseColumnMappings(layoutParams);
  eckit::PathName p(odcparams.outputFile);
  odc::Writer<> oda(p);
  odc::Writer<>::iterator writer = oda.begin();
  writer->setNumberOfColumns(num_columns);

  int column_number = 0;
  for (const auto& v: column_infos) {
    setODBColumn(columnMappings, v, writer, column_number);
  }
  writer->setColumn(column_number, "varno", odc::api::INTEGER);
  writer->setColumn(column_number+1, "initial_obsvalue", odc::api::REAL);
  writer->setColumn(column_number+2, "obsvalue", odc::api::REAL);
  writer->setColumn(column_number+3, "effective_error", odc::api::REAL);
  writer->setColumn(column_number+4, "obs_error", odc::api::REAL);
  writer->setColumn(column_number+5, "derived_obs_error", odc::api::REAL);
  writer->setColumn(column_number+6, "datum_flags", odc::api::INTEGER);
  writer->setColumn(column_number+7, "effective_qc", odc::api::INTEGER);
  writer->setColumn(column_number+8, "bgvalue", odc::api::REAL);
  writer->setColumn(column_number+9, "obsbias", odc::api::REAL);
  std::vector<std::vector<double>> data_store;
  writer->writeHeader();
  for (const auto& v: column_infos) {
    readColumn(storageGroup, v, data_store, number_of_rows);
  }

  size_t num_varnos = 0;
  std::vector<int> varnos;
  std::vector<std::string> obsvalue_columns;
  setupVarnos(storageGroup, obsvalue_columns, columnMappings, num_varnos, varnos);
  std::vector<std::vector<double>> obsvalue_store;
  std::vector<std::vector<double>> initial_obsvalue_store;
  std::vector<std::vector<double>> effective_error_store;
  std::vector<std::vector<double>> hofx_store;
  std::vector<std::vector<double>> obsbias_store;
  std::vector<std::vector<double>> obserror_store;
  std::vector<std::vector<double>> derived_obserror_store;
  std::vector<std::vector<int>> diagnosticflags_store;
  std::vector<std::vector<int>> effective_qc;
  for (const auto& v: obsvalue_columns) {
    readBodyColumns(storageGroup, v, number_of_rows, obsvalue_store, effective_error_store, diagnosticflags_store, initial_obsvalue_store, effective_qc, obserror_store, derived_obserror_store, hofx_store, obsbias_store);
  }
  writeODB(num_varnos, number_of_rows, writer, data_store, obsvalue_store, effective_error_store, diagnosticflags_store, initial_obsvalue_store, effective_qc, obserror_store, derived_obserror_store, hofx_store, obsbias_store, num_columns, varnos);
#endif
  return storageGroup;
}

ObsGroup openFile(const ODC_Parameters& odcparams,
  Group storageGroup)
{
  // 1. Check first that the ODB engine is enabled. If the engine
  // is not enabled, then throw an exception.
#if odc_FOUND && oops_FOUND && eckit_FOUND
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
  addMandatoryColumns(columnSelection);

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
  const util::DateTime missingDate = util::missingValue(missingDate);
  const bool writeInitialDateTime =
    odcparams.timeWindowExtendedLowerBound != missingDate;
  if (writeInitialDateTime)
    ignores.push_back("MetaData/initialDateTime");
  ignores.push_back("Channel");

  // Station ID is constructed from other variables for certain observation types.
  const bool constructStationID =
    sql_data.getObsgroup() == obsgroup_sonde ||
    sql_data.getObsgroup() == obsgroup_oceansound;
  if (constructStationID)
    ignores.push_back("MetaData/stationIdentification");

  NewDimensionScales_t vertcos = sql_data.getVertcos();

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
