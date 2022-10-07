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
#include <vector>

#include "DataFromSQL.h"

#include "ioda/Engines/ODC.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/config.h"  // Auto-generated. Defines *_FOUND.
#include "oops/util/Logger.h"

#if odc_FOUND && eckit_FOUND && oops_FOUND
# include "eckit/config/LocalConfiguration.h"
# include "eckit/config/YAMLConfiguration.h"
# include "eckit/filesystem/PathName.h"
# include "odc/api/odc.h"
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

// -------------------------------------------------------------------------------------------------

#endif  // odc_FOUND && eckit_FOUND && oops_FOUND

}  // namespace

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

    // Temporary: Ensure that initial_obsvalue, if present, is the last item. This ensures ODB
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
  ignores.push_back("nlocs");
  ignores.push_back("MetaData/dateTime");
  ignores.push_back("MetaData/receiptdateTime");
  ignores.push_back("nchans");

  // Station ID is constructed from other variables for certain observation types.
  const bool constructStationID =
    sql_data.getObsgroup() == obsgroup_sonde ||
    sql_data.getObsgroup() == obsgroup_oceansound;
  if (constructStationID)
    ignores.push_back("MetaData/station_id");

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
    "MetaData/dateTime", {og.vars["nlocs"]}, params_dates);
    v.atts.add<std::string>("units",
                            queryParameters.variableCreation.epoch);
    v.write(sql_data.getDates("date", "time",
                              getEpochAsDtime(v),
                              queryParameters.variableCreation.missingInt64));
    // MetaData/receiptdateTime
    v = og.vars.createWithScales<int64_t>(
    "MetaData/receiptdateTime", {og.vars["nlocs"]}, params_dates);
    v.atts.add<std::string>("units",
                            queryParameters.variableCreation.epoch);
    v.write(sql_data.getDates("receipt_date", "receipt_time",
                              getEpochAsDtime(v),
                              queryParameters.variableCreation.missingInt64));
  }

  if (constructStationID) {
    ioda::Variable v = og.vars.createWithScales<std::string>(
    "MetaData/station_id", {og.vars["nlocs"]}, params);
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
