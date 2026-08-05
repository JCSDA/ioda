#pragma once
/*
 * (C) Copyright 2021 UCAR
 * (C) Crown copyright 2021-2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** @file DataFromSQL.h
 * @brief implements ODC bindings
**/

#include <cctype>
#include <string>
#include <vector>

#include "ioda/defs.h"
#include "ioda/ObsGroup.h"

#include "unsupported/Eigen/CXX11/Tensor"

namespace eckit::mpi {
class Comm;
}

namespace odc::core {
class MetaData;
}

namespace ioda {
namespace Engines {
namespace ODC {

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
  size_t number_of_rows_ = 0;
  size_t global_number_of_chunks_ = 0;
  std::vector<size_t> number_of_rows_by_chunk_;

  /// \brief Populate structure with data from an sql
  /// \param sql The SQL string to generate the data for the structure
  /// \param filename Name of the file to extract data from.
  /// \param comm
  ///   (Optional) An MPI communicator. If it is non-null and its size is greater than 1, the ODB
  ///   file will be read in parallel.
  /// \param chunks_per_process
  ///   Maximum number of disjoint sequences of ODB frames read by an individual MPI process.
  ///   Ignored if `comm` is null.
  /// \param record_id_columns
  ///   Names of columns storing record ID components. If the ODB file is read in parallel,
  ///   sequences of *consecutive* rows in which all these components have the same values will not
  ///   be split across multiple ranks.
  void setData(const std::string& sql, const std::string& filename,
               const eckit::mpi::Comm* comm = nullptr, int chunks_per_process = 1,
               const std::vector<std::string>& record_id_columns = {"seqno"});

  /// \brief Extract column types from SQL query metadata.
  static std::vector<int> getColumnTypes(const odc::core::MetaData &metadata);
  /// \brief Extract bitfield definitions from SQL query metadata.
  static std::vector<Bitfield> getBitfieldDefs(const odc::core::MetaData &metadata);

public:
  /// \brief Simple constructor
  DataFromSQL() = default;

  /// \brief Returns the total number of rows.
  size_t getNumberOfRows() const;

  /// \brief The number of chunks of the input ODB file that have been read by this MPI process.
  size_t getNumberOfChunks() const;

  /// \brief The total number of chunks of the input ODB file that have been read by all
  /// MPI processes.
  size_t getGlobalNumberOfChunks() const;

  /// \brief A vector mapping the index of each row to the (local) index of the chunk from which
  /// that row was read.
  ///
  /// ("Local" means the index runs only over chunks read by this MPI process.)
  std::vector<size_t> getRowToChunkIndexMapping() const;

  /// \brief Populate structure with data from specified columns, file and varnos
  /// \param columns List of columns to extract
  /// \param filename Extract from this file
  /// \param varnos List of varnos to extract
  /// \param query Selection criteria to apply
  /// \param comm
  ///   (Optional) An MPI communicator. If it is non-null and its size is greater than 1, the ODB
  ///   file will be read in parallel, with each process in `comm` loading data from a subset of ODB
  ///   frames and then exchanging a (typically) small amount of data with other processes to
  ///   prevent consecutive rows with the same record ID (see `record_id_column_indices` below)
  ///   from being split across multiple ranks.
  /// \param chunks_per_process
  ///   Maximum number of disjoint sequences of ODB frames read by an individual MPI process.
  ///   Ignored if `comm` is null.
  /// \param record_id_columns
  ///   Names of columns storing record ID components. If the ODB file is read in parallel,
  ///   sequences of *consecutive* rows in which all these columns have the same values will not
  ///   be split across multiple ranks.
  void select(const std::vector<std::string>& columns, const std::string& filename,
              const std::vector<int>& varnos, const std::string& query,
              const eckit::mpi::Comm* comm = nullptr, int chunks_per_process = 1,
              const std::vector<std::string>& record_id_columns = {"seqno"});

  const std::vector<int> &getVarnos() const;

  /// \brief Returns the index of a specified column
  /// \param column The column to check
  int getColumnIndex(const std::string& column) const;

  /// \brief Returns the value for a particular row/column
  /// \param row Get data for this row
  /// \param column Get data for this column
  double getData(size_t row, size_t column) const;

  /// \brief Returns the value for a particular row/column (as name)
  /// \param row Get data for this row
  /// \param column Get data for this column
  double getData(size_t row, const std::string& column) const;

  /// \brief Returns the vector of names of columns selected by the SQL query
  const std::vector<std::string>& getColumns() const;

  /// \brief Returns the type of a specified column
  /// \param column The column to check
  int getColumnTypeByName(std::string const& column) const;

  /// \brief Get the position and size of a bitfield column member.
  ///
  /// \param[in] column Bitfield column name.
  /// \param[in] member Bitfield member name.
  /// \param[out] positions Index of the first bit belonging to the member.
  /// \param[out] size Number of bits belonging to the member.
  ///
  /// \returns True if \p column exists, is a bitfield column and has a member \p member, false
  /// otherwise.
  bool getBitfieldMemberDefinition(const std::string &column, const std::string &member,
                                   int &position, int &size) const;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
