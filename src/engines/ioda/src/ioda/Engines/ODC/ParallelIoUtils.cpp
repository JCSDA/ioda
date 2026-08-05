/*
 * (C) Crown copyright 2025, Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <numeric>
#include <Eigen/Core>

#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "ioda/Engines/ODC/OdbConstants.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {

using Column = std::vector<double>;
using Table = std::vector<Column>;

using RecordId = Eigen::RowVector<double, Eigen::Dynamic>;
using RecordIds = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

void getRecordId(const std::vector<const Column *>& record_columns, size_t row,
                 RecordId &record_id) {
  record_id.resize(record_columns.size());
  for (size_t col = 0; col != record_columns.size(); ++col)
    record_id[col] = (*record_columns[col])[row];
}

void allGatherv(const eckit::mpi::Comm &comm, const RecordIds& record_ids,
                eckit::mpi::Buffer<double> &recv) {
  comm.allGatherv(record_ids.data(),
                  record_ids.data() + record_ids.rows() * record_ids.cols(),
                  recv);
}

}  // namespace

std::vector<size_t> splitIntoChunksAsEvenlyAsPossible(size_t total, size_t num_chunks) {
  ASSERT(num_chunks > 0 && num_chunks <= total);
  std::vector<size_t> chunks(num_chunks, total / num_chunks);
  for (size_t i = 0; i < total % num_chunks; ++i)
      ++chunks[i];
  ASSERT(std::accumulate(chunks.begin(), chunks.end(), size_t(0)) == total);
  return chunks;
}

void mergeRecordsSplitAcrossChunks(const eckit::mpi::Comm &comm,
                                   size_t total_num_chunks,
                                   size_t num_columns,
                                   const std::vector<int> &record_id_column_indices,
                                   std::vector<Table> &local_chunks) {
  const size_t my_rank = comm.rank();
  const size_t num_processes = comm.size();
  const size_t num_local_chunks = local_chunks.size();
  const size_t record_id_size = record_id_column_indices.size();
  if (record_id_size == 0)
      return;  // Nothing to do

  // Collect information about records present in each chunk held by this process.

  const double missing_value = static_cast<double>(odb_missing_int);
  const auto missing_record_id = RecordId::Constant(record_id_size, missing_value);

  RecordIds first_record_id_by_local_index = RecordIds::Constant(num_local_chunks, record_id_size,
                                                                 missing_value);
  RecordIds last_record_id_by_local_index = RecordIds::Constant(num_local_chunks, record_id_size,
                                                                missing_value);
  std::vector<size_t> num_rows_in_first_record_by_local_index(num_local_chunks, 0);
  std::vector<char> is_record_id_constant_by_local_index(num_local_chunks, 1);
  {
    std::vector<const Column *> record_id_columns(record_id_size, nullptr);
    for (size_t chunk_index = 0; chunk_index < num_local_chunks; ++chunk_index) {
      if (local_chunks[chunk_index].empty())
        continue;

      record_id_columns.clear();
      for (int column_index : record_id_column_indices)
        record_id_columns.push_back(&local_chunks[chunk_index][column_index]);
      const size_t num_rows = record_id_columns.front()->size();
      for (size_t record_id_component_index = 1; record_id_component_index != record_id_size;
           ++record_id_component_index)
        if (record_id_columns[record_id_component_index]->size() != num_rows)
          throw eckit::BadValue("All columns must have the same length", Here());
      if (num_rows == 0)
        continue;

      RecordId first_record_id;
      getRecordId(record_id_columns, 0, first_record_id);
      first_record_id_by_local_index.row(chunk_index) = first_record_id;

      size_t num_rows_in_first_record = 1;
      {
        RecordId record_id;
        for (; num_rows_in_first_record != num_rows; ++num_rows_in_first_record ) {
          getRecordId(record_id_columns, num_rows_in_first_record, record_id);
          if (record_id != first_record_id)
            break;
        }
      }
      num_rows_in_first_record_by_local_index[chunk_index] = num_rows_in_first_record;

      RecordId last_record_id;
      getRecordId(record_id_columns, num_rows - 1, last_record_id);
      last_record_id_by_local_index.row(chunk_index) = last_record_id;

      is_record_id_constant_by_local_index[chunk_index] = num_rows_in_first_record == num_rows;
    }
  }

  // Gather analogous information about chunks held by all processes.

  RecordIds first_record_id_by_global_index = RecordIds::Constant(total_num_chunks, record_id_size,
                                                                  missing_value);
  RecordIds last_record_id_by_global_index = RecordIds::Constant(total_num_chunks, record_id_size,
                                                                 missing_value);
  std::vector<size_t> num_rows_in_first_record_by_global_index(total_num_chunks, 0);
  std::vector<char> is_record_id_constant_by_global_index(total_num_chunks, 1);
  {
    eckit::mpi::Buffer<double> first_record_id_by_rank_then_local_index(num_processes);
    eckit::mpi::Buffer<double> last_record_id_by_rank_then_local_index(num_processes);
    eckit::mpi::Buffer<size_t> num_rows_in_first_record_by_rank_then_local_index(num_processes);
    eckit::mpi::Buffer<char> is_record_id_constant_by_rank_then_local_index(num_processes);
    allGatherv(comm, first_record_id_by_local_index, first_record_id_by_rank_then_local_index);
    allGatherv(comm, last_record_id_by_local_index, last_record_id_by_rank_then_local_index);
    comm.allGatherv(num_rows_in_first_record_by_local_index.begin(),
                    num_rows_in_first_record_by_local_index.end(),
                    num_rows_in_first_record_by_rank_then_local_index);
    comm.allGatherv(is_record_id_constant_by_local_index.begin(),
                    is_record_id_constant_by_local_index.end(),
                    is_record_id_constant_by_rank_then_local_index);
    for (size_t rank = 0; rank < num_processes; ++rank) {
      const size_t num_local_indices =
          static_cast<size_t>(num_rows_in_first_record_by_rank_then_local_index.counts[rank]);
      for (size_t local_index = 0; local_index < num_local_indices; ++local_index) {
        const size_t global_index = rank + local_index * num_processes;
        const size_t buffer_index =
            num_rows_in_first_record_by_rank_then_local_index.displs[rank] + local_index;
        first_record_id_by_global_index.row(global_index) = Eigen::Map<RecordId>(
            &first_record_id_by_rank_then_local_index.buffer[buffer_index * record_id_size],
            record_id_size);
        last_record_id_by_global_index.row(global_index) = Eigen::Map<RecordId>(
            &last_record_id_by_rank_then_local_index.buffer[buffer_index * record_id_size],
            record_id_size);
        num_rows_in_first_record_by_global_index[global_index] =
            num_rows_in_first_record_by_rank_then_local_index.buffer[buffer_index];
        is_record_id_constant_by_global_index[global_index] =
            is_record_id_constant_by_rank_then_local_index.buffer[buffer_index];
      }
    }
  }

  // For empty chunks, copy the index of the first and last seqno from the nearest preceding
  // non-empty chunk, if one exists.
  {
    RecordId current_record_id(record_id_size);
    current_record_id.fill(missing_value);
    for (size_t global_index = 0; global_index < total_num_chunks; ++global_index) {
      if (first_record_id_by_global_index.row(global_index) == missing_record_id)
        first_record_id_by_global_index.row(global_index) = current_record_id;
      if (last_record_id_by_global_index.row(global_index) == missing_record_id)
        last_record_id_by_global_index.row(global_index) = current_record_id;
      current_record_id = last_record_id_by_global_index.row(global_index);
    }
  }

  // Identify the record id in the first row of each chunk and determine the chunk containing the
  // start of the surrounding sequence of rows with the same id. This will be the chunk "owning"
  // this sequence of rows, and we will send these rows to the process holding that chunk.
  std::vector<size_t> global_index_of_chunk_owning_first_record(total_num_chunks);
  {
    RecordId current_record_id = missing_record_id;
    size_t global_index_of_chunk_where_current_record_started = size_t(-1);
    for (size_t global_index = 0; global_index < total_num_chunks; ++global_index) {
      if (global_index == 0 ||
          first_record_id_by_global_index.row(global_index) != current_record_id) {
        global_index_of_chunk_owning_first_record[global_index] = global_index;
        current_record_id = last_record_id_by_global_index.row(global_index);
        global_index_of_chunk_where_current_record_started = global_index;
      } else {
        global_index_of_chunk_owning_first_record[global_index] =
            global_index_of_chunk_where_current_record_started;
        if (!is_record_id_constant_by_global_index[global_index]) {
          current_record_id = last_record_id_by_global_index.row(global_index);
          global_index_of_chunk_where_current_record_started = global_index;
        }
      }
    }
  }

  // Use Comm::allToallv() to send rows belonging to the first record in each chunk held by this
  // process to the process identified as the owner the surrounding sequence of rows with the same
  // record id.

  // First, prepare the data needed by the sending process...
  std::vector<double> send_buffer;
  std::vector<int> send_counts(num_processes);
  for (size_t destination_rank = 0; destination_rank < num_processes; ++destination_rank) {
    for (size_t local_index = 0; local_index < num_local_chunks; ++local_index) {
      const size_t global_index = my_rank + local_index * num_processes;
      const size_t first_record_owner_global_index =
          global_index_of_chunk_owning_first_record[global_index];
      if (first_record_owner_global_index == global_index)
        continue; // no need to transfer any data
      const size_t first_record_owner_rank = first_record_owner_global_index % num_processes;
      if (first_record_owner_rank != destination_rank)
        continue;

      const Table &local_chunk = local_chunks[local_index];
      const size_t num_rows_in_first_record = num_rows_in_first_record_by_local_index[local_index];
      for (size_t column_index = 0; column_index < num_columns; ++column_index)
        send_buffer.insert(send_buffer.end(),
                           local_chunk[column_index].begin(),
                           local_chunk[column_index].begin() + num_rows_in_first_record);
      send_counts[destination_rank] += num_rows_in_first_record * num_columns;
    }
  }

  std::vector<int> send_displacements;
  send_displacements.reserve(num_processes);
  int seed = 0;
  for (size_t i = 0; i < send_counts.size(); i++){
    send_displacements.push_back(seed);
    seed += send_counts[i];
  }

  // Second, prepare the data needed by the receiving process...
  std::vector<int> receive_counts(num_processes);
  for (size_t source_rank = 0; source_rank < num_processes; ++source_rank) {
    for (size_t global_index = source_rank; global_index < total_num_chunks;
         global_index += num_processes) {
      const size_t first_record_owner_global_index =
          global_index_of_chunk_owning_first_record[global_index];
      if (first_record_owner_global_index == global_index)
        continue; // no need to transfer any data
      const size_t first_record_owner_rank = first_record_owner_global_index % num_processes;
      if (first_record_owner_rank != my_rank)
        continue;

      const size_t num_rows_in_first_record =
          num_rows_in_first_record_by_global_index[global_index];
      receive_counts[source_rank] += num_rows_in_first_record * num_columns;
    }
  }

  std::vector<int> receive_displacements;
  receive_displacements.reserve(num_processes);
  seed = 0;
  for (size_t i = 0; i < receive_counts.size(); i++){
    receive_displacements.push_back(seed);
    seed += receive_counts[i];
  }
  const size_t receive_buffer_size = receive_displacements.back() + receive_counts.back();
  std::vector<double> receive_buffer(receive_buffer_size);

  // Third, call allToAllv.
  comm.allToAllv(send_buffer.data(), send_counts.data(), send_displacements.data(),
                 receive_buffer.data(), receive_counts.data(), receive_displacements.data());

  // Delete the rows we have sent to other processes.

  for (size_t local_index = 0; local_index < num_local_chunks; ++local_index) {
    const size_t global_index = my_rank + local_index * num_processes;
    if (global_index_of_chunk_owning_first_record[global_index] == global_index)
      continue;

    Table &local_chunk = local_chunks[local_index];
    const size_t num_rows_to_erase = num_rows_in_first_record_by_global_index[global_index];
    for (size_t column = 0; column < local_chunk.size(); ++column)
      local_chunk[column].erase(local_chunk[column].begin(),
                                local_chunk[column].begin() + num_rows_to_erase);
  }

  // Append rows we have received to the appropriate chunks.

  std::vector<size_t> current_offset(num_processes);
  for (size_t global_index = 0; global_index < total_num_chunks; ++global_index) {
    const size_t first_record_owner_global_index =
        global_index_of_chunk_owning_first_record[global_index];
    ASSERT(first_record_owner_global_index <= global_index);
    if (first_record_owner_global_index == global_index)
      continue;
    const size_t first_record_owner_rank = first_record_owner_global_index % num_processes;
    if (first_record_owner_rank != my_rank)
      continue;

    const size_t local_index = first_record_owner_global_index / num_processes;
    const size_t source_rank = global_index % num_processes;
    const std::vector<double>::const_iterator receive_buffer_block_begin =
        receive_buffer.begin() + receive_displacements[source_rank] + current_offset[source_rank];
    const size_t num_rows_to_append = num_rows_in_first_record_by_global_index[global_index];
    Table &local_chunk = local_chunks[local_index];
    for (size_t column = 0; column < num_columns; ++column)
      local_chunk[column].insert(local_chunk[column].end(),
                                 receive_buffer_block_begin + column * num_rows_to_append,
                                 receive_buffer_block_begin + (column + 1) * num_rows_to_append);
    current_offset[source_rank] += num_columns * num_rows_to_append;
  }
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
