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

#include "Engines/ODC/ParallelIoUtils.h"

// -----------------------------------------------------------------------------

using Column = std::vector<double>;
using Table = std::vector<std::vector<double>>;

namespace eckit {
  // Don't use the contracted output for these types: contracted output works only for integers.
  template <> struct VectorPrintSelector<Column> { typedef VectorPrintSimple selector; };
  template <> struct VectorPrintSelector<Table> { typedef VectorPrintSimple selector; };
} // namespace eckit

// -----------------------------------------------------------------------------

class ChunkParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(ChunkParameters, Parameters)
 public:
  oops::RequiredParameter<std::vector<double>> seqno{"seqno", this};
  oops::RequiredParameter<std::vector<double>> temperature{"temperature", this};
};

class MergeSeqnosSplitAcrossChunksTestParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(MergeSeqnosSplitAcrossChunksTestParameters, Parameters)
 public:
  oops::RequiredParameter<size_t> totalNumChunks{"total number of chunks", this};
  oops::RequiredParameter<std::vector<std::vector<ChunkParameters>>> chunksOnRanksAtInput{
    "chunks on ranks at input", this};
  oops::RequiredParameter<std::vector<std::vector<ChunkParameters>>> expectedChunksOnRanksAtOutput{
    "expected chunks on ranks at output", this};
};

class TestsParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(TestsParameters, Parameters)
 public:
  oops::RequiredParameter<std::vector<MergeSeqnosSplitAcrossChunksTestParameters>>
    mergeSeqnosSplitAcrossChunks{"mergeSeqnosSplitAcrossChunks", this};
};

// -----------------------------------------------------------------------------

CASE("mergeSeqnosSplitAcrossChunks") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  std::vector<eckit::LocalConfiguration> confs;
  conf.get("mergeSeqnosSplitAcrossChunks", confs);
  for (size_t jconf = 0; jconf < confs.size(); ++jconf) {
    oops::Log::test() << "Subcase " << jconf << std::endl;
    eckit::LocalConfiguration config = confs[jconf];
    MergeSeqnosSplitAcrossChunksTestParameters params;
    params.validateAndDeserialize(config);

    const eckit::mpi::Comm &comm = oops::mpi::world();
    const size_t rank = comm.rank();
    const std::vector<ChunkParameters> &inputChunkParameters =
        params.chunksOnRanksAtInput.value().at(rank);
    const std::vector<ChunkParameters> &expectedOutputChunkParameters =
        params.expectedChunksOnRanksAtOutput.value().at(rank);

    std::vector<std::vector<std::vector<double>>> chunks;
    for (const ChunkParameters &params : inputChunkParameters)
      chunks.push_back(Table({params.temperature.value(), params.seqno.value()}));
    std::vector<std::vector<std::vector<double>>> expectedOutputChunks;
    for (const ChunkParameters &params : expectedOutputChunkParameters)
      expectedOutputChunks.push_back(Table({params.temperature.value(), params.seqno.value()}));

    ioda::Engines::ODC::mergeSeqnosSplitAcrossChunks(comm, params.totalNumChunks,
                                                     2 /* num columns */,
                                                     1 /* seqno column index */,
                                                     chunks);
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
