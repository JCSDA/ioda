#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** @file DataFromSQL.h
 * @brief implements ODC bindings
**/

#include <cctype>
#include <iomanip>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ioda/defs.h"
#include "ioda/ObsGroup.h"

#include "oops/util/DateTime.h"

#include "unsupported/Eigen/CXX11/Tensor"

namespace ioda {
namespace Engines {
namespace ODC {

  // TODO(DJDavies2): Take these obsgroup and varno
  // definitions and encapsulate these into the YAML
  // file structure.
  static constexpr int obsgroup_surface      = 1;
  static constexpr int obsgroup_scatwind     = 2;
  static constexpr int obsgroup_aircraft     = 4;
  static constexpr int obsgroup_sonde        = 5;
  static constexpr int obsgroup_atovs        = 7;
  static constexpr int obsgroup_oceansound   = 11;
  static constexpr int obsgroup_airs         = 16;
  static constexpr int obsgroup_gnssro       = 18;
  static constexpr int obsgroup_ssmis        = 19;
  static constexpr int obsgroup_iasi         = 26;
  static constexpr int obsgroup_seviriclr    = 27;
  static constexpr int obsgroup_geocloud     = 28;
  static constexpr int obsgroup_amsr         = 29;
  static constexpr int obsgroup_abiclr       = 37;
  static constexpr int obsgroup_atms         = 38;
  static constexpr int obsgroup_cris         = 39;
  static constexpr int obsgroup_surfacecloud = 42;
  static constexpr int obsgroup_mwsfy3       = 44;
  static constexpr int obsgroup_ahiclr       = 51;
  static constexpr int obsgroup_mwri         = 55;
  static constexpr int obsgroup_gmilow       = 56;
  static constexpr int obsgroup_gmihigh      = 57;
  static constexpr int obsgroup_sattcwv      = 58;
  static constexpr int obsgroup_hiras        = 60;

  static constexpr int varno_dd                     = 111;
  static constexpr int varno_ff                     = 112;
  static constexpr int varno_rawbt                  = 119;
  static constexpr int varno_bending_angle          = 162;
  static constexpr int varno_rawsca                 = 233;
  static constexpr int varno_u_amb                  = 242;
  static constexpr int varno_rawbt_hirs             = 248;
  static constexpr int varno_rawbt_amsu             = 249;
  static constexpr int varno_cloud_fraction_covered = 266;
  static constexpr int varno_rawbt_amsr_89ghz       = 267;
  static constexpr int varno_rawbt_mwts             = 274;
  static constexpr int varno_rawbt_mwhs             = 275;

  static constexpr int odb_type_int      = 1;
  static constexpr int odb_type_real     = 2;
  static constexpr int odb_type_string   = 3;
  static constexpr int odb_type_bitfield = 4;

  static constexpr float odb_missing_float = -2147483648.0f;
  static constexpr int odb_missing_int = 2147483647;
  static constexpr char* odb_missing_string = const_cast<char *>("*** MISSING ***");

class DataFromSQL {
private:
  template <typename T>
  using ArrayX = Eigen::Array<T, Eigen::Dynamic, 1>;

  /// Member of a bitfield column
  struct BitfieldMember {
    std::string name;
    std::int32_t start = 0;  // index of the first bit belonging to the member
    std::int32_t size = 1;   // number of bits belonging to the member
  };
  /// All members of a bitfield column
  typedef std::vector<BitfieldMember> Bitfield;

  std::vector<std::string> columns_;
  std::vector<int> column_types_;
  std::vector<Bitfield> column_bitfield_defs_;
  std::vector<int> varnos_;
  /// Each element contains values from a particular column
  std::vector<std::vector<double>> data_;
  size_t number_of_rows_           = 0;
  size_t number_of_metadata_rows_  = 0;
  size_t number_of_varnos_         = 0;
  size_t max_number_channels_      = 0;
  int obsgroup_                    = 0;
  std::map<int, size_t> varnos_and_levels_;
  std::map<int, size_t> varnos_and_levels_to_use_;

  /// \brief Returns the value for a particular row/column
  /// \param row Get data for this row
  /// \param column Get data for this column
  double getData(size_t row, size_t column) const;

  /// \brief Returns the value for a particular row/column (as name)
  /// \param row Get data for this row
  /// \param column Get data for this column
  double getData(size_t row, const std::string& column) const;

  /// \brief Populate structure with data from an sql
  /// \param sql The SQL string to generate the data for the structure
  void setData(const std::string& sql);

  /// \brief Append a new value to a particular column
  /// \param column Column to append data to
  /// \param value  Value to append
  void appendData(size_t column, double value);

  /// \brief Returns the number of rows for a particular varno
  /// \param varno The varno to check
  size_t numberOfRowsForVarno(int varno) const;

  /// \brief Returns true if a particular varno is present
  /// \param varno The varno to check
  bool hasVarno(int varno) const;

  /// \brief Returns the index of a specified column
  /// \param column The column to check
  int getColumnIndex(const std::string& column) const;

  /// \brief Returns the number of levels for each varno
  size_t numberOfLevels(int varno) const;

  /// \brief Returns data for a metadata (varno-independent) column
  /// \param column Get data for this column
  Eigen::ArrayXf getMetadataColumn(std::string const& column) const;

  /// \brief Returns data for a metadata (varno-independent) column
  /// \param column Get data for this column
  Eigen::ArrayXi getMetadataColumnInt(std::string const& column) const;

  /// \brief Returns data for a metadata (varno-independent) column
  /// \param column Get data for this column
  std::vector<std::string> getMetadataStringColumn(std::string const& column) const;

  /// \brief Strings are read about doubles from the ODB. Re-interpret this as a string and trim any spaces.
  /// \param ud The double which is to be re-interpreted.
  std::string reinterpretString(double ud) const;

  /// \brief Returns data for a metadata (varno-independent) column
  /// \param column Get data for this column
  template <typename T>
  ArrayX<T> getNumericMetadataColumn(std::string const& column) const;

  /// \brief Create an ioda variable for a specified varno-independent column
  /// \param column Column to create a variable for
  template <typename T>
  void createNumericVarnoIndependentIodaVariable(
      std::string const& column, ioda::ObsGroup og,
      const ioda::VariableCreationParameters &params) const;

  /// \brief Returns data for a varno for a varno column
  /// \param varno Get data for this varno
  /// \param column Get data for this column
  /// \param nchans Number of channels to store
  /// \param nchans_actual Actual number of channels
  template <typename T>
  Eigen::Array<T, Eigen::Dynamic, 1> getVarnoColumn(const std::vector<int>& varnos,
                                                    std::string const& column,
                                                    const int nchans,
                                                    const int nchans_actual) const;

  /// \brief Returns the dimension scales to attach to a variable holding the restriction of
  /// a varno-dependent column to rows with the specified varno
  std::vector<ioda::Variable> getVarnoDependentVariableDimensionScales(
      int varno, const ObsGroup &og) const;

  /// \brief Produces the arguments that createVarnoDependentIodaVariable() should pass to
  /// getVarnoColumn().
  ///
  /// \param[in] varno
  ///   Varno to retrieve.
  /// \param[out] varnos, nchans, nchans_actual
  ///   Arguments to be passed to getVarnoColumn().
  void getVarnoColumnCallArguments(int varno, std::vector<int> &varnos,
                                   int &nchans, int &nchans_actual) const;

public:
  /// \brief Simple constructor
  DataFromSQL(int maxNumberChannels);

  /// \brief Returns the number of "metadata" rows, i.e. hdr-type rows
  size_t numberOfMetadataRows() const;

  /// \brief Returns the dimensions for the ODB
  NewDimensionScales_t getVertcos() const;

  /// \brief Populate structure with data from specified columns, file and varnos
  /// \param columns List of columns to extract
  /// \param filename Extract from this file
  /// \param varnos List of varnos to extract
  /// \param query Selection criteria to apply
  /// \param truncateProfilesToNumLev Truncate multi-level profiles using the `numlev` variable.
  void select(const std::vector<std::string>& columns, const std::string& filename,
              const std::vector<int>& varnos, const std::string& query,
              const bool truncateProfilesToNumLev);

  /// \brief Returns a vector of date strings
  std::vector<int64_t> getDates(std::string const& date_col,
                                std::string const& time_col,
                                util::DateTime const& epoch,
                                int64_t const missingInt64,
                                std::string const& time_disp_col = "") const;

  /// \brief Returns a vector of station IDs
  std::vector<std::string> getStationIDs() const;

  /// \brief Returns the vector of names of columns selected by the SQL query
  const std::vector<std::string>& getColumns() const;

  /// \brief Creates an ioda variable for a specified varno-independent column
  /// \param column Column to be converted into an ioda variable
  void createVarnoIndependentIodaVariable(std::string const& column, ioda::ObsGroup og,
                                          const ioda::VariableCreationParameters &params) const;

  /// \brief Converts specified varno-independent bitfield column members into ioda variables
  ///
  /// \param column   Bitfield column name
  /// \param members  Names of the column members to be converted into ioda variables
  /// \param og       ObsGroup receiving the new ioda variables
  /// \param params   Creation parameters for the ioda variables
  void createVarnoIndependentIodaVariables(
      const std::string &column, const std::set<std::string> &members,
      ioda::ObsGroup og, const ioda::VariableCreationParameters &params) const;

  /// \brief Returns a list of channels associated with a particular varno.
  ioda::Variable assignChannelNumbers(int varno, ioda::ObsGroup og) const;

  /// \brief Returns a list of channels associated with a particular varno.
  ioda::Variable assignChannelNumbersSeq(const std::vector<int> varnos, const ioda::ObsGroup og) const;

  /// \brief Creates an ioda variable for a specified column
  void createVarnoDependentIodaVariable(std::string const &column, int varno,
                                        ioda::ObsGroup og,
                                        const VariableCreationParameters &params) const;

  /// \brief Converts specified varno-dependent bitfield column members into ioda variables
  /// containing the values from rows with a specified varno
  ///
  /// \param column   Bitfield column name
  /// \param members  Names of the column members to be converted into ioda variables
  /// \param varno    Varno to select
  /// \param og       ObsGroup receiving the new ioda variables
  /// \param params   Creation parameters for the ioda variables
  void createVarnoDependentIodaVariables(
      const std::string &column, const std::set<std::string> &members, int varno,
      ioda::ObsGroup og, const VariableCreationParameters &params) const;

  /// \brief Returns the type of a specified column
  /// \param column The column to check
  int getColumnTypeByName(std::string const& column) const;

  /// \brief Returns the obsgroup number
  int getObsgroup() const;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
