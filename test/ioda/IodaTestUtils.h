/*
 * (C) Crown copyright 2026 Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */


#pragma once

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include "eckit/config/Configuration.h"
#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"

namespace ioda {
namespace test {

// Returns true if the same elements exist in both vectors with the same multiplicities
// but not necessarily in the same order
bool unorderedVectorComparison(std::vector<std::string> firstVector,
                               std::vector<std::string> secondVector) {
  std::sort(firstVector.begin(), firstVector.end());
  std::sort(secondVector.begin(), secondVector.end());
  return (firstVector == secondVector);
}

// -----------------------------------------------------------------------------
/// \brief Insert a tag into the output file name of an obs space configuration.
///
/// \details
/// Inserts \p tag ahead of the file name suffix, so that "testoutput/diagout.nc4"
/// becomes "testoutput/diagout_osdf.nc4". Does nothing if the obs space writes no
/// output file. The IO pool appends its own "_0000", "_0001", ... after the tag, which
/// is the base path form that tools/ioda_compare_obs.py expands.
///
/// \param obsConf an individual "obs space" configuration, updated in place
/// \param tag the string to insert, including any leading separator
inline void tagObsOutputFile(eckit::LocalConfiguration & obsConf,
                             const std::string & tag) {
  const std::string fileKey = "obsdataout.engine.obsfile";
  if (!obsConf.has(fileKey)) return;

  std::string fileName = obsConf.getString(fileKey);
  const std::size_t suffixPos = fileName.find_last_of('.');
  if (suffixPos == std::string::npos) {
    fileName += tag;
  } else {
    fileName.insert(suffixPos, tag);
  }
  obsConf.set(fileKey, fileName);
}

// -----------------------------------------------------------------------------
/// \brief Reject an obs space whose files are shared with another test.
///
/// \details
/// This function checks for conditions where problems could arise if the same test is run twice
/// only with different output tagging. (e.g. _osdf vs _obsgroup) because of shared input or output
/// files. If this function throws, the two tests should instead be split into two YAML files, one
/// for each container, so that the two runs do not collide.
///
/// \param obsConf an individual "obs space" configuration
/// \param envValue the IODA_TEST_CONTAINER value that triggered the tagging
inline void checkObsSpaceNotChained(const eckit::LocalConfiguration & obsConf,
                                    const std::string & envValue) {
  const std::string prefix = "IODA_TEST_CONTAINER=" + envValue + " cannot be used here: ";
  const std::string remedy = " Use a separate YAML file for this container instead.";

  const std::string inputKey = "obsdatain.engine.obsfile";
  if (obsConf.has(inputKey)) {
    std::string inputFile = obsConf.getString(inputKey);
    if (inputFile.rfind("./", 0) == 0) inputFile.erase(0, 2);
    if (inputFile.rfind("testoutput/", 0) == 0 || inputFile.rfind("testwork/", 0) == 0) {
      throw eckit::UserError(prefix + "this obs space reads another test's output (" +
                             inputFile + "), which is not renamed by output tagging." +
                             remedy, Here());
    }
  }

  const std::string prepKey = "io pool.file preparation.output file";
  if (obsConf.has(prepKey)) {
    throw eckit::UserError(prefix + "this obs space prepares input files for another test (" +
                           obsConf.getString(prepKey) + "), which is not renamed by output "
                           "tagging." + remedy, Here());
  }
}

// -----------------------------------------------------------------------------
/// \brief Apply a test-wide obs data container default to an individual obs space config.
///
/// \details
/// This is the ioda-native counterpart to oops::detail::applyObsSpaceDefaults: it reads
/// the setting from the top level of the test configuration, or from an environment
/// variable, so one YAML file can be registered as two ctests (one per container)
/// without duplication.
///
/// Precedence, highest first:
///   1. the obs space's own "use data frame container" setting
///   2. a top-level "obs data container" key in the test YAML
///   3. the IODA_TEST_CONTAINER environment variable
///
/// When none of these is present this is a no-op, so existing test configurations retain
/// their current behaviour (the ObsGroup container).
///
/// Only when the environment variable is what selected the container does this also tag
/// the obs space's output file name (so the two runs do not collide) and reject an obs
/// space that shares files with another test.
///
/// TODO(someone): Remove this function, the IODA_TEST_CONTAINER environment variable and
/// oops::detail::applyObsSpaceDefaults once the migration to the OSDF container is
/// complete and the ObsGroup container has been retired.
///
/// \param topLevelConf the whole test configuration (not the "observations" section)
/// \param obsConf an individual "obs space" configuration, updated in place
inline void applyContainerDefault(const eckit::Configuration & topLevelConf,
                                  eckit::LocalConfiguration & obsConf) {
  // A setting on the individual obs space always takes precedence.
  if (obsConf.has("use data frame container")) return;

  std::string container;
  bool fromEnvironment = false;
  if (!topLevelConf.get("obs data container", container)) {
    const char * envContainer = std::getenv("IODA_TEST_CONTAINER");
    if (envContainer == nullptr) return;
    container = envContainer;
    fromEnvironment = true;
  }

  std::string outputTag;
  if (container == "OSDF") {
    obsConf.set("use data frame container", true);
    outputTag = "_osdf";
  } else if (container == "ObsGroup") {
    obsConf.set("use data frame container", false);
    outputTag = "_obsgroup";
  } else {
    throw eckit::BadValue("Unknown 'obs data container': " + container +
                          ", expected 'ObsGroup' or 'OSDF'", Here());
  }

  if (fromEnvironment) {
    checkObsSpaceNotChained(obsConf, container);
    tagObsOutputFile(obsConf, outputTag);
  }
}

}  // namespace test
}  // namespace ioda
