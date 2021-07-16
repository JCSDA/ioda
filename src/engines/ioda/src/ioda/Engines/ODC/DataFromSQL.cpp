/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** @file DataFromSQL.cpp
 * @brief implements ODC bindings
**/

#include "./DataFromSQL.h"

#include "odc/Select.h"
#include "odc/api/odc.h"

namespace ioda {
namespace Engines {
namespace ODC {

DataFromSQL::DataFromSQL() {}

size_t DataFromSQL::numberOfRows() const { return number_of_rows_; }
size_t DataFromSQL::numberOfMetadataRows() const { return number_of_metadata_rows_; }
size_t DataFromSQL::numberOfVarnos() const { return number_of_varnos_; }
size_t DataFromSQL::numberOfColumns() const { return columns_.size(); }

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

size_t DataFromSQL::numberOfLevels(const size_t varno) const {
  if (hasVarno(varno) && number_of_metadata_rows_ > 0) {
    return numberOfRowsForVarno(varno) / number_of_metadata_rows_;
  }
  return 0;
}

void DataFromSQL::setData(const size_t row, const size_t column, const double val) {
  data_.at(row * columns_.size() + column) = val;
}

void DataFromSQL::setData(const std::string& sql) {
  odc::Select sodb(sql);
  odc::Select::iterator begin = sodb.begin();
  size_t rowNumber            = 0;
  for (size_t i = 0; i < begin->columns().size(); i++) {
    column_types_.push_back(begin->columns().at(i)->type());
  }
  for (auto &row : sodb) {
    for (size_t i = 0; i < begin->columns().size(); i++) {
      setData(rowNumber, i, (row)[i]);
    }
    rowNumber++;
  }
}

int DataFromSQL::getColumnTypeByName(std::string const& column) const {
  size_t col_index = getColumnIndex(column);
  return column_types_.at(col_index);
}

NewDimensionScales_t DataFromSQL::getVertcos() const {
  NewDimensionScales_t vertcos;
  const int num_rows = numberOfMetadataRows();
  vertcos.push_back(
    NewDimensionScale<int>("nlocs", num_rows, num_rows, num_rows));
  if (obsgroup_ == obsgroup_iasi || obsgroup_ == obsgroup_cris || obsgroup_ == obsgroup_hiras) {
    int number_of_levels = numberOfLevels(varno_rawbt);
    vertcos.push_back(NewDimensionScale<int>("Channels", number_of_levels,
                                             number_of_levels, number_of_levels));
    number_of_levels = numberOfLevels(varno_rawsca);
    vertcos.push_back(NewDimensionScale<int>("ChannelsRadiance", number_of_levels,
                                             number_of_levels, number_of_levels));
    if (obsgroup_ == obsgroup_iasi) {
      number_of_levels = numberOfLevels(varno_rawbt_amsu);
      vertcos.push_back(NewDimensionScale<int>(
        "ChannelsAMSU", number_of_levels, number_of_levels, number_of_levels));
    }
  } else if (obsgroup_ == obsgroup_atovs) {
    int number_of_levels = numberOfLevels(varno_rawbt_hirs) + numberOfLevels(varno_rawbt_amsu);
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
  } else if (obsgroup_ == obsgroup_gpsro) {
    int number_of_levels = numberOfLevels(varno_bending_angle);
    vertcos.push_back(NewDimensionScale<int>(
      "MetaData/impact_parameters", number_of_levels, number_of_levels, number_of_levels));
  }
  return vertcos;
}

double DataFromSQL::getData(const size_t row, const size_t column) const {
  return data_.at(row * columns_.size() + column);
}

double DataFromSQL::getData(const size_t row, const std::string& column) const {
  int column_index = getColumnIndex(column);
  return data_.at(row * columns_.size() + column_index);
}

Eigen::ArrayXf DataFromSQL::getMetadataColumn(std::string const& col) const {
  int column_index = getColumnIndex(col);
  int seqno_index  = getColumnIndex("seqno");
  int varno_index  = getColumnIndex("varno");
  Eigen::ArrayXf arr(number_of_metadata_rows_);
  if (column_index != -1) {
    size_t seqno = -1;
    size_t j     = 0;
    for (size_t i = 0; i < number_of_rows_; i++) {
      size_t seqno_new = getData(i, seqno_index);
      int varno        = getData(i, varno_index);
      if ((seqno != seqno_new && obsgroup_ != obsgroup_sonde)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_sonde)
          || (seqno != seqno_new && obsgroup_ != obsgroup_scatwind)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_scatwind)
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

Eigen::ArrayXi DataFromSQL::getMetadataColumnInt(std::string const& col) const {
  int column_index = getColumnIndex(col);
  int seqno_index  = getColumnIndex("seqno");
  int varno_index  = getColumnIndex("varno");
  Eigen::ArrayXi arr(number_of_metadata_rows_);
  if (column_index != -1) {
    size_t seqno = -1;
    size_t j     = 0;
    for (size_t i = 0; i < number_of_rows_; i++) {
      size_t seqno_new = getData(i, seqno_index);
      int varno        = getData(i, varno_index);
      if ((seqno != seqno_new && obsgroup_ != obsgroup_sonde)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_sonde)
          || (seqno != seqno_new && obsgroup_ != obsgroup_scatwind)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_scatwind)
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
  if (column_index != -1) {
    size_t seqno = -1;
    for (size_t i = 0; i < number_of_rows_; i++) {
      size_t seqno_new = getData(i, seqno_index);
      int varno        = getData(i, varno_index);
      if ((seqno != seqno_new && obsgroup_ != obsgroup_sonde)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_sonde)
          || (seqno != seqno_new && obsgroup_ != obsgroup_scatwind)
          || (varnos_[0] == varno && obsgroup_ == obsgroup_scatwind)
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

Eigen::ArrayXf DataFromSQL::getVarnoColumn(const std::vector<int>& varnos,
                                           std::string const& col) const {
  int column_index = getColumnIndex(col);
  int varno_index  = getColumnIndex("varno");
  size_t num_rows  = 0;
  for (auto& it : varnos) {
    num_rows = num_rows + numberOfRowsForVarno(it);
  }
  Eigen::ArrayXf arr(num_rows);
  if (column_index != -1 && varno_index != -1) {
    size_t j = 0;
    for (size_t i = 0; i < number_of_rows_; i++) {
      if (std::find(varnos.begin(), varnos.end(), getData(i, varno_index)) != varnos.end()) {
        arr[j] = getData(i, column_index);
        j++;
      }
    }
  }
  return arr;
}

void DataFromSQL::select(const std::vector<std::string>& columns, const std::string& filename,
                         const std::vector<int>& varnos) {
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
  sql              = sql + ");";
  size_t totalRows = countRows(sql);
  data_.resize(totalRows * columns_.size());
  setData(sql);
  obsgroup_        = getData(0, getColumnIndex("ops_obsgroup"));
  number_of_rows_  = data_.size() / columns_.size();
  int varno_column = getColumnIndex("varno");
  for (size_t i = 0; i < number_of_rows_; i++) {
    int varno = getData(i, varno_column);
    if (std::find(varnos_.begin(), varnos_.end(), varno) == varnos_.end()) {
      varnos_.push_back(varno);
    }
  }
  number_of_varnos_ = varnos_.size();
  int seqno_column  = getColumnIndex("seqno");
  for (size_t i = 0; i < number_of_rows_; i++) {
    int seqno = getData(i, seqno_column);
    if (std::find(seqnos_.begin(), seqnos_.end(), seqno) == seqnos_.end()) {
      seqnos_.push_back(seqno);
    }
  }
  number_of_obs_           = seqnos_.size();
  number_of_metadata_rows_ = 0;
  int seqno_index          = getColumnIndex("seqno");
  size_t seqno             = -1;
  if (obsgroup_ == obsgroup_sonde || obsgroup_ == obsgroup_scatwind
      || obsgroup_ == obsgroup_gpsro) {
    number_of_metadata_rows_ = number_of_rows_ / number_of_varnos_;
  } else {
    for (size_t i = 0; i < number_of_rows_; i++) {
      size_t seqno_new = getData(i, seqno_index);
      if (seqno != seqno_new) {
        number_of_metadata_rows_++;
        seqno = seqno_new;
      }
    }
  }
  number_of_rows_per_ob_    = number_of_rows_ / number_of_obs_;
  number_of_rows_per_varno_ = number_of_rows_per_ob_ / number_of_varnos_;
}

int DataFromSQL::getObsgroup() const { return obsgroup_; }

std::vector<std::string> DataFromSQL::getDates(std::string const& date_col,
                                               std::string const& time_col) const {
  Eigen::ArrayXf var_date = getMetadataColumn(date_col);
  Eigen::ArrayXf var_time = getMetadataColumn(time_col);
  std::vector<std::string> date_strings;
  for (int i = 0; i < var_date.size(); i++) {
    int year   = var_date[i] / 10000;
    int month  = var_date[i] / 100 - year * 100;
    int day    = var_date[i] - 10000 * year - 100 * month;
    int hour   = var_time[i] / 10000;
    int minute = var_time[i] / 100 - hour * 100;
    int second = var_time[i] - 10000 * hour - 100 * minute;
    std::ostringstream stream;
    stream << year << "-" << std::setfill('0') << std::setw(2) << month << "-" << std::setfill('0')
           << std::setw(2) << day << "T" << std::setfill('0') << std::setw(2) << hour << ":"
           << std::setfill('0') << std::setw(2) << minute << ":" << std::setfill('0')
           << std::setw(2) << second << "Z";
    std::string datetimestring = stream.str();
    date_strings.push_back(datetimestring);
  }
  return date_strings;
}

ioda::Variable DataFromSQL::getIodaVariable(std::string const& column, ioda::ObsGroup og,
                                            ioda::VariableCreationParameters params) const {
  ioda::Variable v;
  int col_type = getColumnTypeByName(column);
  Eigen::ArrayXf var;
  Eigen::ArrayXi vari;
  std::vector<std::string> vard;
  if (col_type == odb_type_int || col_type == odb_type_bitfield) {
    vari = getMetadataColumnInt(column);
  } else if (col_type == odb_type_real) {
    var = getMetadataColumn(column);
  } else {
    vard = getMetadataStringColumn(column);
  }
  if (col_type == odb_type_int || col_type == odb_type_bitfield) {
    v = og.vars.createWithScales<int>(column, {og.vars["nlocs"]}, params);
    v.writeWithEigenRegular(vari);
    v.atts.add<int>("_FillValue", 2147483647);
  } else if (col_type == odb_type_real) {
    v = og.vars.createWithScales<float>(column, {og.vars["nlocs"]}, params);
    v.writeWithEigenRegular(var);
    v.atts.add<float>("_FillValue", -2147483648.0f);
  } else {
    v = og.vars.createWithScales<std::string>(column, {og.vars["nlocs"]}, params);
    v.write(vard);
  }
  return v;
}

ioda::Variable DataFromSQL::getIodaObsvalue(const int varno, ioda::ObsGroup og,
                                            ioda::VariableCreationParameters params) const {
  ioda::Variable v;
  Eigen::ArrayXf var;
  if (obsgroup_ == obsgroup_atovs && varno == varno_rawbt_hirs) {
    const std::vector<int> varnos{varno_rawbt_hirs, varno_rawbt_amsu};
    var = getVarnoColumn(varnos, "initial_obsvalue");
  } else if (obsgroup_ == obsgroup_atovs && varno == varno_rawbt_amsu) {
    const std::vector<int> varnos{varno_rawbt_hirs, varno_rawbt_amsu};
    var = getVarnoColumn(varnos, "initial_obsvalue");
  } else if (obsgroup_ == obsgroup_amsr && varno == varno_rawbt) {
    const std::vector<int> varnos{varno_rawbt, varno_rawbt_amsr_89ghz};
    var = getVarnoColumn(varnos, "initial_obsvalue");
  } else if (obsgroup_ == obsgroup_amsr && varno != varno_rawbt) {
    const std::vector<int> varnos{varno_rawbt, varno_rawbt_amsr_89ghz};
    var = getVarnoColumn(varnos, "initial_obsvalue");
  } else if (obsgroup_ == obsgroup_mwsfy3 && varno == varno_rawbt_mwts) {
    const std::vector<int> varnos{varno_rawbt_mwts, varno_rawbt_mwhs};
    var = getVarnoColumn(varnos, "initial_obsvalue");
  } else if (obsgroup_ == obsgroup_mwsfy3 && varno != varno_rawbt_mwts) {
    const std::vector<int> varnos{varno_rawbt_mwts, varno_rawbt_mwhs};
    var = getVarnoColumn(varnos, "initial_obsvalue");
  } else {
    const std::vector<int> varnos{varno};
    var = getVarnoColumn(varnos, "initial_obsvalue");
  }
  int number_of_levels;
  if (obsgroup_ == obsgroup_atovs && varno == varno_rawbt_hirs) {
    number_of_levels = numberOfLevels(varno_rawbt_hirs) + numberOfLevels(varno_rawbt_amsu);
  } else {
    number_of_levels = numberOfLevels(varno);
  }
  if (number_of_levels <= 0) return v;
  if (number_of_levels == 1) {
    v = og.vars.createWithScales<float>(std::to_string(varno), {og.vars["nlocs"]}, params);
  } else {
    std::string vertco_type;
    if (obsgroup_ == obsgroup_iasi || obsgroup_ == obsgroup_cris || obsgroup_ == obsgroup_hiras) {
      if (varno == varno_rawbt) {
        vertco_type = "Channels";
      } else if (varno == varno_rawsca) {
        vertco_type = "ChannelsRadiance";
      } else if (varno == varno_rawbt_amsu) {
        vertco_type = "ChannelsAMSU";
      }
    } else if (obsgroup_ == obsgroup_atovs) {
      if (varno == varno_rawbt_hirs) {
        vertco_type = "nchans";
      }
    } else if (obsgroup_ == obsgroup_amsr) {
      if (varno == varno_rawbt) {
        vertco_type = "nchans";
      }
    } else if (obsgroup_ == obsgroup_mwsfy3) {
      if (varno == varno_rawbt_mwts) {
        vertco_type = "nchans";
      }
    } else if (obsgroup_ == obsgroup_abiclr || obsgroup_ == obsgroup_ahiclr
               || obsgroup_ == obsgroup_airs || obsgroup_ == obsgroup_atms
               || obsgroup_ == obsgroup_gmihigh || obsgroup_ == obsgroup_gmilow
               || obsgroup_ == obsgroup_mwri || obsgroup_ == obsgroup_seviriclr
               || obsgroup_ == obsgroup_ssmis) {
      if (varno == varno_rawbt) {
        vertco_type = "nchans";
      }
    } else if (obsgroup_ == obsgroup_gpsro) {
      if (varno == varno_bending_angle) {
        vertco_type = "MetaData/impact_parameters";
      }
    }
    if (vertco_type == "") {
      v = og.vars.createWithScales<float>(std::to_string(varno), {og.vars["nlocs"]},
                                          params);
    } else if (obsgroup_ == obsgroup_abiclr || obsgroup_ == obsgroup_ahiclr
               || obsgroup_ == obsgroup_airs || obsgroup_ == obsgroup_atms
               || obsgroup_ == obsgroup_gmihigh || obsgroup_ == obsgroup_gmilow
               || obsgroup_ == obsgroup_mwri || obsgroup_ == obsgroup_mwsfy3
               || obsgroup_ == obsgroup_seviriclr || obsgroup_ == obsgroup_ssmis) {
      v = og.vars.createWithScales<float>(std::to_string(varno),
                                          {og.vars["nlocs"], og.vars["nchans"]}, params);
    } else {
      v = og.vars.createWithScales<float>(
        std::to_string(varno), {og.vars["nlocs"], og.vars[vertco_type]}, params);
    }
  }
  v.writeWithEigenRegular(var);
  v.atts.add<float>("_FillValue", -2147483648.0f);
  return v;
}

size_t DataFromSQL::countRows(const std::string& sql) {
  odc::Select sodb(sql);
  odc::Select::iterator it  = sodb.begin();
  odc::Select::iterator end = sodb.end();
  size_t totalRows          = 0;
  for (; it != end; ++it) {
    totalRows++;
  }
  return totalRows;
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
