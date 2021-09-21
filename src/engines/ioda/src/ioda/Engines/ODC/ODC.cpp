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
#include <set>
#include <string>
#include <vector>
#include <algorithm>

#include "DataFromSQL.h"

#include "eckit/config/LocalConfiguration.h"
#include "eckit/config/YAMLConfiguration.h"
#include "eckit/filesystem/PathName.h"

#include "ioda/Engines/ODC.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/config.h"

#if odc_FOUND
#include "odc/api/odc.h"
#include "./DataFromSQL.h"
#include "ioda/Engines/OdbQueryParameters.h"
#endif

namespace ioda {
namespace Engines {
namespace ODC {

/// @brief Standard message when the ODC API is unavailable.
const char odcMissingMessage[] {
  "The ODB / ODC engine is disabled. odc was "
  "not found at compile time."};

/// @brief Function initializes the ODC API, just once.
void initODC() { static bool inited = false;
  if (!inited) {
#if odc_FOUND
    odc_initialise_api();
#else
    throw Exception(odcMissingMessage, ioda_Here());
#endif
    inited = true;
  }
}

ObsGroup openFile(const ODC_Parameters& odcparams,
  Group storageGroup)
{
  // Check first that the ODB engine is enabled. If the engine
  // is not enabled, then throw an exception.
#if odc_FOUND
  initODC();

  using std::set;
  using std::string;
  using std::vector;

  // Parse the query file to extract the list of columns and varnos
  eckit::YAMLConfiguration conf(eckit::PathName(odcparams.queryFile));
  OdbQueryParameters queryParameters;
  queryParameters.validateAndDeserialize(conf);

  vector<std::string> columns = {"lat", "lon", "date", "time", "seqno", "varno", "ops_obsgroup", "vertco_type", "initial_obsvalue"};
  for (const OdbVariableParameters &varParameters : queryParameters.variables.value()) {
    columns.push_back(varParameters.name);
  }
  std::sort(columns.begin(), columns.end());
  auto last = std::unique(columns.begin(), columns.end());
  columns.erase(last, columns.end());

  // TODO(someone): Handle the case of the 'varno' option being set to ALL.
  const vector<int> &varnos = queryParameters.where.value().varno.value().as<std::vector<int>>();

  DataFromSQL sql_data;
  sql_data.select(columns, odcparams.filename, varnos, queryParameters.where.value().query);

  const int num_rows = sql_data.numberOfMetadataRows();
  if (num_rows <= 0) return storageGroup;

  NewDimensionScales_t vertcos = sql_data.getVertcos();

  std::vector<std::string> ignores;
  ignores.push_back("nlocs");
  ignores.push_back("MetaData/datetime");
  ignores.push_back("MetaData/receiptdatetime");
  ignores.push_back("nchans");

  auto og = ObsGroup::generate(
    storageGroup,
    vertcos,
    detail::DataLayoutPolicy::generate(
      detail::DataLayoutPolicy::Policies::ObsGroupODB, odcparams.mappingFile, ignores));

  ioda::VariableCreationParameters params;
  // Writing in the data
  ioda::Variable v;

  // Datetime variables are handled specially -- date and time are stored in separate ODB columns,
  // but ioda represents them in a single variable.
  v = og.vars.createWithScales<std::string>(
    "MetaData/datetime", {og.vars["nlocs"]}, params);
  v.write(sql_data.getDates("date", "time"));
  v = og.vars.createWithScales<std::string>(
    "MetaData/receiptdatetime", {og.vars["nlocs"]}, params);
  v.write(sql_data.getDates("receipt_date", "receipt_time"));

  auto groups = og.listObjects();

  auto sqlColumns = sql_data.getColumns();
  for (int i = 0; i < sqlColumns.size(); i++) {
    const set<string> ignoredNames{"initial_obsvalue",
                                   "initial_vertco_reference",
                                   "date",
                                   "time",
                                   "receipt_date",
                                   "receipt_time",
                                   "seqno",
                                   "varno",
                                   "vertco_type",
                                   "entryno",
                                   "ops_obsgroup"};

    if (ignoredNames.count(sqlColumns.at(i))) continue;
    sql_data.getIodaVariable(sqlColumns.at(i), og, params);
  }

  groups = og.listObjects();
  for (int i = 0; i < varnos.size(); i++) {
    if (sql_data.getObsgroup() == obsgroup_amsr) {
      if (varnos.at(i) == varno_rawbt) {
        sql_data.getIodaObsvalue(varnos.at(i), og, params);
      }
    } else if (sql_data.getObsgroup() == obsgroup_mwsfy3) {
      if (varnos.at(i) == varno_rawbt_mwts) {
        sql_data.getIodaObsvalue(varnos.at(i), og, params);
      }
    } else {
      sql_data.getIodaObsvalue(varnos.at(i), og, params);
    }
  }

  return og;
#else
  throw Exception(odcMissingMessage, ioda_Here());
#endif
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
