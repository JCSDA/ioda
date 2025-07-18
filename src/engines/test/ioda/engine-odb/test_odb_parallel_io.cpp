/*
 * (C) Crown Copyright 2025 UK Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Run.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

#include "Engines/ODC/ParallelIoUtils.h"

// -----------------------------------------------------------------------------

using Column = std::vector<double>;
using Chunk = std::vector<std::vector<double>>;

namespace eckit {
  // Don't use the contracted output for these types: contracted output works only for integers.
  template <> struct VectorPrintSelector<Column> { typedef VectorPrintSimple selector; };
  template <> struct VectorPrintSelector<Chunk> { typedef VectorPrintSimple selector; };
} // namespace eckit

// -----------------------------------------------------------------------------

using Columns = std::map<std::string, Column>;

std::vector<Chunk> makeChunks(const std::vector<Columns> &columnsByChunk,
                              const std::vector<std::string> &columnNames)
{
  std::vector<Chunk> chunks;
  for (const Columns &columns : columnsByChunk) {
    Chunk chunk;
    for (const std::string &name : columnNames)
      chunk.push_back(columns.at(name));
    chunks.push_back(chunk);
  }
  return chunks;
}

// -----------------------------------------------------------------------------

class MergeRecordsSplitAcrossChunksTestParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(MergeRecordsSplitAcrossChunksTestParameters, Parameters)
 public:
  oops::RequiredParameter<std::vector<std::string>> columns{"columns", this};
  oops::RequiredParameter<std::vector<std::string>> recordIdColumns{"record id columns", this};
  oops::RequiredParameter<size_t> totalNumChunks{"total number of chunks", this};
  oops::RequiredParameter<std::vector<std::vector<Columns>>> chunksOnRanksAtInput{
    "chunks on ranks at input", this};
  oops::RequiredParameter<std::vector<std::vector<Columns>>> expectedChunksOnRanksAtOutput{
    "expected chunks on ranks at output", this};
};

class TestsParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(TestsParameters, Parameters)
 public:
  oops::RequiredParameter<std::vector<MergeRecordsSplitAcrossChunksTestParameters>>
    mergeRecordsSplitAcrossChunks{"mergeRecordsSplitAcrossChunks", this};
};

// -----------------------------------------------------------------------------

CASE("mergeRecordsSplitAcrossChunks") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  std::vector<eckit::LocalConfiguration> confs;
  conf.get("mergeRecordsSplitAcrossChunks", confs);
  for (size_t jconf = 0; jconf < confs.size(); ++jconf) {
    oops::Log::test() << "Subcase " << jconf << std::endl;
    eckit::LocalConfiguration config = confs[jconf];
    MergeRecordsSplitAcrossChunksTestParameters params;
    params.validateAndDeserialize(config);

    const std::vector<std::string> &columnNames = params.columns;
    const std::vector<std::string> &recordIdColumnNames = params.recordIdColumns;

    const eckit::mpi::Comm &comm = oops::mpi::world();
    const size_t rank = comm.rank();

    const std::vector<Columns> &inputChunkColumns =
        params.chunksOnRanksAtInput.value().at(rank);
    std::vector<Chunk> chunks = makeChunks(inputChunkColumns, columnNames);

    std::vector<int> recordIdColumnIndices;
    for (size_t i = 0; i < columnNames.size(); ++i)
      if (std::find(recordIdColumnNames.begin(), recordIdColumnNames.end(),
                    columnNames[i]) != recordIdColumnNames.end())
        recordIdColumnIndices.push_back(static_cast<int>(i));

    ioda::Engines::ODC::mergeRecordsSplitAcrossChunks(comm, params.totalNumChunks,
                                                      columnNames.size() /* num columns */,
                                                      recordIdColumnIndices,
                                                      chunks);

    const std::vector<Columns> &expectedOutputChunkColumns =
        params.expectedChunksOnRanksAtOutput.value().at(rank);
    const std::vector<Chunk> expectedOutputChunks = makeChunks(expectedOutputChunkColumns,
                                                               columnNames);
    EXPECT_EQUAL(chunks, expectedOutputChunks);
  }
}

// -----------------------------------------------------------------------------

class OdbParallelIo : public oops::Test {
 private:
  std::string testid() const override {return "ioda::test::OdbParallelIo";}

  void register_tests() const override {}

  void clear() const override {}
};

// -----------------------------------------------------------------------------

int main(int argc, char **argv) {
  oops::Run run(argc, argv);
  OdbParallelIo tests;
  return run.execute(tests);
}
