/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_FILEFORMAT_H_
#define TEST_IODA_FILEFORMAT_H_

#include <memory>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

#include "ioda/core/FileFormat.h"
#include "ioda/core/ParameterTraitsFileFormat.h"

namespace ioda {
namespace test {

class ConversionTestParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(ConversionTestParameters, Parameters)
 public:
  oops::RequiredParameter<ioda::FileFormat> format{"format", this};
};

class FormatDeterminationParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(FormatDeterminationParameters, Parameters)
 public:
  oops::RequiredParameter<ioda::FileFormat> format{"format", this};
  oops::RequiredParameter<ioda::FileFormat> expectedFormat{"expected format", this};
  oops::RequiredParameter<std::string> path{"path", this};
};

CASE("ioda/FileFormat") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();

  {
    ConversionTestParameters params;
    params.validateAndDeserialize(conf.getSubConfiguration("auto"));
    EXPECT(params.format == FileFormat::AUTO);
  }
  {
    ConversionTestParameters params;
    params.validateAndDeserialize(conf.getSubConfiguration("hdf5"));
    EXPECT(params.format == FileFormat::HDF5);
  }
  {
    ConversionTestParameters params;
    params.validateAndDeserialize(conf.getSubConfiguration("odb"));
    EXPECT(params.format == FileFormat::ODB);
  }
  {
    ConversionTestParameters params;
    EXPECT_THROWS(params.validateAndDeserialize(conf.getSubConfiguration("invalid")));
  }
}

CASE("ioda/determineFileFormat") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();
  for (const eckit::LocalConfiguration &caseConf : conf.getSubConfigurations("determine format")) {
    FormatDeterminationParameters params;
    params.validateAndDeserialize(caseConf);

    FileFormat expectedFormat = params.expectedFormat;
    FileFormat format = determineFileFormat(params.path, params.format);
    EXPECT(format == expectedFormat);
  }
}

class FileFormat : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::FileFormat";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_FILEFORMAT_H_
