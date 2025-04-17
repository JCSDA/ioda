/*
 * (C) Crown copyright 2025, Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <algorithm>
#include <numeric>

#include "eckit/exception/Exceptions.h"
#include "eckit/io/FileHandle.h"
#include "eckit/io/PartFileHandle.h"
#include "eckit/mpi/Comm.h"

#include "ioda/Engines/ODC/OdbConstants.h"

namespace ioda {
namespace Engines {
namespace ODC {

namespace {

using Column = std::vector<double>;
using Table = std::vector<Column>;

}

std::vector<size_t> splitIntoChunksAsEvenlyAsPossible(size_t total, size_t num_chunks) {
  ASSERT(num_chunks > 0 && num_chunks <= total);
  std::vector<size_t> chunks(num_chunks, total / num_chunks);
  for (size_t i = 0; i < total % num_chunks; ++i)
      ++chunks[i];
  ASSERT(std::accumulate(chunks.begin(), chunks.end(), size_t(0)) == total);
  return chunks;
}

void mergeSeqnosSplitAcrossChunks(const eckit::mpi::Comm &comm,
                                  size_t total_num_chunks,
                                  size_t num_columns,
                                  int seqno_column_index,
                                  std::vector<Table> &local_chunks) {
  const size_t my_rank = comm.rank();
  const size_t num_processes = comm.size();
  const size_t num_local_chunks = local_chunks.size();

  // Collect information about seqnos present in each chunk held by this process.

  const size_t missing_seqno = odb_missing_int;
  std::vector<size_t> first_seqno_by_local_index(num_local_chunks, missing_seqno);
  std::vector<size_t> last_seqno_by_local_index(num_local_chunks, missing_seqno);
  std::vector<size_t> num_rows_in_first_seqno_by_local_index(num_local_chunks, 0);
  std::vector<char> is_seqno_constant_by_local_index(num_local_chunks, 1);
  for (size_t chunk_index = 0; chunk_index < num_local_chunks; ++chunk_index) {
    if (local_chunks[chunk_index].empty())
      continue;

    const Column &seqnos = local_chunks[chunk_index][seqno_column_index];
    if (seqnos.empty())
      continue;

    const double first_seqno = seqnos.front();
    first_seqno_by_local_index[chunk_index] = first_seqno;
    num_rows_in_first_seqno_by_local_index[chunk_index] =
        std::find_if(seqnos.begin() + 1, seqnos.end(),
                     [first_seqno](double seqno) { return seqno != first_seqno; }) -
        seqnos.begin();
    last_seqno_by_local_index[chunk_index] = seqnos.back();
    is_seqno_constant_by_local_index[chunk_index] = std::all_of(
          seqnos.begin() + 1, seqnos.end(),
          [first_seqno](double seqno) { return seqno == first_seqno; });
  }

  // Gather analogous information about chunks held by all processes.

  std::vector<size_t> first_seqno_by_global_index(total_num_chunks);
  std::vector<size_t> last_seqno_by_global_index(total_num_chunks);
  std::vector<size_t> num_rows_in_first_seqno_by_global_index(total_num_chunks);
  std::vector<char> is_seqno_constant_by_global_index(total_num_chunks);
  {
    eckit::mpi::Buffer<size_t> first_seqno_by_rank_then_local_index(num_processes);
    eckit::mpi::Buffer<size_t> last_seqno_by_rank_then_local_index(num_processes);
    eckit::mpi::Buffer<size_t> num_rows_in_first_seqno_by_rank_then_local_index(num_processes);
    eckit::mpi::Buffer<char> is_seqno_constant_by_rank_then_local_index(num_processes);
    comm.allGatherv(first_seqno_by_local_index.begin(),
                    first_seqno_by_local_index.end(),
                    first_seqno_by_rank_then_local_index);
    comm.allGatherv(last_seqno_by_local_index.begin(),
                    last_seqno_by_local_index.end(),
                    last_seqno_by_rank_then_local_index);
    comm.allGatherv(num_rows_in_first_seqno_by_local_index.begin(),
                    num_rows_in_first_seqno_by_local_index.end(),
                    num_rows_in_first_seqno_by_rank_then_local_index);
    comm.allGatherv(is_seqno_constant_by_local_index.begin(),
                    is_seqno_constant_by_local_index.end(),
                    is_seqno_constant_by_rank_then_local_index);
    for (size_t rank = 0; rank < num_processes; ++rank)
      for (size_t local_index = 0;
           local_index < static_cast<size_t>(first_seqno_by_rank_then_local_index.counts[rank]);
           ++local_index) {
        const size_t global_index = rank + local_index * num_processes;
        const size_t buffer_index = first_seqno_by_rank_then_local_index.displs[rank] + local_index;
        first_seqno_by_global_index[global_index] =
            first_seqno_by_rank_then_local_index.buffer[buffer_index];
        last_seqno_by_global_index[global_index] =
            last_seqno_by_rank_then_local_index.buffer[buffer_index];
        num_rows_in_first_seqno_by_global_index[global_index] =
            num_rows_in_first_seqno_by_rank_then_local_index.buffer[buffer_index];
        is_seqno_constant_by_global_index[global_index] =
            is_seqno_constant_by_rank_then_local_index.buffer[buffer_index];
      }
  }

  // For empty chunks, copy the index of the first and last seqno from the nearest preceding
  // non-empty chunk, if one exists.
  {
    size_t current_seqno = missing_seqno;
    for (size_t global_index = 0; global_index < total_num_chunks; ++global_index) {
      if (first_seqno_by_global_index[global_index] == missing_seqno)
        first_seqno_by_global_index[global_index] = current_seqno;
      if (last_seqno_by_global_index[global_index] == missing_seqno)
        last_seqno_by_global_index[global_index] = current_seqno;
      current_seqno = last_seqno_by_global_index[global_index];
    }
  }

  // Identify the seqno in the first row of each chunk and determine the chunk containing the
  // start of the surrounding sequence of rows with the same seqno. This will be the chunk "owning"
  // this sequence of rows, and we will send these rows to the process holding that chunk.
  std::vector<size_t> global_index_of_chunk_owning_first_seqno(total_num_chunks);
  size_t current_seqno = missing_seqno;
  size_t global_index_of_chunk_where_current_seqno_started = size_t(-1);
  for (size_t global_index = 0; global_index < total_num_chunks; ++global_index) {
    if (global_index == 0 || first_seqno_by_global_index[global_index] != current_seqno) {
      global_index_of_chunk_owning_first_seqno[global_index] = global_index;
      current_seqno = last_seqno_by_global_index[global_index];
      global_index_of_chunk_where_current_seqno_started = global_index;
    } else {
      global_index_of_chunk_owning_first_seqno[global_index] =
          global_index_of_chunk_where_current_seqno_started;
      if (!is_seqno_constant_by_global_index[global_index]) {
        current_seqno = last_seqno_by_global_index[global_index];
        global_index_of_chunk_where_current_seqno_started = global_index;
      }
    }
  }

  // Use Comm::allToallv() to send rows belonging to the first seqno in each chunk held by this
  // process to the process identified as the owner the surrounding sequence of rows with the same
  // seqno.

  // First, prepare the data needed by the sending process...
  std::vector<double> send_buffer;
  std::vector<int> send_counts(num_processes);
  for (size_t destination_rank = 0; destination_rank < num_processes; ++destination_rank) {
    for (size_t local_index = 0; local_index < num_local_chunks; ++local_index) {
      const size_t global_index = my_rank + local_index * num_processes;
      const size_t first_seqno_owner_global_index =
          global_index_of_chunk_owning_first_seqno[global_index];
      if (first_seqno_owner_global_index == global_index)
        continue; // no need to transfer any data
      const size_t first_seqno_owner_rank = first_seqno_owner_global_index % num_processes;
      if (first_seqno_owner_rank != destination_rank)
        continue;

      const Table &local_chunk = local_chunks[local_index];
      const size_t num_rows_in_first_seqno = num_rows_in_first_seqno_by_local_index[local_index];
      for (size_t column_index = 0; column_index < num_columns; ++column_index)
        send_buffer.insert(send_buffer.end(),
                           local_chunk[column_index].begin(),
                           local_chunk[column_index].begin() + num_rows_in_first_seqno);
      send_counts[destination_rank] += num_rows_in_first_seqno * num_columns;
    }
  }

  std::vector<int> send_displacements;
  send_displacements.reserve(num_processes);
  std::exclusive_scan(send_counts.begin(), send_counts.end(),
                      std::back_inserter(send_displacements), 0);

  // Second, prepare the data needed by the receiving process...
  std::vector<int> receive_counts(num_processes);
  for (size_t source_rank = 0; source_rank < num_processes; ++source_rank) {
    for (size_t global_index = source_rank; global_index < total_num_chunks;
         global_index += num_processes) {
      const size_t first_seqno_owner_global_index =
          global_index_of_chunk_owning_first_seqno[global_index];
      if (first_seqno_owner_global_index == global_index)
        continue; // no need to transfer any data
      const size_t first_seqno_owner_rank = first_seqno_owner_global_index % num_processes;
      if (first_seqno_owner_rank != my_rank)
        continue;

      const size_t num_rows_in_first_seqno =
          num_rows_in_first_seqno_by_global_index[global_index];
      receive_counts[source_rank] += num_rows_in_first_seqno * num_columns;
    }
  }

  std::vector<int> receive_displacements;
  receive_displacements.reserve(num_processes);
  std::exclusive_scan(receive_counts.begin(), receive_counts.end(),
                      std::back_inserter(receive_displacements), 0);
  const size_t receive_buffer_size = receive_displacements.back() + receive_counts.back();
  std::vector<double> receive_buffer(receive_buffer_size);

  // Third, call allToAllv.
  comm.allToAllv(send_buffer.data(), send_counts.data(), send_displacements.data(),
                 receive_buffer.data(), receive_counts.data(), receive_displacements.data());

  // Delete the rows we have sent to other processes.

  for (size_t local_index = 0; local_index < num_local_chunks; ++local_index) {
    const size_t global_index = my_rank + local_index * num_processes;
    if (global_index_of_chunk_owning_first_seqno[global_index] == global_index)
      continue;

    Table &local_chunk = local_chunks[local_index];
    const size_t num_rows_to_erase = num_rows_in_first_seqno_by_global_index[global_index];
    for (size_t column = 0; column < local_chunk.size(); ++column)
      local_chunk[column].erase(local_chunk[column].begin(),
                                local_chunk[column].begin() + num_rows_to_erase);
  }

  // Append rows we have received to the appropriate chunks.

  std::vector<size_t> current_offset(num_processes);
  for (size_t global_index = 0; global_index < total_num_chunks; ++global_index) {
    const size_t first_seqno_owner_global_index =
        global_index_of_chunk_owning_first_seqno[global_index];
    ASSERT(first_seqno_owner_global_index <= global_index);
    if (first_seqno_owner_global_index == global_index)
      continue;
    const size_t first_seqno_owner_rank = first_seqno_owner_global_index % num_processes;
    if (first_seqno_owner_rank != my_rank)
      continue;

    const size_t local_index = first_seqno_owner_global_index / num_processes;
    const size_t source_rank = global_index % num_processes;
    const std::vector<double>::const_iterator receive_buffer_block_begin =
        receive_buffer.begin() + receive_displacements[source_rank] + current_offset[source_rank];
    const size_t num_rows_to_append = num_rows_in_first_seqno_by_global_index[global_index];
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
