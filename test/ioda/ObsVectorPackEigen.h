/*
 * (C) Copyright 2021- UCAR.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSVECTORPACKEIGEN_H_
#define TEST_IODA_OBSVECTORPACKEIGEN_H_

#include <Eigen/Dense>

#include <numeric>
#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"
#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

/// \brief tests ObsVector::maskAndSerialize method and mask methods
/// \details Tests that:
/// - norm of the vector returned by ObsVector::maskAndSerialize is close to the
///   reference specified in yaml (reference local masked norm);
/// - norm of a random vector with mask applied is different from the same vector
///   before mask application;
void testPackEigen() {
  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);
  const util::TimeWindow timeWindow
    (::test::TestEnvironment::config().getSubConfiguration("time window"));

  for (std::size_t jj = 0; jj < conf.size(); ++jj) {
     eckit::LocalConfiguration obsconf(conf[jj], "obs space");
     ioda::ObsSpace obsdb(obsconf, oops::mpi::world(), timeWindow, oops::mpi::myself());

     const size_t rank = obsdb.distribution()->rank();
     ioda::ObsVector obsvec(obsdb, "ObsValue");

     const std::string maskname = conf[jj].getString("mask variable");
     ioda::ObsDataVector<int> mask(obsdb, obsdb.assimvariables(), maskname);
     // emulate the flow in the applications: ObsDataVector<float> obs errors
     // get masked with ObsDataVector<int> QC flags; copied into ObsVector
     // obs errors, and used as a mask for another ObsVector (e.g. H(x)).
     ioda::ObsDataVector<float> masked(obsdb, obsdb.assimvariables());
     ioda::ObsVector maskvector(obsdb);
     masked.mask(mask);
     maskvector = masked;

     // test maskAndSerialize
     std::vector<double> packed;
     obsvec.maskAndSerialize(maskvector, packed);
     const std::vector<double> ref_norms =
                       conf[jj].getDoubleVector("reference local masked norm");
     const std::vector<size_t> ref_sizes =
                       conf[jj].getUnsignedVector("reference local masked nobs");
     EXPECT_EQUAL(packed.size(), ref_sizes[rank]);
     double norm = std::sqrt(std::inner_product(packed.begin(), packed.end(), packed.begin(), 0.0));
     EXPECT(oops::is_close(norm, ref_norms[rank], 1.e-5));

     // Test directly masking the ObsVector with the ObsDataVector<int> mask
     ioda::ObsVector obsvec2(obsdb, "ObsValue");
     ioda::ObsVector maskvector2(obsdb);
     maskvector2.mask(mask);
     std::vector<double> packed2;
     obsvec2.maskAndSerialize(maskvector2, packed2);
     const std::vector<double> ref_norms2 = conf[jj].getDoubleVector("reference local masked norm");
     const std::vector<size_t> ref_sizes2
       = conf[jj].getUnsignedVector("reference local masked nobs");
     EXPECT_EQUAL(packed2.size(), ref_sizes2[rank]);
     double norm2
       = std::sqrt(std::inner_product(packed2.begin(), packed2.end(), packed2.begin(), 0.0));
     EXPECT(oops::is_close(norm2, ref_norms2[rank], 1.e-5));
  }
}

// -----------------------------------------------------------------------------

class ObsVectorPackEigen : public oops::Test {
 public:
  ObsVectorPackEigen() = default;
  virtual ~ObsVectorPackEigen() = default;

 private:
  std::string testid() const override {return "test::ObsVector<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

     ts.emplace_back(CASE("ioda/ObsVectorPackEigen/testPackEigen")
      { testPackEigen(); });
  }

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSVECTORPACKEIGEN_H_
