/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** @file DataFromSQL.cpp
 * @brief implements ODC bindings
**/

#include <fstream>
#include <algorithm>

#include "./DataFromSQL.h"

#include "odc/Select.h"
#include "odc/api/odc.h"

#include "oops/util/Duration.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {

template <class T>
constexpr T odb_missing();
template <>
constexpr float odb_missing<float>() { return odb_missing_float; }
template <>
constexpr int odb_missing<int>() { return odb_missing_int; }

}  // namespace

DataFromSQL::DataFromSQL() {}

size_t DataFromSQL::numberOfMetadataRows() const { return number_of_metadata_rows_; }

int DataFromSQL::getColumnIndex(const std::string& col) const {
  for (int i = 0; i < columns_.size(); i++) {
    if (columns_.at(i) == col) {
      return i;
    }
  }
  return -1;
}

size_t DataFromSQL::numberOfRowsForVarno(const int varno) const {
  int varno_index = getColumnIndex("varno");
  size_t tot      = 0;
  for (int i = 0; i < number_of_rows_; i++) {
    size_t val = getData(i, varno_index);
    if (val == varno) {
      tot++;
    }
  }
  return tot;
}

bool DataFromSQL::hasVarno(const int varno) const {
  for (int i = 0; i < number_of_varnos_; i++) {
    if (varno == varnos_.at(i)) return true;
  }
  return false;
}

size_t DataFromSQL::numberOfLevels(const int varno) const {
  if (hasVarno(varno) && number_of_metadata_rows_ > 0) {
    return varnos_and_levels_.at(varno);
  }
  return 0;
}

void DataFromSQL::setData(const std::string& sql) {
  odc::Select sodb(sql);
  odc::Select::iterator begin = sodb.begin();
  const size_t number_of_columns = columns_.size();
  ASSERT(begin->columns().size() == number_of_columns);

  // Determine column types and bitfield definitions
  column_types_.clear();
  for (const odc::core::Column *column : begin->columns()) {
    column_types_.push_back(column->type());

    Bitfield bitfield;
    const eckit::sql::BitfieldDef &bitfieldDef = column->bitfieldDef();
    const eckit::sql::FieldNames &fieldNames = bitfieldDef.first;
    const eckit::sql::Sizes &sizes = bitfieldDef.second;
    ASSERT(fieldNames.size() == sizes.size());
    std::int32_t pos = 0;
    for (size_t i = 0; i < fieldNames.size(); ++i) {
      bitfield.push_back({fieldNames[i], pos, sizes[i]});
      pos += sizes[i];
    }
    column_bitfield_defs_.push_back(std::move(bitfield));
  }

  // Retrieve data
  data_.clear();
  data_.resize(number_of_columns);
  for (auto &row : sodb) {
    ASSERT(row.columns().size() == number_of_columns);
    for (size_t i = 0; i < number_of_columns; i++) {
      appendData(i, row[i]);
    }
  }

  // Free unused memory
  for (auto &column : data_) {
    column.shrink_to_fit();
  }
}

void DataFromSQL::appendData(size_t column, double value) {
  data_.at(column).push_back(value);
}

int DataFromSQL::getColumnTypeByName(std::string const& column) const {
  return column_types_.at(getColumnIndex(column));
}

NewDimensionScales_t DataFromSQL::getVertcos() const {
  NewDimensionScales_t vertcos;
  const int num_rows = number_of_metadata_rows_;
  vertcos.push_back(
    NewDimensionScale<int>("nlocs", num_rows, num_rows, num_rows));
  if (obsgroup_ == obsgroup_iasi || obsgroup_ == obsgroup_cris || obsgroup_ == obsgroup_hiras) {
    int number_of_levels = numberOfLevels(varno_rawsca);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  } else if (obsgroup_ == obsgroup_atovs) {
    int number_of_levels = numberOfLevels(varno_rawbt_amsu);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  } else if (obsgroup_ == obsgroup_amsr) {
    int number_of_levels = numberOfLevels(varno_rawbt) + numberOfLevels(varno_rawbt_amsr_89ghz);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  } else if (obsgroup_ == obsgroup_abiclr || obsgroup_ == obsgroup_ahiclr
             || obsgroup_ == obsgroup_airs || obsgroup_ == obsgroup_atms
             || obsgroup_ == obsgroup_gmihigh || obsgroup_ == obsgroup_gmilow
             || obsgroup_ == obsgroup_mwri || obsgroup_ == obsgroup_seviriclr
             || obsgroup_ == obsgroup_ssmis) {
    int number_of_levels = numberOfLevels(varno_rawbt);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  } else if (obsgroup_ == obsgroup_mwsfy3) {
    int number_of_levels = numberOfLevels(varno_rawbt_mwts) + numberOfLevels(varno_rawbt_mwhs);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  } else if (obsgroup_ == obsgroup_geocloud) {
    int number_of_levels = numberOfLevels(varno_cloud_fraction_covered);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  } else if (obsgroup_ == obsgroup_surfacecloud) {
    int number_of_levels = numberOfLevels(varno_cloud_fraction_covered);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  } else if (obsgroup_ == obsgroup_scatwind) {
    int number_of_levels = numberOfLevels(varno_dd);
    vertcos.push_back(NewDimensionScale<int>("nchans", number_of_levels,
                                             number_of_levels, number_of_levels));
  }
  return vertcos;
}

double DataFromSQL::getData(const size_t row, const size_t column) const {
  if (data_.size() > 0) {
    return data_.at(column).at(row);
  } else {
    return odb_missing_float;
  }
}

double DataFromSQL::getData(const size_t row, const std::string& column) const {
  return getData(row, getColumnIndex(column));
}

Eigen::ArrayXf DataFromSQL::getMetadataColumn(std::string const& col) const {
  return getNumericMetadataColumn<float>(col);
}

Eigen::ArrayXi DataFromSQL::getMetadataColumnInt(std::string const& col) const {
  return getNumericMetadataColumn<int>(col);
}

template <typename T>
DataFromSQL::ArrayX<T> DataFromSQL::getNumericMetadataColumn(std::string const& col) const {
  int column_index = getColumnIndex(col);
  int seqno_index  = getColumnIndex("seqno");
  int varno_index  = getColumnIndex("varno");
  ArrayX<T> arr(number_of_metadata_rows_);
  if (column_index != -1) {
    size_t seqno = -1;
    size_t j     = 0;
    for (size_t i = 0; i < number_of_rows_; i++) {
      size_t seqno_new = getData(i, seqno_index);
      int varno        = getData(i, varno_index);
      if ((seqno != seqno_new && obsgroup_ != obsgroup_sonde)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_sonde)
          || (seqno != seqno_new && obsgroup_ != obsgroup_oceansound)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_oceansound)
          || (seqno != seqno_new && obsgroup_ != obsgroup_geocloud)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_geocloud)
          || (seqno != seqno_new && obsgroup_ != obsgroup_gpsro)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_gpsro)) {
        arr[j] = getData(i, column_index);
        j++;
        seqno = seqno_new;
      }
    }
  }
  return arr;
}

std::vector<std::string> DataFromSQL::getMetadataStringColumn(std::string const& col) const {
  int column_index = getColumnIndex(col);
  int seqno_index  = getColumnIndex("seqno");
  int varno_index  = getColumnIndex("varno");
  std::vector<std::string> arr;
  arr.reserve(number_of_metadata_rows_);
  if (column_index != -1) {
    size_t seqno = -1;
    for (size_t i = 0; i < number_of_rows_; i++) {
      size_t seqno_new = getData(i, seqno_index);
      int varno        = getData(i, varno_index);
      if ((seqno != seqno_new && obsgroup_ != obsgroup_sonde)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_sonde)
          || (seqno != seqno_new && obsgroup_ != obsgroup_oceansound)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_oceansound)
          || (seqno != seqno_new && obsgroup_ != obsgroup_geocloud)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_geocloud)
          || (seqno != seqno_new && obsgroup_ != obsgroup_gpsro)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_gpsro)) {
        // In ODB data is retrieved as doubles but character data is stored as ASCII bits.
        // A reinterpret_cast is used here to re-interpret the retrieved doubles as 8 character chunks.
        char uc[9];
        double ud = getData(i, column_index);
        strncpy(uc, reinterpret_cast<char*>(&ud), 8);
        uc[8] = '\0';

        // This attempts to trim the string.
        std::string s   = std::string(uc);
        size_t endpos   = s.find_last_not_of(" ");
        size_t startpos = s.find_first_not_of(" ");
        if (std::string::npos != endpos) {
          s = s.substr(0, endpos + 1);
          s = s.substr(startpos);
        } else {
          s.erase(std::remove(std::begin(s), std::end(s), ' '), std::end(s));
        }

        arr.push_back(s);
        seqno = seqno_new;
      }
    }
  }
  return arr;
}

const std::vector<std::string>& DataFromSQL::getColumns() const { return columns_; }

template <typename T>
Eigen::Array<T, Eigen::Dynamic, 1> DataFromSQL::getVarnoColumn(const std::vector<int>& varnos,
                                                               std::string const& col,
                                                               const int nchans,
                                                               const int nchans_actual) const {
  int column_index = getColumnIndex(col);
  int varno_index  = getColumnIndex("varno");
  size_t num_rows  = 0;
  if (nchans > 1) {
    num_rows = nchans * number_of_metadata_rows_;
  } else {
    for (auto& it : varnos) {
      num_rows = num_rows + numberOfRowsForVarno(it);
    }
  }
  typedef Eigen::Array<T, Eigen::Dynamic, 1> Array;
  Array arr = Array::Constant(num_rows, odb_missing<T>());
  if (nchans == 1) {
    if (column_index != -1 && varno_index != -1) {
      size_t j = 0;
      for (size_t i = 0; i < number_of_rows_; i++) {
        if (std::find(varnos.begin(), varnos.end(), getData(i, varno_index)) != varnos.end()) {
          arr[j] = getData(i, column_index);
          j++;
        }
      }
    }
  } else {
    if (column_index != -1 && varno_index != -1) {
      size_t j = 0;
      int k_chan = 1;
      for (size_t i = 0; i < number_of_rows_; i++) {
        if (std::find(varnos.begin(), varnos.end(), getData(i, varno_index)) != varnos.end()) {
          k_chan++;
          arr[j] = getData(i, column_index);
          j++;
          if (k_chan > nchans_actual) {
            j += (nchans - nchans_actual);  // skip unused channels
            k_chan = 1;
          }
        }
      }
    }
  }
  return arr;
}

void DataFromSQL::select(const std::vector<std::string>& columns, const std::string& filename,
                         const std::vector<int>& varnos, const std::string& query) {
  columns_ = columns;
  std::string sql = "select ";
  for (int i = 0; i < columns_.size(); i++) {
    if (i == 0) {
      sql = sql + columns_.at(i);
    } else {
      sql = sql + "," + columns_.at(i);
    }
  }
  sql = sql + " from \"" + filename + "\" where (";
  for (int i = 0; i < varnos.size(); i++) {
    if (i == 0) {
      sql = sql + "varno = " + std::to_string(varnos.at(i));
    } else {
      sql = sql + " or varno = " + std::to_string(varnos.at(i));
    }
  }
  sql              = sql + ")";
  if (!query.empty()) {
    sql = sql + " and (" + query + ");";
  } else {
    sql = sql + ";";
  }
  std::ifstream ifile;
  ifile.open(filename);
  if (ifile) {
    if (ifile.peek() == std::ifstream::traits_type::eof()) {
      ifile.close();
    } else {
      ifile.close();
      setData(sql);
    }
  }
  obsgroup_        = getData(0, getColumnIndex("ops_obsgroup"));
  number_of_rows_  = data_.empty() ? 0 : data_.front().size();
  int varno_column = getColumnIndex("varno");
  for (size_t i = 0; i < number_of_rows_; i++) {
    int varno = getData(i, varno_column);
    if (std::find(varnos_.begin(), varnos_.end(), varno) == varnos_.end()) {
      varnos_.push_back(varno);
    }
  }
  number_of_varnos_ = varnos_.size();
  number_of_metadata_rows_ = 0;

  if (obsgroup_ == obsgroup_sonde || obsgroup_ == obsgroup_gpsro ||
      obsgroup_ == obsgroup_oceansound) {
    number_of_metadata_rows_ = number_of_rows_ / number_of_varnos_;
  } else {
    int seqno_index          = getColumnIndex("seqno");
    size_t seqno             = -1;
    for (size_t i = 0; i < number_of_rows_; i++) {
      size_t seqno_new = getData(i, seqno_index);
      if (seqno != seqno_new) {
        number_of_metadata_rows_++;
        seqno = seqno_new;
      }
    }
  }

  for (const int varno : varnos_) {
    if (hasVarno(varno) && number_of_metadata_rows_ > 0) {
      const size_t number_of_varno_rows = numberOfRowsForVarno(varno);
      if (number_of_varno_rows % number_of_metadata_rows_ != 0)
        throw eckit::UnexpectedState("Not all observation sequences have the same number of rows "
                                     "with varno " + std::to_string(varno) + ". This is currently "
                                     "unsupported. As a workaround, modify the elements file used "
                                     "to generate the ODB file to ensure each observation sequence "
                                     "contains the same varnos");
      varnos_and_levels_[varno] = number_of_varno_rows / number_of_metadata_rows_;
    }
  }
}

int DataFromSQL::getObsgroup() const { return obsgroup_; }

std::vector<int64_t> DataFromSQL::getDates(std::string const& date_col,
                                           std::string const& time_col,
                                           util::DateTime const& epoch,
                                           int64_t const missingInt64) const {
  const Eigen::ArrayXi var_date = getMetadataColumnInt(date_col);
  const Eigen::ArrayXi var_time = getMetadataColumnInt(time_col);
  std::vector<int64_t> offsets;
  offsets.reserve(var_date.size());
  for (int i = 0; i < var_date.size(); i++) {
    if (var_date[i] != odb_missing_int && var_time[i] != odb_missing_int) {
      const int year   = var_date[i] / 10000;
      const int month  = var_date[i] / 100 - year * 100;
      const int day    = var_date[i] - 10000 * year - 100 * month;
      const int hour   = var_time[i] / 10000;
      const int minute = var_time[i] / 100 - hour * 100;
      const int second = var_time[i] - 10000 * hour - 100 * minute;
      const util::DateTime datetime(year, month, day, hour, minute, second);
      const int64_t offset = (datetime - epoch).toSeconds();
      offsets.push_back(offset);
    } else {
      offsets.push_back(missingInt64);
    }
  }
  return offsets;
}

std::vector<std::string> DataFromSQL::getStationIDs() const {
  std::ostringstream stream;
  std::vector<std::string> stationIDs;
  if (obsgroup_ == obsgroup_sonde) {
    const std::vector<std::string> var_statid = getMetadataStringColumn("statid");
    const Eigen::ArrayXi var_wmo_block_number = getMetadataColumnInt("wmo_block_number");
    const Eigen::ArrayXi var_wmo_station_number = getMetadataColumnInt("wmo_station_number");
    const size_t nlocs = var_wmo_block_number.size();
    stationIDs.assign(nlocs, odb_missing_string);
    for (int loc = 0; loc < nlocs; loc++) {
      // If statid is not empty, use that to fill the station ID.
      if (var_statid[loc] != "")
        stationIDs[loc] = var_statid[loc];
      // If WMO block and station numbers are present, use those to fill the station ID.
      // (This can override the assignment based on statid.)
      if (var_wmo_block_number[loc] != odb_missing_int &&
          var_wmo_station_number[loc] != odb_missing_int) {
        stream.str("");
        stream << std::setfill('0') << std::setw(2) << var_wmo_block_number[loc]
               << std::setfill('0') << std::setw(3) << var_wmo_station_number[loc];
        stationIDs[loc] = stream.str();
      }
    }
  } else if (obsgroup_ == obsgroup_oceansound) {
    const std::vector<std::string> var_statid = getMetadataStringColumn("statid");
    const Eigen::ArrayXi var_argo_identifier = getMetadataColumnInt("argo_identifier");
    const Eigen::ArrayXi var_buoy_identifier = getMetadataColumnInt("buoy_identifier");
    const size_t nlocs = var_argo_identifier.size();
    stationIDs.assign(nlocs, odb_missing_string);
    for (int loc = 0; loc < nlocs; loc++) {
      // If Argo identifier present, use those to fill the station ID.
      if (var_argo_identifier[loc] != odb_missing_int) {
        stream.str("");
        stream << std::setfill('0') << std::setw(8) << var_argo_identifier[loc];
        stationIDs[loc] = stream.str();
      // If Buoy identifier present, use those to fill the station ID.
      } else if (var_buoy_identifier[loc] != odb_missing_int) {
        stream.str("");
        stream << std::setfill('0') << std::setw(8) << var_buoy_identifier[loc];
        stationIDs[loc] = stream.str();
      // Neither Argo nor buoy identifier present; fill with statid
      } else if (var_statid[loc] != "") {
        stationIDs[loc] = var_statid[loc];
      }  // if statid empty, stationIDs[loc] defaults to missing string
    }
  }
  return stationIDs;
}

void DataFromSQL::createVarnoIndependentIodaVariable(
    std::string const& column, ioda::ObsGroup og,
    const ioda::VariableCreationParameters &params) const {
  const int col_type = getColumnTypeByName(column);
  if (col_type == odb_type_int || col_type == odb_type_bitfield) {
    createNumericVarnoIndependentIodaVariable<int>(column, og, params);
  } else if (col_type == odb_type_real) {
    createNumericVarnoIndependentIodaVariable<float>(column, og, params);
  } else {
    const std::vector<std::string> var = getMetadataStringColumn(column);
    ioda::Variable v = og.vars.createWithScales<std::string>(column, {og.vars["nlocs"]}, params);
    v.write(var);
  }
}

template <typename T>
void DataFromSQL::createNumericVarnoIndependentIodaVariable(
    std::string const& column, ioda::ObsGroup og,
    const ioda::VariableCreationParameters &params) const {
  const ArrayX<T> var = getNumericMetadataColumn<T>(column);
  VariableCreationParameters params_copy = params;
  params_copy.setFillValue<T>(odb_missing<T>());
  ioda::Variable v = og.vars.createWithScales<T>(column, {og.vars["nlocs"]}, params_copy);
  v.writeWithEigenRegular(var);
}

void DataFromSQL::createVarnoIndependentIodaVariables(
    const std::string &column, const std::set<std::string> &members, ioda::ObsGroup og,
    const ioda::VariableCreationParameters &params) const {
  const int col_index = getColumnIndex(column);
  const int col_type = column_types_.at(col_index);
  if (col_type != odb_type_bitfield)
    throw eckit::BadValue("Column '" + column + "' is not a bitfield", Here());
  const Eigen::ArrayXi var = getMetadataColumnInt(column);

  std::vector<char> member_values(var.size());

  const Bitfield &bitfield = column_bitfield_defs_.at(col_index);
  for (const BitfieldMember &member : bitfield) {
    if (!members.count(member.name))
      continue;
    if (member.size != 1)
      throw eckit::NotImplemented("Loading of bitfield members composed of multiple bits, "
                                  "such as '" + member.name +"', is not supported", Here());
    // Extract values of the current bitfield member into 'memberValues'
    const int mask = 1 << member.start;
    for (size_t loc = 0; loc < member_values.size(); ++loc)
      member_values[loc] = (var(loc) != odb_missing_int && (var(loc) & mask)) ? 1 : 0;
    ioda::Variable v = og.vars.createWithScales<char>(
          column + "." + member.name, {og.vars["nlocs"]}, params);
    v.write(member_values);
  }
}

ioda::Variable DataFromSQL::assignChannelNumbers(const int varno, ioda::ObsGroup og) const {
  const std::vector<int> varnos{varno};
  Eigen::ArrayXi var = getVarnoColumn<int>(varnos, std::string("initial_vertco_reference"),
                                           numberOfLevels(varno), numberOfLevels(varno));

  int number_of_levels = numberOfLevels(varno);
  ioda::Variable v = og.vars["nchans"];
  Eigen::ArrayXi var_single(number_of_levels);
  for (size_t i = 0; i < number_of_levels; i++) {
    var_single[i] = var[i];
  }
  v.writeWithEigenRegular(var_single);
  return v;
}

ioda::Variable DataFromSQL::assignChannelNumbersSeq(const std::vector<int> varnos, const ioda::ObsGroup og) const {
  int number_of_levels = 0;
  for (size_t i = 0; i < varnos.size(); i++) {
    number_of_levels += numberOfLevels(varnos[i]);
  }
  ioda::Variable v = og.vars["nchans"];
  Eigen::ArrayXi var_single(number_of_levels);
  for (size_t i = 0; i < number_of_levels; i++) {
    if (obsgroup_ == obsgroup_abiclr || obsgroup_ == obsgroup_ahiclr) {
      var_single[i] = i+7;
    } else if (obsgroup_ == obsgroup_gmihigh) {
      var_single[i] = i+10;
    } else {
      var_single[i] = i+1;
    }
  }
  v.writeWithEigenRegular(var_single);
  return v;
}

void DataFromSQL::createVarnoDependentIodaVariable(
    std::string const &column, const int varno,
    ioda::ObsGroup og, const ioda::VariableCreationParameters &params) const {
  const std::vector<ioda::Variable> dimensionScales =
      getVarnoDependentVariableDimensionScales(varno, og);
  if (dimensionScales.empty())
    return;

  std::vector<int> varnos;
  int nchans, nchans_actual;
  getVarnoColumnCallArguments(varno, varnos, nchans, nchans_actual);

  VariableCreationParameters params_copy = params;

  const int col_index = getColumnIndex(column);
  const int col_type = column_types_.at(col_index);
  if (col_type == odb_type_int || col_type == odb_type_bitfield) {
    Eigen::ArrayXi var = getVarnoColumn<int>(varnos, column, nchans, nchans_actual);
    params_copy.setFillValue<int>(odb_missing_int);
    ioda::Variable v = og.vars.createWithScales<int>(column + "/" + std::to_string(varno),
                                                     dimensionScales, params_copy);
    v.writeWithEigenRegular(var);
  } else if (col_type == odb_type_real) {
    Eigen::ArrayXf var = getVarnoColumn<float>(varnos, column, nchans, nchans_actual);
    params_copy.setFillValue<float>(odb_missing_float);
    ioda::Variable v = og.vars.createWithScales<float>(column + "/" + std::to_string(varno),
                                                       dimensionScales, params_copy);
    v.writeWithEigenRegular(var);
  } else {
    throw eckit::NotImplemented(
          "Retrieval of varno-dependent columns of type string is not supported yet", Here());
  }
}

void DataFromSQL::createVarnoDependentIodaVariables(
    const std::string &column, const std::set<std::string> &members, int varno,
    ioda::ObsGroup og, const ioda::VariableCreationParameters &params) const {
  const std::vector<ioda::Variable> dimensionScales =
      getVarnoDependentVariableDimensionScales(varno, og);
  if (dimensionScales.empty())
    return;

  std::vector<int> varnos;
  int nchans, nchans_actual;
  getVarnoColumnCallArguments(varno, varnos, nchans, nchans_actual);

  VariableCreationParameters params_copy = params;

  const int col_index = getColumnIndex(column);
  const int col_type = column_types_.at(col_index);
  if (col_type != odb_type_bitfield)
    throw eckit::BadValue("Column '" + column + "' is not a bitfield", Here());

  const Eigen::ArrayXi var = getVarnoColumn<int>(varnos, column, nchans, nchans_actual);
  std::vector<char> memberValues(var.size());
  const Bitfield &bitfield = column_bitfield_defs_.at(col_index);
  for (const BitfieldMember &member : bitfield) {
    if (!members.count(member.name))
      continue;
    if (member.size != 1)
      throw eckit::NotImplemented("Loading of bitfield members composed of multiple bits, "
                                  "such as '" + member.name +"', is not supported", Here());
    // Extract values of the current bitfield member into 'memberValues'
    const int mask = 1 << member.start;
    for (size_t loc = 0; loc < memberValues.size(); ++loc)
      memberValues[loc] = (var(loc) != odb_missing_int && (var(loc) & mask)) ? 1 : 0;
    ioda::Variable v = og.vars.createWithScales<char>(
          column + "." + member.name + "/" + std::to_string(varno), dimensionScales, params_copy);
    v.write(memberValues);
  }
}

std::vector<ioda::Variable> DataFromSQL::getVarnoDependentVariableDimensionScales(
    int varno, const ObsGroup &og) const {
  // FIXME: In general, the dimension scales to use might probably depend not only on the varno,
  // but also on the name of the ODB column to be converted into the ioda variable

  std::vector<ioda::Variable> dimensionScales;

  size_t number_of_levels;
  if (obsgroup_ == obsgroup_geocloud || obsgroup_ == obsgroup_surfacecloud) {
    number_of_levels = numberOfLevels(varno_cloud_fraction_covered);
  } else {
    number_of_levels = numberOfLevels(varno);
  }

  if (number_of_levels == 0) {
    // Leave dimensionScales empty
  } else if (number_of_levels == 1 && obsgroup_ != obsgroup_geocloud && obsgroup_ != obsgroup_surfacecloud) {
    dimensionScales = {og.vars["nlocs"]};
  } else {
    dimensionScales = {og.vars["nlocs"], og.vars["nchans"]};
  }

  return dimensionScales;
}

void DataFromSQL::getVarnoColumnCallArguments(int varno, std::vector<int> &varnos,
                                              int &nchans, int &nchans_actual) const {
  // Defaults...
  varnos = {varno};
  nchans = 1;
  nchans_actual = 1;
  // ... which in some cases need to be overridden
  if (obsgroup_ == obsgroup_atovs && varno == varno_rawbt_amsu) {
    varnos = {varno_rawbt_amsu};
  } else if (obsgroup_ == obsgroup_amsr && varno == varno_rawbt) {
    varnos = {varno_rawbt, varno_rawbt_amsr_89ghz};
  } else if (obsgroup_ == obsgroup_amsr && varno != varno_rawbt) {
    varnos = {varno_rawbt, varno_rawbt_amsr_89ghz};
  } else if (obsgroup_ == obsgroup_mwsfy3 && varno == varno_rawbt_mwts) {
    varnos = {varno_rawbt_mwts, varno_rawbt_mwhs};
  } else if (obsgroup_ == obsgroup_mwsfy3 && varno != varno_rawbt_mwts) {
    varnos = {varno_rawbt_mwts, varno_rawbt_mwhs};
  } else if (obsgroup_ == obsgroup_cris || obsgroup_ == obsgroup_hiras || obsgroup_ == obsgroup_iasi) {
    nchans = numberOfLevels(varno_rawsca);
    nchans_actual = numberOfLevels(varno);
  } else if (obsgroup_ == obsgroup_geocloud || obsgroup_ == obsgroup_surfacecloud) {
    nchans = numberOfLevels(varno_cloud_fraction_covered);
    nchans_actual = numberOfLevels(varno);
  }
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
