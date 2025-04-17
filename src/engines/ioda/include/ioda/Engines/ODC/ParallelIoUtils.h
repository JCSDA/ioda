#pragma once
/*
 * (C) Crown copyright 2025, Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <vector>

#include "ioda/defs.h"

namespace eckit::mpi {
class Comm;
}

namespace ioda {
namespace Engines {
namespace ODC {

/// \brief Split `total` into `num_chunks` approximately equal chunks.
///
/// For example, splitting 14 into 4 chunks yields [4, 4, 3, 3].
std::vector<size_t> splitIntoChunksAsEvenlyAsPossible(size_t total, size_t num_chunks);

/// \brief Exchange data read from an ODB file by individual MPI processes so that all rows from
/// each seqno end up on a single process.
///
/// \param comm
///   MPI communicator encompassing all processes participating in the parallel I/O.
/// \param total_num_chunks
///   Total number of chunks of ODB frames read by all the processes in `comm`.
/// \param num_columns
///   Number of columns read from the ODB file.
/// \param seqno_column_index
///   Index of the `seqno` column.
/// \param[inout] local_chunks
///   On input, the tables of data read from successive chunks of ODB frames by the calling process;
///   local_chunks[i][j][k] is the value in the kth row in the jth column of the ith chunk.
///   The number of columns in each chunk is expected to be `num_columns`.
///   On output, these tables will be modified so that all consecutive rows with the same seqno end
///   up in the table that, on input, contained the first of these rows.
void mergeSeqnosSplitAcrossChunks(const eckit::mpi::Comm &comm,
                                  size_t total_num_chunks,
                                  size_t num_columns,
                                  int seqno_column_index,
                                  std::vector<std::vector<std::vector<double>>> &local_chunks);

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
