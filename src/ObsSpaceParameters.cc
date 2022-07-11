/*
 * (C) Copyright 2017-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpaceParameters.h"

namespace ioda {

void ObsTopLevelParameters::deserialize(util::CompositePath &path,
                                        const eckit::Configuration &config)  {
    oops::Parameters::deserialize(path, config);

    // Check that only one set of options controlling the obsio is present.
    const int numIoOptionsSet =
        static_cast<int>(obsInFile.value() != boost::none) +
        static_cast<int>(obsGenerate.value() != boost::none &&
                         obsGenerate.value()->random.value() != boost::none) +
        static_cast<int>(obsGenerate.value() != boost::none &&
                         obsGenerate.value()->list.value() != boost::none) +
        static_cast<int>(source.value() != boost::none);
    if (numIoOptionsSet != 1)
        throw eckit::UserError(
            path.path() + ": Exactly one of the following options must be set: "
            "obsdatain, generate.list, generate.random, source", Here());

    // If the derived variables list is present, check that the observed variables list
    // is also present.
    if (config.has("derived variables") && !(config.has("observed variables")))
        throw eckit::UserError("If a derived variables list is specified then the"
                          " observed variables list must also be specified.", Here());

    // Store the contents of the `obsdatain` or `generate` section (if present)
    // in the `source` member variable.
    if (obsInFile.value() != boost::none) {
        eckit::LocalConfiguration sourceConfig;
        obsInFile.value()->serialize(sourceConfig);
        sourceConfig.set("type", "FileRead");

        eckit::LocalConfiguration config;
        config.set("source", sourceConfig);
        util::CompositePath path;
        source.deserialize(path, config);
    } else if (obsGenerate.value() != boost::none) {
        const LegacyObsGenerateParameters &legacyParams = *obsGenerate.value();
        eckit::LocalConfiguration sourceConfig;
        // Store all these parameters at the root level of sourceConfig.
        legacyParams.obsGrouping.serialize(sourceConfig);
        legacyParams.maxFrameSize.serialize(sourceConfig);
        legacyParams.obsErrors.serialize(sourceConfig);
        if (legacyParams.list.value() != boost::none) {
            legacyParams.list.value()->serialize(sourceConfig);
            sourceConfig.set("type", "GenerateList");
        } else if (legacyParams.random.value() != boost::none) {
            legacyParams.random.value()->serialize(sourceConfig);
            sourceConfig.set("type", "GenerateRandom");
        }

        eckit::LocalConfiguration config;
        config.set("source", sourceConfig);
        util::CompositePath path;
        source.deserialize(path, config);
    }
}

}  // namespace ioda
