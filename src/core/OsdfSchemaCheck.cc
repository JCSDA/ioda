/*
 * (C) Copyright 2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/core/OsdfSchemaCheck.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "eckit/exception/Exceptions.h"
#include "ioda/containers/FrameUtils.h"
#include "ioda/containers/IFrame.h"
#include "ioda/core/IodaUtils.h"
#include "oops/mpi/mpi.h"

namespace ioda {

namespace {
// The parts of a column's description that must match across MPI tasks.
using SchemaEntry = std::pair<std::string, osdf::consts::eDataTypes>;

std::string describeEntry(const std::vector<SchemaEntry> & schema, const std::size_t i) {
    if (i >= schema.size()) {
        return "no column";
    }
    return "'" + schema[i].first + "' (" + osdf::FrameUtils::typeName(schema[i].second) + ")";
}

// Serialized schemas hold, for each column in order, the column name, a null character and
// then a single byte holding the data type.
std::vector<SchemaEntry> parseSchema(std::vector<char>::const_iterator first,
                                     const std::vector<char>::const_iterator last) {
    std::vector<SchemaEntry> schema;
    while (first != last) {
        const auto nameEnd = std::find(first, last, '\0');
        ASSERT(nameEnd != last && nameEnd + 1 != last);
        schema.emplace_back(std::string(first, nameEnd),
                            static_cast<osdf::consts::eDataTypes>(*(nameEnd + 1)));
        first = nameEnd + 2;
    }
    return schema;
}

std::vector<std::string> columnNames(const std::vector<SchemaEntry> & schema) {
    std::vector<std::string> names;
    names.reserve(schema.size());
    for (const auto & entry : schema) {
        names.push_back(entry.first);
    }
    return names;
}

// Lists column names for an error message, up to a limit.
std::string formatColumnList(const std::vector<std::string> & names) {
    const std::size_t maxListed = 10;
    if (names.empty()) {
        return "none";
    }
    std::string list;
    for (std::size_t i = 0; i < std::min(names.size(), maxListed); ++i) {
        list += (i == 0 ? "" : ", ") + names[i];
    }
    if (names.size() > maxListed) {
        list += ", ... (" + std::to_string(names.size()) + " in total)";
    }
    return list;
}
}  // namespace

// @brief Check that the IFrame schema is identical on all MPI tasks in a communicator
// @detail This function first computes a hash of the schema on each task and does an allReduce to
// determine if the hashes are identical. So under expected conditions this function is light and
// efficient, so it can be called frequently without significant performance impact.
// Only when the hashes differ does it gather the full schema from all tasks to build a detailed
// error message. This is a collective call and must be made by every task in comm.
void osdfCheckSchemaAcrossRanks(const osdf::IFrame & frame,
                                const eckit::mpi::Comm & comm,
                                const std::string & context) {
    std::vector<char> localSchema;
    for (const auto & colName : frame.columnNames()) {
        localSchema.insert(localSchema.end(), colName.begin(), colName.end());
        localSchema.push_back('\0');
        localSchema.push_back(static_cast<char>(frame.getColumnType(colName)));
    }

    // 64-bit FNV-1a hash of the serialized schema. Unlike std::hash, it is guaranteed to give
    // the same value on every task.
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char c : localSchema) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ULL;
    }

    // The minima of hash and ~hash (bitwise NOT of hash) over all tasks equal this task's values if
    // and only if every task holds the same hash, so a single allReduce yields the result
    // on all tasks. The values are reduced as signed integers since only their equality matters.
    const std::vector<std::int64_t> localValues = {static_cast<std::int64_t>(hash),
                                                   static_cast<std::int64_t>(~hash)};
    std::vector<std::int64_t> minima = localValues;
    comm.allReduceInPlace(minima.begin(), minima.end(), eckit::mpi::min());
    if (minima == localValues) {
        return;  // Expected, fast path ends here: all tasks have the same schema
    }

    // The schemas differ. Gather them all so that every task can build the same error message.
    const std::size_t nTasks = comm.size();
    std::vector<int> schemaSizes(nTasks);
    comm.allGather(static_cast<int>(localSchema.size()), schemaSizes.begin(), schemaSizes.end());
    std::vector<char> allSchemas = localSchema;
    oops::mpi::allGatherv(comm, allSchemas);

    std::vector<std::vector<SchemaEntry>> schemas;
    auto first = allSchemas.cbegin();
    for (std::size_t rank = 0; rank < nTasks; ++rank) {
        schemas.push_back(parseSchema(first, first + schemaSizes[rank]));
        first += schemaSizes[rank];
    }

    std::vector<std::size_t> differingRanks;
    for (std::size_t rank = 1; rank < nTasks; ++rank) {
        if (schemas[rank] != schemas[0]) {
            differingRanks.push_back(rank);
        }
    }
    ASSERT(!differingRanks.empty());

    const std::size_t rank = differingRanks.front();
    std::size_t pos = 0;
    while (pos < schemas[0].size() && pos < schemas[rank].size() &&
           schemas[0][pos] == schemas[rank][pos]) {
        ++pos;
    }

    std::string rankList;
    for (std::size_t i = 0; i < differingRanks.size(); ++i) {
        rankList += (i == 0 ? "" : ", ") + std::to_string(differingRanks[i]);
    }

    const vectorDifference unshared =
        findElementsNotInBothVectors(columnNames(schemas[0]), columnNames(schemas[rank]));

    const std::string rankStr = "rank " + std::to_string(rank);
    std::string errMsg = context + ": the OSDF column schema differs across MPI tasks. "
        "Every task must hold the same columns, in the same order and with the same data types, "
        "even if it holds no locations.\n"
        "  Ranks whose schema differs from rank 0: " + rankList + "\n"
        "  rank 0 has " + std::to_string(schemas[0].size()) + " columns, " + rankStr + " has " +
        std::to_string(schemas[rank].size()) + "\n"
        "  First difference at column " + std::to_string(pos) + ": rank 0 has " +
        describeEntry(schemas[0], pos) + ", " + rankStr + " has " +
        describeEntry(schemas[rank], pos) + "\n"
        "  Columns on rank 0 but not on " + rankStr + ": " +
        formatColumnList(unshared.onlyInFirstVector) + "\n"
        "  Columns on " + rankStr + " but not on rank 0: " +
        formatColumnList(unshared.onlyInSecondVector);
    throw eckit::BadValue(errMsg, Here());
}

}  // namespace ioda
