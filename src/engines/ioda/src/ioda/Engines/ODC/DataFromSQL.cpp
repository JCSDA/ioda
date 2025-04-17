/*
 * (C) Copyright 2021 UCAR
 * (C) Crown copyright 2021-2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/** @file DataFromSQL.cpp
 * @brief implements ODC bindings
**/

#include <fstream>
#include <algorithm>

#include "eckit/mpi/Comm.h"
#include "eckit/io/FileHandle.h"
#include "eckit/io/PartFileHandle.h"

#include "ioda/Engines/ODC/OdbConstants.h"
#include "ioda/Engines/ODC/DataFromSQL.h"
#include "ioda/Engines/ODC/ParallelIoUtils.h"
#include "ioda/Engines/ODC/OdbTablesRange.h"

#include "odc/api/Odb.h"
#include "odc/Select.h"

#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {

using Column = std::vector<double>;
using Table = std::vector<Column>;

}

size_t DataFromSQL::getNumberOfRows() const { return number_of_rows_; }

int DataFromSQL::getColumnIndex(const std::string& col) const {
  for (size_t i = 0; i < columns_.size(); i++) {
    if (columns_.at(i) == col) {
      return i;
    }
  }
  return -1;
}

const std::vector<int> &DataFromSQL::getVarnos() const {
  return varnos_;
}

void DataFromSQL::setData(const std::string& sql, const std::string &filename,
                          const eckit::mpi::Comm* comm, int chunks_per_process) {
  const size_t num_processes = comm ? comm->size() : 1;
  const bool enable_parallel_io = num_processes > 1;
  std::vector<std::unique_ptr<eckit::DataHandle>> chunk_handles;
  size_t total_num_chunks = 0;
  if (!enable_parallel_io) {
    chunk_handles.push_back(std::make_unique<eckit::FileHandle>(filename));
    total_num_chunks = 1;
  } else {
    std::vector<std::pair<eckit::Offset, eckit::Length>> frames;
    {
      for (const odc::core::Table &table :
           OdbTablesRange(std::make_unique<eckit::FileHandle>(filename))) {
        frames.push_back(std::make_pair(table.startPosition(),
                                        table.nextPosition() - table.startPosition()));
      }
    }
    if (frames.empty()) {
      // Ensure there's at least one chunk.
      chunk_handles.push_back(std::make_unique<eckit::FileHandle>(filename));
      total_num_chunks = 1;
    } else {
      const size_t my_rank = comm->rank();
      const size_t num_frames = frames.size();
      total_num_chunks = std::min(num_frames, num_processes * chunks_per_process);
      const std::vector<size_t> chunk_sizes =
          splitIntoChunksAsEvenlyAsPossible(num_frames, total_num_chunks);
      ASSERT(chunk_sizes.size() == total_num_chunks);
      size_t frame_index = 0;
      for (size_t chunk_index = 0; chunk_index < total_num_chunks; ++chunk_index) {
        if (chunk_index % num_processes == my_rank) {
          const eckit::Offset chunk_offset = frames[frame_index].first;
          eckit::Length chunk_length = 0;
          for (size_t i = 0; i < chunk_sizes[chunk_index]; ++i, ++frame_index)
            chunk_length += frames[frame_index].second;
          chunk_handles.push_back(std::make_unique<eckit::PartFileHandle>(
                                    filename, chunk_offset, chunk_length));
        } else {
          frame_index += chunk_sizes[chunk_index];
        }
      }
    }
  }

  const size_t num_columns = columns_.size();
  const size_t num_chunks = chunk_handles.size();
  std::vector<Table> data_chunks(num_chunks, Table(num_columns));

  // Retrieve data from frames making up each chunk.
  std::vector<int> column_types;
  std::vector<Bitfield> column_bitfield_defs;
  for (size_t chunk_index = 0; chunk_index < num_chunks; ++chunk_index) {
    odc::Select query(sql, *chunk_handles[chunk_index]);
    if (chunk_index == 0) {
      column_types = getColumnTypes(query.begin()->columns());
      column_bitfield_defs = getBitfieldDefs(query.begin()->columns());
    }

    Table &data_chunk = data_chunks[chunk_index];
    for (auto &row : query) {
      ASSERT(row.columns().size() == num_columns);
      for (size_t column = 0; column < num_columns; ++column) {
        data_chunk[column].push_back(static_cast<double>(row[column]));
      }
    }
  }

  if (enable_parallel_io) {
    const int seqno_column_index = getColumnIndex("seqno");
    if (seqno_column_index != -1)
      mergeSeqnosSplitAcrossChunks(*comm, total_num_chunks, num_columns, seqno_column_index,
                                   data_chunks);
  }

  size_t num_rows = 0;
  if (num_columns > 0) {
    for (const Table &data_chunk : data_chunks)
      num_rows += data_chunk.front().size();
  }

  Table data;
  if (num_chunks != 0) {
    data = std::move(data_chunks.front());
    for (size_t column = 0; column < num_columns; ++column) {
      std::vector<double> & destination = data[column];
      destination.reserve(num_rows);
      for (size_t chunk_index = 1; chunk_index < num_chunks; ++chunk_index) {
        std::vector<double> & source = data_chunks[chunk_index][column];
        destination.insert(destination.end(), source.begin(), source.end());
        // The source array has just been copied to destination and won't be needed any more.
        // Free it immediately to keep peak memory usage low.
        source.clear();
        source.shrink_to_fit();
      }
      destination.shrink_to_fit();
    }
  }

  data_ = std::move(data);
  column_types_ = std::move(column_types);
  column_bitfield_defs_ = std::move(column_bitfield_defs);
}

std::vector<int> DataFromSQL::getColumnTypes(const odc::core::MetaData &metadata) {
  std::vector<int> column_types;
  for (const odc::core::Column *column : metadata)
    column_types.push_back(column->type());
  return column_types;
}

std::vector<DataFromSQL::Bitfield> DataFromSQL::getBitfieldDefs(
    const odc::core::MetaData &metadata) {
  std::vector<DataFromSQL::Bitfield> column_bitfield_defs;
  for (const odc::core::Column *column : metadata) {
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
    column_bitfield_defs.push_back(std::move(bitfield));
  }
  return column_bitfield_defs;
}

int DataFromSQL::getColumnTypeByName(std::string const& column) const {
  return column_types_.at(getColumnIndex(column));
}

bool DataFromSQL::getBitfieldMemberDefinition(const std::string &column, const std::string &member,
                                              int &position, int &size) const {
  const int columnIndex = getColumnIndex(column);
  if (columnIndex < 0)
    return false;
  if (column_types_.at(columnIndex) != odb_type_bitfield)
    return false;

  for (const BitfieldMember& m : column_bitfield_defs_.at(columnIndex))
    if (m.name == member) {
      position = m.start;
      size = m.size;
      return true;
    }

  return false;
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

const std::vector<std::string>& DataFromSQL::getColumns() const { return columns_; }

void DataFromSQL::select(const std::vector<std::string>& columns, const std::string& filename,
                         const std::vector<int>& varnos, const std::string& query,
                         const eckit::mpi::Comm* comm, int chunks_per_process) {
  columns_ = columns;
  std::string sql = "select ";
  for (size_t i = 0; i < columns_.size(); i++) {
    if (i == 0) {
      sql = sql + columns_.at(i);
    } else {
      sql = sql + "," + columns_.at(i);
    }
  }
  sql = sql + " where (";
  for (size_t i = 0; i < varnos.size(); i++) {
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
  oops::Log::info() << "Using SQL: " << sql << std::endl;
  std::ifstream ifile;
  ifile.open(filename);
  if (ifile) {
    if (ifile.peek() == std::ifstream::traits_type::eof()) {
      ifile.close();
    } else {
      ifile.close();
      setData(sql, filename, comm, chunks_per_process);
    }
  }
  obsgroup_        = getData(0, getColumnIndex("ops_obsgroup"));
  number_of_rows_  = data_.empty() ? 0 : data_.front().size();
  int varno_column = getColumnIndex("varno");
  if (varno_column >= 0) {
    for (size_t i = 0; i < number_of_rows_; i++) {
      int varno = getData(i, varno_column);
      if (std::find(varnos_.begin(), varnos_.end(), varno) == varnos_.end()) {
        varnos_.push_back(varno);
      }
    }
  }
}

int DataFromSQL::getObsgroup() const { return obsgroup_; }

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
