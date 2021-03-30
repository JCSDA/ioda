/*
 * (C) Crown Copyright 2021 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/Variables/Has_Variables.h"

#include "ioda/Engines/Factory.h"
#include "ioda/Layout.h"
#include "ioda/ObsGroup.h"

#include "eckit/testing/Test.h"

using namespace eckit::testing;
using namespace ioda;

namespace ioda {
namespace test {

const int locations = 40;
const int channels = 30;
CASE("Stitch variables, remove originals defaulted as true") {
  typedef std::string str;
  str mappingFile = str(TEST_SOURCE_DIR) + "/hasvariables_stitching_map.yaml";
  Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;
  Engines::BackendCreationParameters backendParams;
  backendParams.fileName = "ioda-engines_hasvariables_stitch-file.hdf5";
  backendParams.action = Engines::BackendFileActions::Create;
  backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
  Group backend = ioda::Engines::constructBackend(backendName, backendParams);

  ioda::Variable completePart1 = backend.vars.create<str>("completeCombinationPart1", {3});
  completePart1.write<str>({str("a"), str("A"), str("1")});
  ioda::Variable completePart2 = backend.vars.create<str>("completeCombinationPart2", {3});
  completePart2.write<str>({str("b"), str("B"), str("2")});
  ioda::Variable completePart3 = backend.vars.create<str>("completeCombinationPart3", {3});
  completePart3.write<str>({str("c"), str("C"), str("3")});
  ioda::Variable incompletePart1 = backend.vars.create<str>("incompleteCombinationPart1", {3});
  incompletePart1.write<str>({str("a"), str("A"), str("1")});
  ioda::Variable incompletePart2 = backend.vars.create<str>("incompleteCombinationPart2", {3});
  incompletePart2.write<str>({str("b"), str("B"), str("2")});
  ioda::Variable oneVar = backend.vars.create<str>("oneVariableCombination", {5});
  oneVar.write<str>({str("foo"), str("bar"), str("baz"), str("lorem"), str("ipsum")});

  ObsGroup og = ObsGroup::generate(
          backend,
          {
            std::make_shared<NewDimensionScale<int>>(
            "nlocs", locations, ioda::Unlimited, locations),
            std::make_shared<NewDimensionScale<int>>(
            "nchans", channels, channels, channels), },
          detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroupODB,
                                             mappingFile));

  EXPECT(og.vars.exists(str("completeCombinationPart1")));
  EXPECT(og.vars.exists(str("completeCombinationPart2")));
  EXPECT(og.vars.exists(str("completeCombinationPart3")));
  EXPECT(og.vars.exists(str("incompleteCombinationPart1")));
  EXPECT(og.vars.exists(str("incompleteCombinationPart2")));
  EXPECT(og.vars.exists(str("oneVariableCombination")));
  std::vector<str> singleVarCombPreStitch;
  og.vars.open("oneVariableCombination").read<str>(singleVarCombPreStitch);
  og.vars.stitchComplementaryVariables();

  EXPECT(og.vars.exists(str("completeCombination")));
  std::vector<str> combinedVariable;
  (og.vars.open("completeCombination")).read<str>(combinedVariable);
  std::vector<str> expectedCombinedVariable{"abc", "ABC", "123"};
  EXPECT(combinedVariable == expectedCombinedVariable);
  EXPECT_NOT(og.vars.exists(str("completeCombinationPart1")));
  EXPECT_NOT(og.vars.exists(str("completeCombinationPart2")));
  EXPECT_NOT(og.vars.exists(str("completeCombinationPart3")));

  EXPECT_NOT(og.vars.exists(str("incompleteCombination")));
  EXPECT(og.vars.exists(str("incompleteCombinationPart1")));
  EXPECT(og.vars.exists(str("incompleteCombinationPart2")));

  //fails on this line
  EXPECT_NOT(og.vars.exists(str("oneVariableCombination")));
  EXPECT(og.vars.exists("oneVariableDerivedVariable"));
  std::vector<str> singleVarCombPostStitch;
  (og.vars.open("oneVariableDerivedVariable")).read<str>(singleVarCombPostStitch);
  EXPECT(singleVarCombPreStitch == singleVarCombPostStitch);
}

CASE("Stitch variables, remove originals set to false") {
  typedef std::string str;
  str mappingFile = str(TEST_SOURCE_DIR) + "/hasvariables_stitching_map.yaml";
  Engines::BackendNames backendName = Engines::BackendNames::Hdf5File;
  Engines::BackendCreationParameters backendParams;
  backendParams.fileName = "ioda-engines_hasvariables_stitch-file-originals-kept.hdf5";
  backendParams.action = Engines::BackendFileActions::Create;
  backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
  Group backend = ioda::Engines::constructBackend(backendName, backendParams);

  ioda::Variable completePart1 = backend.vars.create<str>("completeCombinationPart1", {3});
  completePart1.write<str>({str("a"), str("A"), str("1")});
  ioda::Variable completePart2 = backend.vars.create<str>("completeCombinationPart2", {3});
  completePart2.write<str>({str("b"), str("B"), str("2")});
  ioda::Variable completePart3 = backend.vars.create<str>("completeCombinationPart3", {3});
  completePart3.write<str>({str("c"), str("C"), str("3")});
  ioda::Variable incompletePart1 = backend.vars.create<str>("incompleteCombinationPart1", {3});
  incompletePart1.write<str>({str("a"), str("A"), str("1")});
  ioda::Variable incompletePart2 = backend.vars.create<str>("incompleteCombinationPart2", {3});
  incompletePart2.write<str>({str("b"), str("B"), str("2")});
  ioda::Variable oneVar = backend.vars.create<str>("oneVariableCombination", {5});
  oneVar.write<str>({str("foo"), str("bar"), str("baz"), str("lorem"), str("ipsum")});

  ObsGroup og = ObsGroup::generate(
          backend,
          {
            std::make_shared<NewDimensionScale<int>>(
            "nlocs", locations, ioda::Unlimited, locations),
            std::make_shared<NewDimensionScale<int>>(
            "nchans", channels, channels, channels), },
          detail::DataLayoutPolicy::generate(detail::DataLayoutPolicy::Policies::ObsGroupODB,
                                             mappingFile));
  EXPECT(og.vars.exists(str("completeCombinationPart1")));
  EXPECT(og.vars.exists(str("completeCombinationPart2")));
  EXPECT(og.vars.exists(str("completeCombinationPart3")));
  std::vector<str> completeCombinationPart2PreStitch;
  (og.vars.open("completeCombinationPart2")).read<str>(completeCombinationPart2PreStitch);

  og.vars.stitchComplementaryVariables(false);

  EXPECT(og.vars.exists(str("completeCombination")));
  std::vector<str> combinedVariable =
      (og.vars.open("completeCombination")).readAsVector<str>();
  std::vector<str> expectedCombinedVariable{"abc", "ABC", "123"};
  EXPECT(combinedVariable == expectedCombinedVariable);
  EXPECT(og.vars.exists(str("completeCombinationPart1")));
  EXPECT(og.vars.exists(str("completeCombinationPart2")));
  EXPECT(og.vars.exists(str("completeCombinationPart3")));
  std::vector<str> completeCombinationPart2PostStitch;
  (og.vars.open("completeCombinationPart2")).read<str>(completeCombinationPart2PostStitch);
  EXPECT(completeCombinationPart2PreStitch == completeCombinationPart2PostStitch);
}

}  // namespace test
}  // namespace ioda

int main(int argc, char** argv) {
  return run_tests(argc, argv);
}
