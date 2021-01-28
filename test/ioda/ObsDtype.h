/*
 * (C) Copyright 2021 Met Office UK
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSDTYPE_H_
#define TEST_IODA_OBSDTYPE_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/make_unique.hpp>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"

#include "ioda/core/ObsData.h"
#include "ioda/core/ParameterTraitsObsDtype.h"

namespace ioda {
namespace test {

class MyParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(MyParameters, Parameters)
 public:
  oops::RequiredParameter<ioda::ObsDtype> dtype{"dtype", this};
};

CASE("ioda/ObsDtype") {
  const eckit::Configuration &conf = ::test::TestEnvironment::config();

  {
    MyParameters params;
    params.validateAndDeserialize(conf.getSubConfiguration("int"));
    EXPECT(params.dtype == ObsDtype::Integer);
  }
  {
    MyParameters params;
    params.validateAndDeserialize(conf.getSubConfiguration("float"));
    EXPECT(params.dtype == ObsDtype::Float);
  }
  {
    MyParameters params;
    params.validateAndDeserialize(conf.getSubConfiguration("string"));
    EXPECT(params.dtype == ObsDtype::String);
  }
  {
    MyParameters params;
    params.validateAndDeserialize(conf.getSubConfiguration("datetime"));
    EXPECT(params.dtype == ObsDtype::DateTime);
  }
  {
    MyParameters params;
    EXPECT_THROWS(params.validateAndDeserialize(conf.getSubConfiguration("invalid")));
  }
}

class ObsDtype : public oops::Test {
 private:
  std::string testid() const override {return "test::ioda::ObsDtype";}

  void register_tests() const override {}

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSDTYPE_H_
